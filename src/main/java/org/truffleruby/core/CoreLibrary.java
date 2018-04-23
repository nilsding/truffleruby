/*
 * Copyright (c) 2013, 2018 Oracle and/or its affiliates. All rights reserved. This
 * code is released under a tri EPL/GPL/LGPL license. You can use it,
 * redistribute it and/or modify it under the terms of the:
 *
 * Eclipse Public License version 1.0
 * GNU General Public License version 2
 * GNU Lesser General Public License version 2.1
 */
package org.truffleruby.core;

import com.oracle.truffle.api.CallTarget;
import com.oracle.truffle.api.CompilerDirectives;
import com.oracle.truffle.api.CompilerDirectives.CompilationFinal;
import com.oracle.truffle.api.CompilerDirectives.TruffleBoundary;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.TruffleOptions;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.object.DynamicObject;
import com.oracle.truffle.api.object.DynamicObjectFactory;
import com.oracle.truffle.api.object.Layout;
import com.oracle.truffle.api.object.Property;
import com.oracle.truffle.api.source.Source;
import com.oracle.truffle.api.source.SourceSection;
import org.jcodings.specific.USASCIIEncoding;
import org.jcodings.specific.UTF8Encoding;
import org.jcodings.transcode.EConvFlags;
import org.truffleruby.Layouts;
import org.truffleruby.RubyContext;
import org.truffleruby.RubyLanguage;
import org.truffleruby.builtins.CoreMethodNodeManager;
import org.truffleruby.builtins.PrimitiveManager;
import org.truffleruby.core.klass.ClassNodes;
import org.truffleruby.core.module.ModuleNodes;
import org.truffleruby.core.rope.Rope;
import org.truffleruby.core.rope.RopeOperations;
import org.truffleruby.core.string.StringOperations;
import org.truffleruby.core.thread.ThreadBacktraceLocationLayoutImpl;
import org.truffleruby.extra.TruffleNodes;
import org.truffleruby.language.NotProvided;
import org.truffleruby.language.RubyGuards;
import org.truffleruby.language.RubyNode;
import org.truffleruby.language.RubyRootNode;
import org.truffleruby.language.backtrace.BacktraceFormatter;
import org.truffleruby.language.control.JavaException;
import org.truffleruby.language.control.RaiseException;
import org.truffleruby.language.control.TruffleFatalException;
import org.truffleruby.language.globals.GlobalVariableStorage;
import org.truffleruby.language.globals.GlobalVariables;
import org.truffleruby.language.loader.CodeLoader;
import org.truffleruby.language.loader.SourceLoader;
import org.truffleruby.language.methods.DeclarationContext;
import org.truffleruby.language.methods.InternalMethod;
import org.truffleruby.language.methods.SharedMethodInfo;
import org.truffleruby.language.objects.SingletonClassNode;
import org.truffleruby.language.objects.SingletonClassNodeGen;
import org.truffleruby.parser.ParserContext;
import org.truffleruby.parser.TranslatorDriver;
import org.truffleruby.platform.NativeConfiguration;
import org.truffleruby.platform.NativeTypes;
import org.truffleruby.platform.Platform;
import org.truffleruby.shared.BuildInformationImpl;
import org.truffleruby.shared.TruffleRuby;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

public class CoreLibrary {

    private static final String ERRNO_CONFIG_PREFIX = NativeConfiguration.PREFIX + "errno.";

    private static final Property ALWAYS_FROZEN_PROPERTY = Property.create(Layouts.FROZEN_IDENTIFIER, Layout.createLayout().createAllocator().constantLocation(true), 0);

    private final RubyContext context;

    private final SourceSection sourceSection = initCoreSourceSection();

    private final DynamicObject argumentErrorClass;
    private final DynamicObject arrayClass;
    private final DynamicObjectFactory arrayFactory;
    private final DynamicObject basicObjectClass;
    private final DynamicObject bignumClass;
    private final DynamicObjectFactory bignumFactory;
    private final DynamicObject bindingClass;
    private final DynamicObjectFactory bindingFactory;
    private final DynamicObject classClass;
    private final DynamicObject complexClass;
    private final DynamicObject dirClass;
    private final DynamicObject encodingClass;
    private final DynamicObjectFactory encodingFactory;
    private final DynamicObject encodingConverterClass;
    private final DynamicObject encodingErrorClass;
    private final DynamicObject exceptionClass;
    private final DynamicObject falseClass;
    private final DynamicObject fiberClass;
    private final DynamicObjectFactory fiberFactory;
    private final DynamicObject fixnumClass;
    private final DynamicObject floatClass;
    private final DynamicObject floatDomainErrorClass;
    private final DynamicObject hashClass;
    private final DynamicObjectFactory hashFactory;
    private final DynamicObject integerClass;
    private final DynamicObject indexErrorClass;
    private final DynamicObject ioErrorClass;
    private final DynamicObject loadErrorClass;
    private final DynamicObject localJumpErrorClass;
    private final DynamicObject matchDataClass;
    private final DynamicObjectFactory matchDataFactory;
    private final DynamicObject moduleClass;
    private final DynamicObject nameErrorClass;
    private final DynamicObjectFactory nameErrorFactory;
    private final DynamicObject nilClass;
    private final DynamicObject noMemoryErrorClass;
    private final DynamicObject noMethodErrorClass;
    private final DynamicObjectFactory noMethodErrorFactory;
    private final DynamicObject notImplementedErrorClass;
    private final DynamicObject numericClass;
    private final DynamicObject objectClass;
    private final DynamicObjectFactory objectFactory;
    private final DynamicObject procClass;
    private final DynamicObjectFactory procFactory;
    private final DynamicObject processModule;
    private final DynamicObject rangeClass;
    private final DynamicObjectFactory intRangeFactory;
    private final DynamicObjectFactory longRangeFactory;
    private final DynamicObject rangeErrorClass;
    private final DynamicObject rationalClass;
    private final DynamicObject regexpClass;
    private final DynamicObjectFactory regexpFactory;
    private final DynamicObject regexpErrorClass;
    private final DynamicObject graalErrorClass;
    private final DynamicObject runtimeErrorClass;
    private final DynamicObject systemStackErrorClass;
    private final DynamicObject securityErrorClass;
    private final DynamicObject standardErrorClass;
    private final DynamicObject stringClass;
    private final DynamicObjectFactory stringFactory;
    private final DynamicObject symbolClass;
    private final DynamicObjectFactory symbolFactory;
    private final DynamicObject syntaxErrorClass;
    private final DynamicObject systemCallErrorClass;
    private final DynamicObject systemExitClass;
    private final DynamicObject threadClass;
    private final DynamicObjectFactory threadFactory;
    private final DynamicObject threadBacktraceClass;
    private final DynamicObject threadBacktraceLocationClass;
    private final DynamicObjectFactory threadBacktraceLocationFactory;
    private final DynamicObject timeClass;
    private final DynamicObjectFactory timeFactory;
    private final DynamicObject trueClass;
    private final DynamicObject typeErrorClass;
    private final DynamicObject zeroDivisionErrorClass;
    private final DynamicObject enumerableModule;
    private final DynamicObject errnoModule;
    private final DynamicObject kernelModule;
    private final DynamicObject truffleFFIModule;
    private final DynamicObject truffleFFIPointerClass;
    private final DynamicObject truffleFFINullPointerErrorClass;
    private final DynamicObject truffleTypeModule;
    private final DynamicObject truffleModule;
    private final DynamicObject truffleBootModule;
    private final DynamicObject truffleExceptionOperationsModule;
    private final DynamicObject truffleInteropModule;
    private final DynamicObject truffleInteropForeignClass;
    private final DynamicObject truffleInteropJavaModule;
    private final DynamicObject truffleKernelOperationsModule;
    private final DynamicObject truffleRegexpOperationsModule;
    private final DynamicObject bigDecimalClass;
    private final DynamicObject encodingCompatibilityErrorClass;
    private final DynamicObject encodingUndefinedConversionErrorClass;
    private final DynamicObject methodClass;
    private final DynamicObjectFactory methodFactory;
    private final DynamicObject unboundMethodClass;
    private final DynamicObjectFactory unboundMethodFactory;
    private final DynamicObject byteArrayClass;
    private final DynamicObjectFactory byteArrayFactory;
    private final DynamicObject fiberErrorClass;
    private final DynamicObject threadErrorClass;
    private final DynamicObject weakRefClass;
    private final DynamicObjectFactory weakRefFactory;
    private final DynamicObject objectSpaceModule;
    private final DynamicObject randomizerClass;
    private final DynamicObjectFactory randomizerFactory;
    private final DynamicObject atomicReferenceClass;
    private final DynamicObject handleClass;
    private final DynamicObjectFactory handleFactory;
    private final DynamicObject ioClass;

