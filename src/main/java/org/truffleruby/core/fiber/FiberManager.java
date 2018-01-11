/*
 * Copyright (c) 2013, 2018 Oracle and/or its affiliates. All rights reserved. This
 * code is released under a tri EPL/GPL/LGPL license. You can use it,
 * redistribute it and/or modify it under the terms of the:
 *
 * Eclipse Public License version 1.0
 * GNU General Public License version 2
 * GNU Lesser General Public License version 2.1
 */
package org.truffleruby.core.fiber;

import java.util.Collections;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.LinkedBlockingQueue;

import org.truffleruby.Layouts;
import org.truffleruby.RubyContext;
import org.truffleruby.RubyLanguage;
import org.truffleruby.core.proc.ProcOperations;
import org.truffleruby.core.thread.ThreadManager;
import org.truffleruby.core.thread.ThreadManager.BlockingAction;
import org.truffleruby.language.RubyGuards;
import org.truffleruby.language.control.BreakException;
import org.truffleruby.language.control.ExitException;
import org.truffleruby.language.control.KillException;
import org.truffleruby.language.control.RaiseException;
import org.truffleruby.language.control.ReturnException;
import org.truffleruby.language.control.TerminationException;
import org.truffleruby.language.objects.ObjectIDOperations;

import com.oracle.truffle.api.CompilerDirectives.TruffleBoundary;
import com.oracle.truffle.api.nodes.Node;
import com.oracle.truffle.api.object.DynamicObject;
import com.oracle.truffle.api.object.DynamicObjectFactory;
import com.oracle.truffle.api.profiles.BranchProfile;
import com.oracle.truffle.api.source.SourceSection;

/**
 * Manages Ruby {@code Fiber} objects for a given Ruby thread.
 */
public class FiberManager {

    public static final String NAME_PREFIX = "Ruby Fiber";

    private final RubyContext context;
    private final DynamicObject rootFiber;
    private DynamicObject currentFiber;
    private final Set<DynamicObject> runningFibers = newFiberSet();

    private final Map<Thread, DynamicObject> rubyFiberForeignMap = new ConcurrentHashMap<>();
    private final ThreadLocal<DynamicObject> rubyFiber = ThreadLocal.withInitial(() -> rubyFiberForeignMap.get(Thread.currentThread()));

    public FiberManager(RubyContext context, DynamicObject rubyThread) {
        this.context = context;
        this.rootFiber = createRootFiber(context, rubyThread);
        this.currentFiber = rootFiber;
    }

    @TruffleBoundary
    private static Set<DynamicObject> newFiberSet() {
        return Collections.synchronizedSet(Collections.newSetFromMap(new WeakHashMap<DynamicObject, Boolean>()));
    }

    private DynamicObject[] iterateRunningFibers() {
        // We need to make a copy while holding the lock
        // Collections.synchronizedSet() does not synchronize iterator()
        return runningFibers.toArray(new DynamicObject[0]);
    }

    public DynamicObject getRootFiber() {
        return rootFiber;
    }

    public DynamicObject getCurrentFiber() {
        assert Layouts.THREAD.getFiberManager(context.getThreadManager().getCurrentThread()) == this :
            "Trying to read the current Fiber of another Thread which is inherently racy";
        return currentFiber;
    }

    // If the currentFiber is read from another Ruby Thread,
    // there is no guarantee that fiber will remain the current one
    // as it could switch to another Fiber before the actual operation on the returned fiber.
    public DynamicObject getCurrentFiberRacy() {
        return currentFiber;
    }

    @TruffleBoundary
    public DynamicObject getRubyFiberFromCurrentJavaThread() {
        return rubyFiber.get();
    }

    private void setCurrentFiber(DynamicObject fiber) {
        assert RubyGuards.isRubyFiber(fiber);
        currentFiber = fiber;
    }

    private DynamicObject createRootFiber(RubyContext context, DynamicObject thread) {
        return createFiber(context, thread, context.getCoreLibrary().getFiberFactory(), "root Fiber for Thread", true);
    }

    public DynamicObject createFiber(RubyContext context, DynamicObject thread, DynamicObjectFactory factory, String name, boolean rootFiber) {
        assert RubyGuards.isRubyThread(thread);

        final DynamicObject fiber = Layouts.FIBER.createFiber(
                factory,
                new FiberData(context),
                newMessageQueue(),
                thread,
                false);

        if (!rootFiber) {
            final BlockingQueue<FiberMessage> queue = Layouts.FIBER.getMessageQueue(fiber);
            context.getFinalizationService().addFinalizer(fiber, FiberManager.class, () -> shutdownFiberWhenUnreachable(queue));
        }

        return fiber;
    }

    public static void shutdownFiberWhenUnreachable(BlockingQueue<FiberMessage> queue) {
        System.err.println("Killing fiber " + queue.hashCode());
        queue.add(new FiberShutdownMessage());
    }

    @TruffleBoundary
    private static LinkedBlockingQueue<FiberMessage> newMessageQueue() {
        return new LinkedBlockingQueue<>();
    }

