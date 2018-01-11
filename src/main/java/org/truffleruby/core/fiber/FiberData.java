/*
 * Copyright (c) 2018 Oracle and/or its affiliates. All rights reserved. This
 * code is released under a tri EPL/GPL/LGPL license. You can use it,
 * redistribute it and/or modify it under the terms of the:
 *
 * Eclipse Public License version 1.0
 * GNU General Public License version 2
 * GNU Lesser General Public License version 2.1
 */
package org.truffleruby.core.fiber;

import java.util.concurrent.CountDownLatch;

import org.truffleruby.Layouts;
import org.truffleruby.RubyContext;
import org.truffleruby.core.array.ArrayHelpers;

import com.oracle.truffle.api.CompilerAsserts;
import com.oracle.truffle.api.object.DynamicObject;

/**
 * Needs to be a separate object than the Ruby Fiber DynamicObject,
 * as we cannot hold a strong reference to the Ruby Fiber DynamicObject.
 * This is needed so an unreferenced Fiber is automatically killed.
 */
public class FiberData {

    private final DynamicObject fiberLocals;
    private final DynamicObject catchTags;
    private final CountDownLatch initializedLatch;
    private final CountDownLatch finishedLatch;

    private volatile DynamicObject lastResumedByFiber;
    private volatile boolean alive;
    private volatile Thread thread;

    public FiberData(RubyContext context) {
        CompilerAsserts.partialEvaluationConstant(context);

        this.fiberLocals = Layouts.BASIC_OBJECT.createBasicObject(context.getCoreLibrary().getObjectFactory());
        this.catchTags = ArrayHelpers.createArray(context, null, 0);
        this.initializedLatch = new CountDownLatch(1);
        this.finishedLatch = new CountDownLatch(1);
        this.lastResumedByFiber = null;
        this.alive = true;
        this.thread = null;
    }

    public DynamicObject getFiberLocals() {
        return fiberLocals;
    }

    public DynamicObject getCatchTags() {
        return catchTags;
    }

    public CountDownLatch getInitializedLatch() {
        return initializedLatch;
    }

    public CountDownLatch getFinishedLatch() {
        return finishedLatch;
    }

    public DynamicObject getLastResumedByFiber() {
        return lastResumedByFiber;
    }

    public boolean isAlive() {
        return alive;
    }

    public Thread getThread() {
        return thread;
    }

    public void setLastResumedByFiber(DynamicObject lastResumedByFiber) {
        this.lastResumedByFiber = lastResumedByFiber;
    }

    public void setAlive(boolean alive) {
        this.alive = alive;
    }

    public void setThread(Thread thread) {
        this.thread = thread;
    }

}