    private final DynamicObject argv;
    private final GlobalVariables globalVariables;
    private final DynamicObject mainObject;
    private final DynamicObject nil;
    private final Object supportUndefined;
    private final DynamicObject digestClass;
    private final DynamicObjectFactory digestFactory;

    private final FrameDescriptor emptyDescriptor;

    @CompilationFinal private DynamicObject eagainWaitReadable;
    @CompilationFinal private DynamicObject eagainWaitWritable;

    private final Map<String, DynamicObject> errnoClasses = new HashMap<>();
    private final Map<Integer, String> errnoValueToNames = new HashMap<>();

    @CompilationFinal private boolean cloningEnabled;
    @CompilationFinal private InternalMethod basicObjectSendMethod;
    @CompilationFinal private InternalMethod truffleBootMainMethod;

    @CompilationFinal private GlobalVariableStorage loadPathStorage;
    @CompilationFinal private GlobalVariableStorage loadedFeaturesStorage;
    @CompilationFinal private GlobalVariableStorage debugStorage;
    @CompilationFinal private GlobalVariableStorage verboseStorage;
    @CompilationFinal private GlobalVariableStorage stderrStorage;

    private final String coreLoadPath;

    @TruffleBoundary
    private static SourceSection initCoreSourceSection() {
        final Source source = Source.newBuilder("").name("(core)").mimeType(RubyLanguage.MIME_TYPE).build();
        return source.createUnavailableSection();
    }

    private String buildCoreLoadPath() {
        String path = context.getOptions().CORE_LOAD_PATH;

        while (path.endsWith("/")) {
            path = path.substring(0, path.length() - 1);
        }

        if (path.startsWith(SourceLoader.RESOURCE_SCHEME)) {
            return path;
        }

        try {
            return new File(path).getCanonicalPath();
        } catch (IOException e) {
            throw new JavaException(e);
        }
    }

    private enum State {
        INITIALIZING,
        LOADING_RUBY_CORE,
        LOADED
    }

    private State state = State.INITIALIZING;

    private static class CoreLibraryNode extends RubyNode {

        @Child SingletonClassNode singletonClassNode;

        public CoreLibraryNode() {
            this.singletonClassNode = SingletonClassNodeGen.create(null);
            adoptChildren();
        }

        public SingletonClassNode getSingletonClassNode() {
            return singletonClassNode;
        }

        public DynamicObject getSingletonClass(Object object) {
            return singletonClassNode.executeSingletonClass(object);
        }

        @Override
        public Object execute(VirtualFrame frame) {
            return nil();
        }

    }

    private final CoreLibraryNode node;