    public void initialize(DynamicObject fiber, DynamicObject block, Node currentNode) {
        context.getThreadManager().spawnFiber(() -> fiberMain(context, fiber, block, currentNode));

        waitForInitialization(context, fiber, currentNode);
    }

    /** Wait for full initialization of the new fiber */
    public static void waitForInitialization(RubyContext context, DynamicObject fiber, Node currentNode) {
        final CountDownLatch initializedLatch = Layouts.FIBER.getFiberData(fiber).getInitializedLatch();

        context.getThreadManager().runUntilResultKeepStatus(currentNode, () -> {
            initializedLatch.await();
            return BlockingAction.SUCCESS;
        });
    }

    private static final BranchProfile UNPROFILED = BranchProfile.create();

    private void fiberMain(RubyContext context, DynamicObject fiber, DynamicObject block, Node currentNode) {
        assert fiber != rootFiber : "Root Fibers execute threadMain() and not fiberMain()";

        final FiberData fiberData = Layouts.FIBER.getFiberData(fiber);
        final BlockingQueue<FiberMessage> messageQueue = Layouts.FIBER.getMessageQueue(fiber);

        final Thread thread = Thread.currentThread();
        final SourceSection sourceSection = Layouts.PROC.getSharedMethodInfo(block).getSourceSection();
        final String oldName = thread.getName();
        thread.setName(NAME_PREFIX + " id=" + thread.getId() + " from " + RubyLanguage.fileLine(sourceSection));

        start(fiber, thread);
        try {

            final Object[] args = waitForResume(fiber, messageQueue);
            final Object result;
            try {
                result = ProcOperations.rootCall(block, args);
            } finally {
                // Make sure that other fibers notice we are dead before they gain control back
                fiberData.setAlive(false);
            }
            resume(fiber, getReturnFiber(fiberData, currentNode, UNPROFILED), FiberOperation.YIELD, result);

        // Handlers in the same order as in ThreadManager
        } catch (KillException | ExitException | RaiseException e) {
            // Propagate the exception until it reaches the root Fiber
            sendExceptionToParentFiber(fiberData, e, currentNode);
        } catch (FiberShutdownException e) {
            // Ends execution of the Fiber
        } catch (BreakException e) {
            sendExceptionToParentFiber(fiberData, new RaiseException(context.getCoreExceptions().breakFromProcClosure(currentNode)), currentNode);
        } catch (ReturnException e) {
            sendExceptionToParentFiber(fiberData, new RaiseException(context.getCoreExceptions().unexpectedReturn(currentNode)), currentNode);
        } finally {
            cleanup(fiberData, thread);
            thread.setName(oldName);
        }
    }

    private void sendExceptionToParentFiber(FiberData fiberData, RuntimeException exception, Node currentNode) {
        addToMessageQueue(getReturnFiber(fiberData, currentNode, UNPROFILED), new FiberExceptionMessage(exception));
    }

    public DynamicObject getReturnFiber(FiberData currentFiber, Node currentNode, BranchProfile errorProfile) {
        assert currentFiber == Layouts.FIBER.getFiberData(this.currentFiber);

        if (currentFiber == Layouts.FIBER.getFiberData(rootFiber)) {
            errorProfile.enter();
            throw new RaiseException(context.getCoreExceptions().yieldFromRootFiberError(currentNode));
        }

        final DynamicObject parentFiber = currentFiber.getLastResumedByFiber();
        if (parentFiber != null) {
            currentFiber.setLastResumedByFiber(null);
            return parentFiber;
        } else {
            return rootFiber;
        }
    }

    @TruffleBoundary
    private void addToMessageQueue(DynamicObject fiber, FiberMessage message) {
        Layouts.FIBER.getMessageQueue(fiber).add(message);
    }

    /**
     * Send the Java thread that represents this fiber to sleep until it receives a resume or exit
     * message.
     */
    @TruffleBoundary
    private Object[] waitForResume(DynamicObject fiber, BlockingQueue<FiberMessage> messageQueue) {
        final FiberMessage message = context.getThreadManager().runUntilResultKeepStatus(null,
                () -> messageQueue.take());

        setCurrentFiber(fiber);

        if (message instanceof FiberShutdownMessage) {
            throw new FiberShutdownException();
        } else if (message instanceof FiberExceptionMessage) {
            throw ((FiberExceptionMessage) message).getException();
        } else if (message instanceof FiberResumeMessage) {
            final FiberResumeMessage resumeMessage = (FiberResumeMessage) message;
            assert context.getThreadManager().getCurrentThread() == Layouts.FIBER.getRubyThread(resumeMessage.getSendingFiber());
            if (resumeMessage.getOperation() == FiberOperation.RESUME) {
                Layouts.FIBER.getFiberData(fiber).setLastResumedByFiber(resumeMessage.getSendingFiber());
            }
            return resumeMessage.getArgs();
        } else {
            throw new UnsupportedOperationException();
        }
    }