    public CoreLibrary(RubyContext context) {
        this.context = context;
        this.coreLoadPath = buildCoreLoadPath();
        this.node = new CoreLibraryNode();

        // Nothing in this constructor can use RubyContext.getCoreLibrary() as we are building it!
        // Therefore, only initialize the core classes and modules here.

        // Create the cyclic classes and modules

        classClass = ClassNodes.createClassClass(context, null);

        basicObjectClass = ClassNodes.createBootClass(context, null, classClass, null, "BasicObject");
        Layouts.CLASS.setInstanceFactoryUnsafe(basicObjectClass, Layouts.BASIC_OBJECT.createBasicObjectShape(basicObjectClass, basicObjectClass));

        objectClass = ClassNodes.createBootClass(context, null, classClass, basicObjectClass, "Object");
        objectFactory = Layouts.BASIC_OBJECT.createBasicObjectShape(objectClass, objectClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(objectClass, objectFactory);

        moduleClass = ClassNodes.createBootClass(context, null, classClass, objectClass, "Module");
        Layouts.CLASS.setInstanceFactoryUnsafe(moduleClass, Layouts.MODULE.createModuleShape(moduleClass, moduleClass));

        // Close the cycles
        // Set superclass of Class to Module
        Layouts.MODULE.getFields(classClass).parentModule = Layouts.MODULE.getFields(moduleClass).start;
        Layouts.CLASS.setSuperclass(classClass, moduleClass);
        Layouts.MODULE.getFields(classClass).newHierarchyVersion();

        // Set constants in Object and lexical parents
        Layouts.MODULE.getFields(classClass).getAdoptedByLexicalParent(context, objectClass, "Class", node);
        Layouts.MODULE.getFields(basicObjectClass).getAdoptedByLexicalParent(context, objectClass, "BasicObject", node);
        Layouts.MODULE.getFields(objectClass).getAdoptedByLexicalParent(context, objectClass, "Object", node);
        Layouts.MODULE.getFields(moduleClass).getAdoptedByLexicalParent(context, objectClass, "Module", node);

        // Create Exception classes

        // Exception
        exceptionClass = defineClass("Exception");
        Layouts.CLASS.setInstanceFactoryUnsafe(exceptionClass, Layouts.EXCEPTION.createExceptionShape(exceptionClass, exceptionClass));

        // NoMemoryError
        noMemoryErrorClass = defineClass(exceptionClass, "NoMemoryError");

        // StandardError
        standardErrorClass = defineClass(exceptionClass, "StandardError");
        argumentErrorClass = defineClass(standardErrorClass, "ArgumentError");
        encodingErrorClass = defineClass(standardErrorClass, "EncodingError");
        fiberErrorClass = defineClass(standardErrorClass, "FiberError");
        ioErrorClass = defineClass(standardErrorClass, "IOError");
        localJumpErrorClass = defineClass(standardErrorClass, "LocalJumpError");
        regexpErrorClass = defineClass(standardErrorClass, "RegexpError");
        runtimeErrorClass = defineClass(standardErrorClass, "RuntimeError");
        threadErrorClass = defineClass(standardErrorClass, "ThreadError");
        typeErrorClass = defineClass(standardErrorClass, "TypeError");
        zeroDivisionErrorClass = defineClass(standardErrorClass, "ZeroDivisionError");

        // StandardError > RangeError
        rangeErrorClass = defineClass(standardErrorClass, "RangeError");
        floatDomainErrorClass = defineClass(rangeErrorClass, "FloatDomainError");

        // StandardError > IndexError
        indexErrorClass = defineClass(standardErrorClass, "IndexError");
        defineClass(indexErrorClass, "KeyError");

        // StandardError > IOError
        defineClass(ioErrorClass, "EOFError");

        // StandardError > NameError
        nameErrorClass = defineClass(standardErrorClass, "NameError");
        nameErrorFactory = Layouts.NAME_ERROR.createNameErrorShape(nameErrorClass, nameErrorClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(nameErrorClass, nameErrorFactory);
        noMethodErrorClass = defineClass(nameErrorClass, "NoMethodError");
        noMethodErrorFactory = Layouts.NO_METHOD_ERROR.createNoMethodErrorShape(noMethodErrorClass, noMethodErrorClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(noMethodErrorClass, noMethodErrorFactory);

        // StandardError > SystemCallError
        systemCallErrorClass = defineClass(standardErrorClass, "SystemCallError");
        Layouts.CLASS.setInstanceFactoryUnsafe(systemCallErrorClass, Layouts.SYSTEM_CALL_ERROR.createSystemCallErrorShape(systemCallErrorClass, systemCallErrorClass));

        errnoModule = defineModule("Errno");

        // ScriptError
        DynamicObject scriptErrorClass = defineClass(exceptionClass, "ScriptError");
        loadErrorClass = defineClass(scriptErrorClass, "LoadError");
        notImplementedErrorClass = defineClass(scriptErrorClass, "NotImplementedError");
        syntaxErrorClass = defineClass(scriptErrorClass, "SyntaxError");

        // SecurityError
        securityErrorClass = defineClass(exceptionClass, "SecurityError");

        // SignalException
        DynamicObject signalExceptionClass = defineClass(exceptionClass, "SignalException");
        defineClass(signalExceptionClass, "Interrupt");

        // SystemExit
        systemExitClass = defineClass(exceptionClass, "SystemExit");

        // SystemStackError
        systemStackErrorClass = defineClass(exceptionClass, "SystemStackError");

        // Create core classes and modules

        numericClass = defineClass("Numeric");
        complexClass = defineClass(numericClass, "Complex");
        floatClass = defineClass(numericClass, "Float");
        integerClass = defineClass(numericClass, "Integer");
        fixnumClass = defineClass(integerClass, "Fixnum");
        bignumClass = defineClass(integerClass, "Bignum");
        bignumFactory = alwaysFrozen(Layouts.BIGNUM.createBignumShape(bignumClass, bignumClass));
        Layouts.CLASS.setInstanceFactoryUnsafe(bignumClass, bignumFactory);
        rationalClass = defineClass(numericClass, "Rational");

        // Classes defined in Object

        arrayClass = defineClass("Array");
        arrayFactory = Layouts.ARRAY.createArrayShape(arrayClass, arrayClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(arrayClass, arrayFactory);
        bindingClass = defineClass("Binding");
        bindingFactory = Layouts.BINDING.createBindingShape(bindingClass, bindingClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(bindingClass, bindingFactory);
        defineClass("Data"); // Needed by Socket::Ifaddr and defined in core MRI
        dirClass = defineClass("Dir");
        encodingClass = defineClass("Encoding");
        encodingFactory = Layouts.ENCODING.createEncodingShape(encodingClass, encodingClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(encodingClass, encodingFactory);
        falseClass = defineClass("FalseClass");
        fiberClass = defineClass("Fiber");
        fiberFactory = Layouts.FIBER.createFiberShape(fiberClass, fiberClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(fiberClass, fiberFactory);
        defineModule("FileTest");
        hashClass = defineClass("Hash");
        final DynamicObjectFactory originalHashFactory = Layouts.HASH.createHashShape(hashClass, hashClass);
        if (context.isPreInitializing()) {
            hashFactory = context.getPreInitializationManager().hookIntoHashFactory(originalHashFactory);
        } else {
            hashFactory = originalHashFactory;
        }
        Layouts.CLASS.setInstanceFactoryUnsafe(hashClass, hashFactory);
        matchDataClass = defineClass("MatchData");
        matchDataFactory = Layouts.MATCH_DATA.createMatchDataShape(matchDataClass, matchDataClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(matchDataClass, matchDataFactory);
        methodClass = defineClass("Method");
        methodFactory = Layouts.METHOD.createMethodShape(methodClass, methodClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(methodClass, methodFactory);
        final DynamicObject mutexClass = defineClass("Mutex");
        Layouts.CLASS.setInstanceFactoryUnsafe(mutexClass, Layouts.MUTEX.createMutexShape(mutexClass, mutexClass));
        nilClass = defineClass("NilClass");
        final DynamicObjectFactory nilFactory = alwaysShared(alwaysFrozen(Layouts.CLASS.getInstanceFactory(nilClass)));
        Layouts.CLASS.setInstanceFactoryUnsafe(nilClass, nilFactory);
        procClass = defineClass("Proc");
        procFactory = Layouts.PROC.createProcShape(procClass, procClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(procClass, procFactory);
        processModule = defineModule("Process");
        DynamicObject queueClass = defineClass("Queue");
        Layouts.CLASS.setInstanceFactoryUnsafe(queueClass, Layouts.QUEUE.createQueueShape(queueClass, queueClass));
        DynamicObject sizedQueueClass = defineClass(queueClass, "SizedQueue");
        Layouts.CLASS.setInstanceFactoryUnsafe(sizedQueueClass, Layouts.SIZED_QUEUE.createSizedQueueShape(sizedQueueClass, sizedQueueClass));
        rangeClass = defineClass("Range");
        Layouts.CLASS.setInstanceFactoryUnsafe(rangeClass, Layouts.OBJECT_RANGE.createObjectRangeShape(rangeClass, rangeClass));
        intRangeFactory = Layouts.INT_RANGE.createIntRangeShape(rangeClass, rangeClass);
        longRangeFactory = Layouts.LONG_RANGE.createLongRangeShape(rangeClass, rangeClass);
        regexpClass = defineClass("Regexp");
        regexpFactory = Layouts.REGEXP.createRegexpShape(regexpClass, regexpClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(regexpClass, regexpFactory);
        stringClass = defineClass("String");
        stringFactory = Layouts.STRING.createStringShape(stringClass, stringClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(stringClass, stringFactory);
        symbolClass = defineClass("Symbol");
        symbolFactory = alwaysShared(alwaysFrozen(Layouts.SYMBOL.createSymbolShape(symbolClass, symbolClass)));
        Layouts.CLASS.setInstanceFactoryUnsafe(symbolClass, symbolFactory);

        threadClass = defineClass("Thread");
        threadClass.define("@abort_on_exception", false);
        threadFactory = Layouts.THREAD.createThreadShape(threadClass, threadClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(threadClass, threadFactory);

        threadBacktraceClass = defineClass(threadClass, objectClass, "Backtrace");
        threadBacktraceLocationClass = defineClass(threadBacktraceClass, objectClass, "Location");
        threadBacktraceLocationFactory = ThreadBacktraceLocationLayoutImpl.INSTANCE.createThreadBacktraceLocationShape(threadBacktraceLocationClass, threadBacktraceLocationClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(threadBacktraceLocationClass, threadBacktraceLocationFactory);
        timeClass = defineClass("Time");
        timeFactory = Layouts.TIME.createTimeShape(timeClass, timeClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(timeClass, timeFactory);
        trueClass = defineClass("TrueClass");
        unboundMethodClass = defineClass("UnboundMethod");
        unboundMethodFactory = Layouts.UNBOUND_METHOD.createUnboundMethodShape(unboundMethodClass, unboundMethodClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(unboundMethodClass, unboundMethodFactory);
        ioClass = defineClass("IO");
        Layouts.CLASS.setInstanceFactoryUnsafe(ioClass, Layouts.IO.createIOShape(ioClass, ioClass));
        defineClass(ioClass, "File");

        weakRefClass = defineClass(basicObjectClass, "WeakRef");
        weakRefFactory = Layouts.WEAK_REF_LAYOUT.createWeakRefShape(weakRefClass, weakRefClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(weakRefClass, weakRefFactory);
        final DynamicObject tracePointClass = defineClass("TracePoint");
        Layouts.CLASS.setInstanceFactoryUnsafe(tracePointClass, Layouts.TRACE_POINT.createTracePointShape(tracePointClass, tracePointClass));

        // Modules

        DynamicObject comparableModule = defineModule("Comparable");
        defineModule("Config");
        enumerableModule = defineModule("Enumerable");
        defineModule("GC");
        kernelModule = defineModule("Kernel");
        defineModule("Math");
        objectSpaceModule = defineModule("ObjectSpace");

        // The rest

        encodingCompatibilityErrorClass = defineClass(encodingClass, encodingErrorClass, "CompatibilityError");
        encodingUndefinedConversionErrorClass = defineClass(encodingClass, encodingErrorClass, "UndefinedConversionError");

        encodingConverterClass = defineClass(encodingClass, objectClass, "Converter");
        Layouts.CLASS.setInstanceFactoryUnsafe(encodingConverterClass, Layouts.ENCODING_CONVERTER.createEncodingConverterShape(encodingConverterClass, encodingConverterClass));

        truffleModule = defineModule("Truffle");
        graalErrorClass = defineClass(truffleModule, exceptionClass, "GraalError");
        truffleExceptionOperationsModule = defineModule(truffleModule, "ExceptionOperations");
        truffleInteropModule = defineModule(truffleModule, "Interop");
        truffleInteropForeignClass = defineClass(truffleInteropModule, objectClass, "Foreign");
        truffleInteropJavaModule = defineModule(truffleInteropModule, "Java");
        defineModule(truffleModule, "CExt");
        defineModule(truffleModule, "Debug");
        defineModule(truffleModule, "Digest");
        defineModule(truffleModule, "ObjSpace");
        defineModule(truffleModule, "Etc");
        defineModule(truffleModule, "Coverage");
        defineModule(truffleModule, "Graal");
        defineModule(truffleModule, "Ropes");
        defineModule(truffleModule, "GC");
        defineModule(truffleModule, "Array");
        truffleRegexpOperationsModule = defineModule(truffleModule, "RegexpOperations");
        defineModule(truffleModule, "StringOperations");
        truffleBootModule = defineModule(truffleModule, "Boot");
        defineModule(truffleModule, "FixnumOperations");
        defineModule(truffleModule, "System");
        truffleKernelOperationsModule = defineModule(truffleModule, "KernelOperations");
        defineModule(truffleModule, "Process");
        defineModule(truffleModule, "Binding");
        defineModule(truffleModule, "POSIX");
        defineModule(truffleModule, "Readline");
        defineModule(truffleModule, "ReadlineHistory");
        defineModule(truffleModule, "ThreadOperations");
        handleClass = defineClass(truffleModule, objectClass, "Handle");
        handleFactory = Layouts.HANDLE.createHandleShape(handleClass, handleClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(handleClass, handleFactory);
        defineModule("Polyglot");

        bigDecimalClass = defineClass(truffleModule, numericClass, "BigDecimal");
        Layouts.CLASS.setInstanceFactoryUnsafe(bigDecimalClass, Layouts.BIG_DECIMAL.createBigDecimalShape(bigDecimalClass, bigDecimalClass));

        final DynamicObject gem = defineModule(truffleModule, "Gem");
        defineModule(gem, "BCrypt");

        truffleFFIModule = defineModule(truffleModule, "FFI");
        truffleFFIPointerClass = defineClass(truffleFFIModule, objectClass, "Pointer");
        Layouts.CLASS.setInstanceFactoryUnsafe(truffleFFIPointerClass, Layouts.POINTER.createPointerShape(truffleFFIPointerClass, truffleFFIPointerClass));
        truffleFFINullPointerErrorClass = defineClass(truffleFFIModule, runtimeErrorClass, "NullPointerError");

        truffleTypeModule = defineModule(truffleModule, "Type");

        byteArrayClass = defineClass(truffleModule, objectClass, "ByteArray");
        byteArrayFactory = Layouts.BYTE_ARRAY.createByteArrayShape(byteArrayClass, byteArrayClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(byteArrayClass, byteArrayFactory);
        defineClass(truffleModule, objectClass, "StringData");
        defineClass(encodingClass, objectClass, "Transcoding");
        randomizerClass = defineClass(truffleModule, objectClass, "Randomizer");
        atomicReferenceClass = defineClass(truffleModule, objectClass, "AtomicReference");
        Layouts.CLASS.setInstanceFactoryUnsafe(atomicReferenceClass,
                Layouts.ATOMIC_REFERENCE.createAtomicReferenceShape(atomicReferenceClass, atomicReferenceClass));
        randomizerFactory = Layouts.RANDOMIZER.createRandomizerShape(randomizerClass, randomizerClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(randomizerClass, randomizerFactory);

        // Standard library

        digestClass = defineClass(truffleModule, basicObjectClass, "Digest");
        digestFactory = Layouts.DIGEST.createDigestShape(digestClass, digestClass);
        Layouts.CLASS.setInstanceFactoryUnsafe(digestClass, digestFactory);

        // Include the core modules

        includeModules(comparableModule);

        // Create some key objects

        mainObject = objectFactory.newInstance();
        nil = nilFactory.newInstance();
        emptyDescriptor = new FrameDescriptor(nil);
        argv = Layouts.ARRAY.createArray(arrayFactory, null, 0);
        supportUndefined = NotProvided.INSTANCE;

        globalVariables = new GlobalVariables(nil);

        // No need for new version since it's null before which is not cached
        assert Layouts.CLASS.getSuperclass(basicObjectClass) == null;
        Layouts.CLASS.setSuperclass(basicObjectClass, nil);
    }

    private static DynamicObjectFactory alwaysFrozen(DynamicObjectFactory factory) {
        return factory.getShape().addProperty(ALWAYS_FROZEN_PROPERTY).createFactory();
    }

    private static DynamicObjectFactory alwaysShared(DynamicObjectFactory factory) {
        return factory.getShape().makeSharedShape().createFactory();
    }

    private void includeModules(DynamicObject comparableModule) {
        assert RubyGuards.isRubyModule(comparableModule);

        Layouts.MODULE.getFields(objectClass).include(context, node, kernelModule);

        Layouts.MODULE.getFields(numericClass).include(context, node, comparableModule);
        Layouts.MODULE.getFields(symbolClass).include(context, node, comparableModule);

        Layouts.MODULE.getFields(arrayClass).include(context, node, enumerableModule);
        Layouts.MODULE.getFields(dirClass).include(context, node, enumerableModule);
        Layouts.MODULE.getFields(hashClass).include(context, node, enumerableModule);
        Layouts.MODULE.getFields(rangeClass).include(context, node, enumerableModule);
    }

    public void initialize() {
        initializeConstants();
    }

    public void loadCoreNodes(PrimitiveManager primitiveManager) {
        final CoreMethodNodeManager coreMethodNodeManager = new CoreMethodNodeManager(context, node.getSingletonClassNode(), primitiveManager);

        coreMethodNodeManager.loadCoreMethodNodes();

        basicObjectSendMethod = getMethod(basicObjectClass, "__send__");
        truffleBootMainMethod = getMethod(node.getSingletonClass(truffleBootModule), "main");

        final CallTarget kernelLamba = getMethod(kernelModule, "lambda").getCallTarget();
        cloningEnabled = Truffle.getRuntime().createDirectCallNode(kernelLamba).isCallTargetCloningAllowed();
    }

    private InternalMethod getMethod(DynamicObject module, String name) {
        InternalMethod method = Layouts.MODULE.getFields(module).getMethod(name);
        if (method == null || method.isUndefined()) {
            throw new AssertionError();
        }
        return method;
    }

    private Object verbosityOption() {
        switch (context.getOptions().VERBOSITY) {
            case NIL:
                return nil;
            case FALSE:
                return false;
            case TRUE:
                return true;
            default:
                throw new UnsupportedOperationException();
        }
    }

    private void findGlobalVariableStorage() {
        GlobalVariables globals = globalVariables;
        loadPathStorage = globals.getStorage("$LOAD_PATH");
        loadedFeaturesStorage = globals.getStorage("$LOADED_FEATURES");
        debugStorage = globals.getStorage("$DEBUG");
        verboseStorage = globals.getStorage("$VERBOSE");
        stderrStorage = globals.getStorage("$stderr");
    }

    private void initializeConstants() {
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_CHAR", NativeTypes.TYPE_CHAR);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_UCHAR", NativeTypes.TYPE_UCHAR);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_BOOL", NativeTypes.TYPE_BOOL);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_SHORT", NativeTypes.TYPE_SHORT);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_USHORT", NativeTypes.TYPE_USHORT);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_INT", NativeTypes.TYPE_INT);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_UINT", NativeTypes.TYPE_UINT);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_LONG", NativeTypes.TYPE_LONG);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_ULONG", NativeTypes.TYPE_ULONG);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_LL", NativeTypes.TYPE_LL);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_ULL", NativeTypes.TYPE_ULL);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_FLOAT", NativeTypes.TYPE_FLOAT);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_DOUBLE", NativeTypes.TYPE_DOUBLE);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_PTR", NativeTypes.TYPE_PTR);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_VOID", NativeTypes.TYPE_VOID);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_STRING", NativeTypes.TYPE_STRING);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_STRPTR", NativeTypes.TYPE_STRPTR);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_CHARARR", NativeTypes.TYPE_CHARARR);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_ENUM", NativeTypes.TYPE_ENUM);
        Layouts.MODULE.getFields(truffleFFIModule).setConstant(context, node, "TYPE_VARARGS", NativeTypes.TYPE_VARARGS);

        Layouts.MODULE.getFields(objectClass).setConstant(context, node, "RUBY_VERSION", frozenUSASCIIString(TruffleRuby.LANGUAGE_VERSION));
        Layouts.MODULE.getFields(truffleModule).setConstant(context, node, "RUBY_BASE_VERSION", frozenUSASCIIString(
                TruffleRuby.LANGUAGE_BASE_VERSION));
        Layouts.MODULE.getFields(objectClass).setConstant(context, node, "RUBY_PATCHLEVEL", 0);
        Layouts.MODULE.getFields(objectClass).setConstant(context, node, "RUBY_REVISION", TruffleRuby.LANGUAGE_REVISION);
        Layouts.MODULE.getFields(objectClass).setConstant(context, node, "RUBY_ENGINE", frozenUSASCIIString(TruffleRuby.ENGINE_ID));
        Layouts.MODULE.getFields(objectClass).setConstant(context, node, "RUBY_ENGINE_VERSION", frozenUSASCIIString(
                TruffleRuby.getEngineVersion()));
        Layouts.MODULE.getFields(objectClass).setConstant(context, node, "RUBY_PLATFORM", frozenUSASCIIString(RubyLanguage.PLATFORM));
        Layouts.MODULE.getFields(objectClass).setConstant(context, node, "RUBY_RELEASE_DATE", frozenUSASCIIString(BuildInformationImpl.INSTANCE.getCompileDate()));
        Layouts.MODULE.getFields(objectClass).setConstant(context, node, "RUBY_DESCRIPTION", frozenUSASCIIString(
                TruffleRuby.getVersionString(TruffleNodes.GraalNode.isGraal(), TruffleOptions.AOT)));
        Layouts.MODULE.getFields(objectClass).setConstant(context, node, "RUBY_COPYRIGHT", frozenUSASCIIString(
                TruffleRuby.RUBY_COPYRIGHT));

        // BasicObject knows itself
        Layouts.MODULE.getFields(basicObjectClass).setConstant(context, node, "BasicObject", basicObjectClass);

        Layouts.MODULE.getFields(objectClass).setConstant(context, node, "ARGV", argv);

        Layouts.MODULE.getFields(truffleModule).setConstant(context, node, "UNDEFINED", supportUndefined);
        Layouts.MODULE.getFields(truffleModule).setConstant(context, node, "LIBC", frozenUSASCIIString(Platform.LIBC));

        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "INVALID_MASK", EConvFlags.INVALID_MASK);
        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "INVALID_REPLACE", EConvFlags.INVALID_REPLACE);
        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "UNDEF_MASK", EConvFlags.UNDEF_MASK);
        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "UNDEF_REPLACE", EConvFlags.UNDEF_REPLACE);
        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "UNDEF_HEX_CHARREF", EConvFlags.UNDEF_HEX_CHARREF);
        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "PARTIAL_INPUT", EConvFlags.PARTIAL_INPUT);
        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "AFTER_OUTPUT", EConvFlags.AFTER_OUTPUT);
        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "UNIVERSAL_NEWLINE_DECORATOR", EConvFlags.UNIVERSAL_NEWLINE_DECORATOR);
        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "CRLF_NEWLINE_DECORATOR", EConvFlags.CRLF_NEWLINE_DECORATOR);
        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "CR_NEWLINE_DECORATOR", EConvFlags.CR_NEWLINE_DECORATOR);
        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "XML_TEXT_DECORATOR", EConvFlags.XML_TEXT_DECORATOR);
        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "XML_ATTR_CONTENT_DECORATOR", EConvFlags.XML_ATTR_CONTENT_DECORATOR);
        Layouts.MODULE.getFields(encodingConverterClass).setConstant(context, node, "XML_ATTR_QUOTE_DECORATOR", EConvFlags.XML_ATTR_QUOTE_DECORATOR);