    /**
     * Send a resume message to a fiber by posting into its message queue. Doesn't explicitly notify
     * the Java thread (although the queue implementation may) and doesn't wait for the message to
     * be received.
     */
    private void resume(DynamicObject fromFiber, DynamicObject fiber, FiberOperation operation, Object... args) {
        addToMessageQueue(fiber, new FiberResumeMessage(operation, fromFiber, args));
    }

    public Object[] transferControlTo(DynamicObject fromFiber, DynamicObject fiber, FiberOperation operation, Object[] args) {
        resume(fromFiber, fiber, operation, args);
        final BlockingQueue<FiberMessage> fromQueue = Layouts.FIBER.getMessageQueue(fromFiber);
        return waitForResume(fromFiber, fromQueue);
    }

    public void start(DynamicObject fiber, Thread javaThread) {
        final ThreadManager threadManager = context.getThreadManager();

        if (Thread.currentThread() == javaThread) {
            rubyFiber.set(fiber);
        }
        if (!threadManager.isRubyManagedThread(javaThread)) {
            rubyFiberForeignMap.put(javaThread, fiber);
        }

        final FiberData fiberData = Layouts.FIBER.getFiberData(fiber);
        fiberData.setThread(javaThread);

        final DynamicObject rubyThread = Layouts.FIBER.getRubyThread(fiber);
        threadManager.initializeValuesForJavaThread(rubyThread, javaThread);

        runningFibers.add(fiber);

        if (threadManager.isRubyManagedThread(javaThread)) {
            context.getSafepointManager().enterThread();
        }

        // fully initialized
        fiberData.getInitializedLatch().countDown();
    }

    public void cleanup(FiberData fiberData, Thread javaThread) {
        fiberData.setAlive(false);

        if (context.getThreadManager().isRubyManagedThread(javaThread)) {
            context.getSafepointManager().leaveThread();
        }

        context.getThreadManager().cleanupValuesForJavaThread(javaThread);

        // TODO: should actively remove it or check alive instead?
        // runningFibers.remove(fiber);

        fiberData.setThread(null);

        if (Thread.currentThread() == javaThread) {
            rubyFiber.remove();
        }
        rubyFiberForeignMap.remove(javaThread);

        fiberData.getFinishedLatch().countDown();
    }

    @TruffleBoundary
    public void shutdown(Thread javaThread) {
        // All Fibers except the current one are in waitForResume(),
        // so sending a FiberShutdownMessage is enough to finish them.
        // This also avoids the performance cost of a safepoint.
        for (DynamicObject fiber : iterateRunningFibers()) {
            if (fiber != rootFiber) {
                addToMessageQueue(fiber, new FiberShutdownMessage());

                // Wait for the Fiber to finish so we only run one Fiber at a time
                final CountDownLatch finishedLatch = Layouts.FIBER.getFiberData(fiber).getFinishedLatch();
                context.getThreadManager().runUntilResultKeepStatus(null, () -> {
                    finishedLatch.await();
                    return BlockingAction.SUCCESS;
                });
            }
        }

        cleanup(Layouts.FIBER.getFiberData(rootFiber), javaThread);
    }

    public String getFiberDebugInfo() {
        final StringBuilder builder = new StringBuilder();

        for (DynamicObject fiber : iterateRunningFibers()) {
            builder.append("  fiber @");
            builder.append(ObjectIDOperations.verySlowGetObjectID(context, fiber));
            builder.append(" #");

            final Thread thread = Layouts.FIBER.getFiberData(fiber).getThread();

            if (thread == null) {
                builder.append("(no Java thread)");
            } else {
                builder.append(thread.getId());
            }

            if (fiber == rootFiber) {
                builder.append(" (root)");
            }

            if (fiber == currentFiber) {
                builder.append(" (current)");
            }

            builder.append("\n");
        }

        if (builder.length() == 0) {
            return "  no fibers\n";
        } else {
            return builder.toString();
        }
    }

    public interface FiberMessage {
    }

    private static class FiberResumeMessage implements FiberMessage {

        private final FiberOperation operation;
        private final DynamicObject sendingFiber;
        private final Object[] args;

        public FiberResumeMessage(FiberOperation operation, DynamicObject sendingFiber, Object[] args) {
            assert RubyGuards.isRubyFiber(sendingFiber);
            this.operation = operation;
            this.sendingFiber = sendingFiber;
            this.args = args;
        }

        public FiberOperation getOperation() {
            return operation;
        }

        public DynamicObject getSendingFiber() {
            return sendingFiber;
        }

        public Object[] getArgs() {
            return args;
        }

    }

    /**
     * Used to cleanup and terminate Fibers when the parent Thread dies.
     */
    private static class FiberShutdownException extends TerminationException {
        private static final long serialVersionUID = 1522270454305076317L;
    }

    private static class FiberShutdownMessage implements FiberMessage {
    }

    private static class FiberExceptionMessage implements FiberMessage {

        private final RuntimeException exception;

        public FiberExceptionMessage(RuntimeException exception) {
            this.exception = exception;
        }

        public RuntimeException getException() {
            return exception;
        }

    }

}