        // Errno classes and constants
        for (Entry<String, Object> entry : context.getNativeConfiguration().getSection(ERRNO_CONFIG_PREFIX)) {
            final String name = entry.getKey().substring(ERRNO_CONFIG_PREFIX.length());
            if (name.equals("EWOULDBLOCK") && getErrnoValue("EWOULDBLOCK") == getErrnoValue("EAGAIN")) {
                continue; // Don't define it as a class, define it as constant later.
            }
            errnoValueToNames.put((int) entry.getValue(), name);
            final DynamicObject rubyClass = defineClass(errnoModule, systemCallErrorClass, name);
            Layouts.MODULE.getFields(rubyClass).setConstant(context, node, "Errno", entry.getValue());
            errnoClasses.put(name, rubyClass);
        }

        if (getErrnoValue("EWOULDBLOCK") == getErrnoValue("EAGAIN")) {
            Layouts.MODULE.getFields(errnoModule).setConstant(context, node, "EWOULDBLOCK", errnoClasses.get("EAGAIN"));
        }
    }

    private DynamicObject frozenUSASCIIString(String string) {
        final Rope rope = RopeOperations.encodeAscii(string, USASCIIEncoding.INSTANCE);
        return StringOperations.createFrozenString(context, rope);
    }

    private DynamicObject defineClass(String name) {
        return defineClass(objectClass, name);
    }

    private DynamicObject defineClass(DynamicObject superclass, String name) {
        assert RubyGuards.isRubyClass(superclass);
        return ClassNodes.createInitializedRubyClass(context, null, objectClass, superclass, name);
    }

    private DynamicObject defineClass(DynamicObject lexicalParent, DynamicObject superclass, String name) {
        assert RubyGuards.isRubyModule(lexicalParent);
        assert RubyGuards.isRubyClass(superclass);
        return ClassNodes.createInitializedRubyClass(context, null, lexicalParent, superclass, name);
    }

    private DynamicObject defineModule(String name) {
        return defineModule(null, objectClass, name);
    }

    private DynamicObject defineModule(DynamicObject lexicalParent, String name) {
        return defineModule(null, lexicalParent, name);
    }

    private DynamicObject defineModule(SourceSection sourceSection, DynamicObject lexicalParent, String name) {
        assert RubyGuards.isRubyModule(lexicalParent);
        return ModuleNodes.createModule(context, sourceSection, moduleClass, lexicalParent, name, node);
    }

    public void loadRubyCore() {
        try {
            state = State.LOADING_RUBY_CORE;

            try {
                for (int n = 0; n < CORE_FILES.length; n++) {
                    final Source source = context.getSourceLoader().load(getCoreLoadPath() + CORE_FILES[n]);

                    final RubyRootNode rootNode = context.getCodeLoader().parse(source,
                            UTF8Encoding.INSTANCE, ParserContext.TOP_LEVEL, null, true, node);

                    final CodeLoader.DeferredCall deferredCall = context.getCodeLoader().prepareExecute(
                            ParserContext.TOP_LEVEL,
                            DeclarationContext.topLevel(context),
                            rootNode,
                            null,
                            context.getCoreLibrary().getMainObject());

                    TranslatorDriver.printParseTranslateExecuteMetric("before-execute", context, source);
                    deferredCall.callWithoutCallNode();
                    TranslatorDriver.printParseTranslateExecuteMetric("after-execute", context, source);
                }
            } catch (IOException e) {
                throw new JavaException(e);
            }
        } catch (RaiseException e) {
            final DynamicObject rubyException = e.getException();
            BacktraceFormatter.createDefaultFormatter(getContext()).printBacktrace(context, rubyException, Layouts.EXCEPTION.getBacktrace(rubyException));
            throw new TruffleFatalException("couldn't load the core library", e);
        } finally {
            state = State.LOADED;
        }

        // Get some references to things defined in the Ruby core

        eagainWaitReadable = (DynamicObject) Layouts.MODULE.getFields(ioClass).getConstant("EAGAINWaitReadable").getValue();
        assert Layouts.CLASS.isClass(eagainWaitReadable);

        eagainWaitWritable = (DynamicObject) Layouts.MODULE.getFields(ioClass).getConstant("EAGAINWaitWritable").getValue();
        assert Layouts.CLASS.isClass(eagainWaitWritable);

        findGlobalVariableStorage();
    }

    public void initializePostBoot() {
        // Load code that can't be run until everything else is boostrapped, such as pre-loaded Ruby stdlib.

        try {
            final RubyRootNode rootNode = context.getCodeLoader().parse(context.getSourceLoader().load(getCoreLoadPath() + "/post-boot/post-boot.rb"),
                    UTF8Encoding.INSTANCE, ParserContext.TOP_LEVEL, null, true, node);
            final CodeLoader.DeferredCall deferredCall = context.getCodeLoader().prepareExecute(
                    ParserContext.TOP_LEVEL, DeclarationContext.topLevel(context), rootNode, null, context.getCoreLibrary().getMainObject());
            deferredCall.callWithoutCallNode();
        } catch (IOException e) {
            throw new JavaException(e);
        } catch (RaiseException e) {
            final DynamicObject rubyException = e.getException();
            BacktraceFormatter.createDefaultFormatter(getContext()).printBacktrace(context, rubyException, Layouts.EXCEPTION.getBacktrace(rubyException));
            throw new TruffleFatalException("couldn't load the post-boot code", e);
        }
    }

    @TruffleBoundary
    public DynamicObject getMetaClass(Object object) {
        if (RubyGuards.isRubyBasicObject(object)) {
            return Layouts.BASIC_OBJECT.getMetaClass((DynamicObject) object);
        } else {
            return getLogicalClass(object);
        }
    }

    @TruffleBoundary
    public DynamicObject getLogicalClass(Object object) {
        if (object instanceof DynamicObject) {
            return Layouts.BASIC_OBJECT.getLogicalClass((DynamicObject) object);
        } else if (object instanceof Boolean) {
            if ((boolean) object) {
                return trueClass;
            } else {
                return falseClass;
            }
        } else if (object instanceof Byte) {
            return fixnumClass;
        } else if (object instanceof Short) {
            return fixnumClass;
        } else if (object instanceof Integer) {
            return fixnumClass;
        } else if (object instanceof Long) {
            return fixnumClass;
        } else if (object instanceof Float) {
            return floatClass;
        } else if (object instanceof Double) {
            return floatClass;
        } else {
            return truffleInteropForeignClass;
        }
    }

    /**
     * Convert a value to a {@code Float}, without doing any lookup.
     */
    public static double toDouble(Object value, DynamicObject nil) {
        assert value != null;

        if (value == nil) {
            return 0;
        }

        if (value instanceof Integer) {
            return (int) value;
        }

        if (value instanceof Long) {
            return (long) value;
        }

        if (RubyGuards.isRubyBignum(value)) {
            return Layouts.BIGNUM.getValue((DynamicObject) value).doubleValue();
        }

        if (value instanceof Double) {
            return (double) value;
        }

        CompilerDirectives.transferToInterpreterAndInvalidate();
        throw new UnsupportedOperationException();
    }

    public static boolean fitsIntoInteger(long value) {
        return ((int) value) == value;
    }

    public static boolean fitsIntoUnsignedInteger(long value) {
        return value == (value & 0xffffffffL) || value < 0 && value >= Integer.MIN_VALUE;
    }

    public static int long2int(long value) {
        assert fitsIntoInteger(value) : value;
        return (int) value;
    }

    public RubyContext getContext() {
        return context;
    }

    public SourceSection getSourceSection() {
        return sourceSection;
    }

    public String getCoreLoadPath() {
        return coreLoadPath;
    }

    public DynamicObject getArrayClass() {
        return arrayClass;
    }

    public DynamicObject getBasicObjectClass() {
        return basicObjectClass;
    }

    public DynamicObject getBigDecimalClass() {
        return bigDecimalClass;
    }

    public DynamicObjectFactory getBindingFactory() {
        return bindingFactory;
    }

    public DynamicObject getClassClass() {
        return classClass;
    }

    public DynamicObject getFalseClass() {
        return falseClass;
    }

    public DynamicObjectFactory getFiberFactory() {
        return fiberFactory;
    }

    public DynamicObject getFixnumClass() {
        return fixnumClass;
    }

    public DynamicObject getFloatClass() {
        return floatClass;
    }

    public DynamicObject getStandardErrorClass() {
        return standardErrorClass;
    }

    public DynamicObject getLoadErrorClass() {
        return loadErrorClass;
    }

    public DynamicObject getMatchDataClass() {
        return matchDataClass;
    }

    public DynamicObjectFactory getMatchDataFactory() {
        return matchDataFactory;
    }

    public DynamicObject getModuleClass() {
        return moduleClass;
    }

    public DynamicObject getNameErrorClass() {
        return nameErrorClass;
    }

    public DynamicObjectFactory getNameErrorFactory() {
        return nameErrorFactory;
    }

    public DynamicObject getNilClass() {
        return nilClass;
    }

    public FrameDescriptor getEmptyDescriptor() {
        return emptyDescriptor;
    }

    public DynamicObject getNoMemoryErrorClass() {
        return noMemoryErrorClass;
    }

    public DynamicObject getNoMethodErrorClass() {
        return noMethodErrorClass;
    }

    public DynamicObjectFactory getNoMethodErrorFactory() {
        return noMethodErrorFactory;
    }

    public DynamicObject getObjectClass() {
        return objectClass;
    }

    public DynamicObjectFactory getObjectFactory() {
        return objectFactory;
    }

    public DynamicObject getProcClass() {
        return procClass;
    }

    public DynamicObject getProcessModule() {
        return processModule;
    }

    public DynamicObject getRangeClass() {
        return rangeClass;
    }

    public DynamicObject getRationalClass() {
        return rationalClass;
    }

    public DynamicObjectFactory getRegexpFactory() {
        return regexpFactory;
    }

    public DynamicObject getTruffleFFIPointerClass() {
        return truffleFFIPointerClass;
    }

    public DynamicObject getTruffleFFINullPointerErrorClass() {
        return truffleFFINullPointerErrorClass;
    }

    public DynamicObject getTruffleTypeModule() {
        return truffleTypeModule;
    }

    public DynamicObject getGraalErrorClass() {
        return graalErrorClass;
    }

    public DynamicObject getStringClass() {
        return stringClass;
    }

    public DynamicObject getThreadClass() {
        return threadClass;
    }

    public DynamicObjectFactory getThreadFactory() {
        return threadFactory;
    }

    public DynamicObject getTypeErrorClass() {
        return typeErrorClass;
    }

    public DynamicObject getTrueClass() {
        return trueClass;
    }

    public DynamicObject getZeroDivisionErrorClass() {
        return zeroDivisionErrorClass;
    }

    public DynamicObject getKernelModule() {
        return kernelModule;
    }

    public GlobalVariables getGlobalVariables() {
        return globalVariables;
    }

    public DynamicObject getLoadPath() {
        return (DynamicObject) loadPathStorage.getValue();
    }

    public DynamicObject getLoadedFeatures() {
        return (DynamicObject) loadedFeaturesStorage.getValue();
    }

    public Object getDebug() {
        if (debugStorage != null) {
            return debugStorage.getValue();
        } else {
            return context.getOptions().DEBUG;
        }
    }

    private Object verbosity() {
        if (verboseStorage != null) {
            return verboseStorage.getValue();
        } else {
            return verbosityOption();
        }
    }
    public boolean warningsEnabled() {
        return verbosity() != nil;
    }

    public boolean isVerbose() {
        return verbosity() == Boolean.TRUE;
    }

    public Object getStderr() {
        return stderrStorage.getValue();
    }

    public DynamicObject getMainObject() {
        return mainObject;
    }

    public DynamicObject getNil() {
        return nil;
    }

    public DynamicObject getENV() {
        return (DynamicObject) Layouts.MODULE.getFields(objectClass).getConstant("ENV").getValue();
    }

    public DynamicObject getNumericClass() {
        return numericClass;
    }

    public DynamicObjectFactory getUnboundMethodFactory() {
        return unboundMethodFactory;
    }

    public DynamicObjectFactory getMethodFactory() {
        return methodFactory;
    }

    public DynamicObject getComplexClass() {
        return complexClass;
    }

    public DynamicObjectFactory getByteArrayFactory() {
        return byteArrayFactory;
    }

    @TruffleBoundary
    public int getErrnoValue(String errnoName) {
        return (int) context.getNativeConfiguration().get(ERRNO_CONFIG_PREFIX + errnoName);
    }

    @TruffleBoundary
    public String getErrnoName(int errnoValue) {
        return errnoValueToNames.get(errnoValue);
    }

    @TruffleBoundary
    public DynamicObject getErrnoClass(String name) {
        return errnoClasses.get(name);
    }

    public DynamicObject getSymbolClass() {
        return symbolClass;
    }

    public DynamicObjectFactory getSymbolFactory() {
        return symbolFactory;
    }

    public DynamicObjectFactory getThreadBacktraceLocationFactory() {
        return threadBacktraceLocationFactory;
    }

    public boolean isInitializing() {
        return state == State.INITIALIZING;
    }

    public boolean isLoadingRubyCore() {
        return state == State.LOADING_RUBY_CORE;
    }

    public boolean isLoaded() {
        return state == State.LOADED;
    }

    public boolean isSend(InternalMethod method) {
        CallTarget callTarget = method.getCallTarget();
        return callTarget == basicObjectSendMethod.getCallTarget();
    }

    public boolean isTruffleBootMainMethod(SharedMethodInfo info) {
        return info == truffleBootMainMethod.getSharedMethodInfo();
    }

    public boolean isCloningEnabled() {
        return cloningEnabled;
    }

    public DynamicObjectFactory getIntRangeFactory() {
        return intRangeFactory;
    }

    public DynamicObjectFactory getLongRangeFactory() {
        return longRangeFactory;
    }

    public DynamicObjectFactory getDigestFactory() {
        return digestFactory;
    }

    public DynamicObjectFactory getArrayFactory() {
        return arrayFactory;
    }

    public DynamicObjectFactory getBignumFactory() {
        return bignumFactory;
    }

    public DynamicObjectFactory getProcFactory() {
        return procFactory;
    }

    public DynamicObjectFactory getStringFactory() {
        return stringFactory;
    }

    public DynamicObject getHashClass() {
        return hashClass;
    }

    public DynamicObjectFactory getHashFactory() {
        return hashFactory;
    }

    public DynamicObjectFactory getWeakRefFactory() {
        return weakRefFactory;
    }

    public Object getObjectSpaceModule() {
        return objectSpaceModule;
    }

    public DynamicObjectFactory getRandomizerFactory() {
        return randomizerFactory;
    }

    public DynamicObject getSystemExitClass() {
        return systemExitClass;
    }

    public DynamicObjectFactory getHandleFactory() {
        return handleFactory;
    }

    public DynamicObject getRuntimeErrorClass() {
        return runtimeErrorClass;
    }

    public DynamicObject getSystemStackErrorClass() {
        return systemStackErrorClass;
    }

    public DynamicObject getArgumentErrorClass() {
        return argumentErrorClass;
    }

    public DynamicObject getIndexErrorClass() {
        return indexErrorClass;
    }

    public DynamicObject getLocalJumpErrorClass() {
        return localJumpErrorClass;
    }

    public DynamicObject getNotImplementedErrorClass() {
        return notImplementedErrorClass;
    }

    public DynamicObject getSyntaxErrorClass() {
        return syntaxErrorClass;
    }

    public DynamicObject getFloatDomainErrorClass() {
        return floatDomainErrorClass;
    }

    public DynamicObject getIOErrorClass() {
        return ioErrorClass;
    }

    public DynamicObject getRangeErrorClass() {
        return rangeErrorClass;
    }

    public DynamicObject getRegexpErrorClass() {
        return regexpErrorClass;
    }

    public DynamicObject getEncodingClass() {
        return encodingClass;
    }

    public DynamicObjectFactory getEncodingFactory() {
        return encodingFactory;
    }

    public DynamicObject getEncodingErrorClass() {
        return encodingErrorClass;
    }

    public DynamicObject getEncodingCompatibilityErrorClass() {
        return encodingCompatibilityErrorClass;
    }

    public DynamicObject getEncodingUndefinedConversionErrorClass() {
        return encodingUndefinedConversionErrorClass;
    }

    public DynamicObject getFiberErrorClass() {
        return fiberErrorClass;
    }

    public DynamicObject getThreadErrorClass() {
        return threadErrorClass;
    }

    public DynamicObject getSecurityErrorClass() {
        return securityErrorClass;
    }

    public DynamicObject getSystemCallErrorClass() {
        return systemCallErrorClass;
    }

    public DynamicObject getEagainWaitReadable() {
        return eagainWaitReadable;
    }

    public DynamicObject getEagainWaitWritable() {
        return eagainWaitWritable;
    }

    public DynamicObject getTruffleModule() {
        return truffleModule;
    }

    public DynamicObject getTruffleBootModule() {
        return truffleBootModule;
    }

    public DynamicObject getTruffleExceptionOperationsModule() {
        return truffleExceptionOperationsModule;
    }

    public DynamicObject getTruffleInteropModule() {
        return truffleInteropModule;
    }

    public DynamicObject getTruffleInteropForeignClass() {
        return truffleInteropForeignClass;
    }

    public DynamicObject getTruffleInteropJavaModule() {
        return truffleInteropJavaModule;
    }

    public DynamicObject getTruffleKernelOperationsModule() {
        return truffleKernelOperationsModule;
    }

    public DynamicObject getTruffleRegexpOperationsModule() {
        return truffleRegexpOperationsModule;
    }

    public DynamicObject getDirClass() {
        return dirClass;
    }

    public static final String[] CORE_FILES = {
            "/core/pre.rb",
            "/core/basic_object.rb",
            "/core/array.rb",
            "/core/channel.rb",
            "/core/configuration.rb",
            "/core/false.rb",
            "/core/gc.rb",
            "/core/nil.rb",
            "/core/truffle/platform.rb",
            "/core/support.rb",
            "/core/string.rb",
            "/core/random.rb",
            "/core/truffle/io_operations.rb",
            "/core/truffle/kernel_operations.rb",
            "/core/truffle/thread_operations.rb",
            "/core/thread.rb",
            "/core/true.rb",
            "/core/type.rb",
            "/core/weakref.rb",
            "/core/truffle/ffi/pointer.rb",
            "/core/truffle/support.rb",
            "/core/kernel.rb",
            "/core/truffle/boot.rb",
            "/core/truffle/debug.rb",
            "/core/truffle/exception_operations.rb",
            "/core/truffle/numeric_operations.rb",
            "/core/truffle/proc_operations.rb",
            "/core/truffle/range_operations.rb",
            "/core/truffle/regexp_operations.rb",
            "/core/truffle/string_operations.rb",
            "/core/splitter.rb",
            "/core/stat.rb",
            "/core/io.rb",
            "/core/immediate.rb",
            "/core/module.rb",
            "/core/proc.rb",
            "/core/enumerable_helper.rb",
            "/core/enumerable.rb",
            "/core/enumerator.rb",
            "/core/argf.rb",
            "/core/exception.rb",
            "/core/hash.rb",
            "/core/comparable.rb",
            "/core/numeric.rb",
            "/core/truffle/ctype.rb",
            "/core/integer.rb",
            "/core/regexp.rb",
            "/core/transcoding.rb",
            "/core/encoding.rb",
            "/core/env.rb",
            "/core/errno.rb",
            "/core/file.rb",
            "/core/dir.rb",
            "/core/dir_glob.rb",
            "/core/file_test.rb",
            "/core/float.rb",
            "/core/marshal.rb",
            "/core/object_space.rb",
            "/core/range.rb",
            "/core/struct.rb",
            "/core/tms.rb",
            "/core/process.rb",
            "/core/truffle/process_operations.rb", // Must load after /core/regexp.rb
            "/core/signal.rb",
            "/core/symbol.rb",
            "/core/mutex.rb",
            "/core/throw_catch.rb",
            "/core/time.rb",
            "/core/rational.rb",
            "/core/rationalizer.rb",
            "/core/complex.rb",
            "/core/complexifier.rb",
            "/core/class.rb",
            "/core/binding.rb",
            "/core/math.rb",
            "/core/method.rb",
            "/core/unbound_method.rb",
            "/core/truffle/interop.rb",
            "/core/truffle/polyglot.rb",
            "/core/posix.rb",
            "/core/main.rb",
            "/core/post.rb"
    };

}
