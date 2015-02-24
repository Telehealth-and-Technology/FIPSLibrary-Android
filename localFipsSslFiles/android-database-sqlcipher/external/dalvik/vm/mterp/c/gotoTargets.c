/*
 * C footer.  This has some common code shared by the various targets.
 */

/*
 * Everything from here on is a "goto target".  In the basic interpreter
 * we jump into these targets and then jump directly to the handler for
 * next instruction.  Here, these are subroutines that return to the caller.
 */

GOTO_TARGET(filledNewArray, bool methodCallRange)
    {
        ClassObject* arrayClass;
        ArrayObject* newArray;
        u4* contents;
        char typeCh;
        int i;
        u4 arg5;

        EXPORT_PC();

        ref = FETCH(1);             /* class ref */
        vdst = FETCH(2);            /* first 4 regs -or- range base */

        if (methodCallRange) {
            vsrc1 = INST_AA(inst);  /* #of elements */
            arg5 = -1;              /* silence compiler warning */
            ILOGV("|filled-new-array-range args=%d @0x%04x {regs=v%d-v%d}",
                vsrc1, ref, vdst, vdst+vsrc1-1);
        } else {
            arg5 = INST_A(inst);
            vsrc1 = INST_B(inst);   /* #of elements */
            ILOGV("|filled-new-array args=%d @0x%04x {regs=0x%04x %x}",
                vsrc1, ref, vdst, arg5);
        }

        /*
         * Resolve the array class.
         */
        arrayClass = dvmDexGetResolvedClass(methodClassDex, ref);
        if (arrayClass == NULL) {
            arrayClass = dvmResolveClass(curMethod->clazz, ref, false);
            if (arrayClass == NULL)
                GOTO_exceptionThrown();
        }
        /*
        if (!dvmIsArrayClass(arrayClass)) {
            dvmThrowException("Ljava/lang/RuntimeError;",
                "filled-new-array needs array class");
            GOTO_exceptionThrown();
        }
        */
        /* verifier guarantees this is an array class */
        assert(dvmIsArrayClass(arrayClass));
        assert(dvmIsClassInitialized(arrayClass));

        /*
         * Create an array of the specified type.
         */
        LOGVV("+++ filled-new-array type is '%s'\n", arrayClass->descriptor);
        typeCh = arrayClass->descriptor[1];
        if (typeCh == 'D' || typeCh == 'J') {
            /* category 2 primitives not allowed */
            dvmThrowException("Ljava/lang/RuntimeError;",
                "bad filled array req");
            GOTO_exceptionThrown();
        } else if (typeCh != 'L' && typeCh != '[' && typeCh != 'I') {
            /* TODO: requires multiple "fill in" loops with different widths */
            LOGE("non-int primitives not implemented\n");
            dvmThrowException("Ljava/lang/InternalError;",
                "filled-new-array not implemented for anything but 'int'");
            GOTO_exceptionThrown();
        }

        newArray = dvmAllocArrayByClass(arrayClass, vsrc1, ALLOC_DONT_TRACK);
        if (newArray == NULL)
            GOTO_exceptionThrown();

        /*
         * Fill in the elements.  It's legal for vsrc1 to be zero.
         */
        contents = (u4*) newArray->contents;
        if (methodCallRange) {
            for (i = 0; i < vsrc1; i++)
                contents[i] = GET_REGISTER(vdst+i);
        } else {
            assert(vsrc1 <= 5);
            if (vsrc1 == 5) {
                contents[4] = GET_REGISTER(arg5);
                vsrc1--;
            }
            for (i = 0; i < vsrc1; i++) {
                contents[i] = GET_REGISTER(vdst & 0x0f);
                vdst >>= 4;
            }
        }
        if (typeCh == 'L' || typeCh == '[') {
            dvmWriteBarrierArray(newArray, 0, newArray->length);
        }

        retval.l = newArray;
    }
    FINISH(3);
GOTO_TARGET_END


GOTO_TARGET(invokeVirtual, bool methodCallRange)
    {
        Method* baseMethod;
        Object* thisPtr;

        EXPORT_PC();

        vsrc1 = INST_AA(inst);      /* AA (count) or BA (count + arg 5) */
        ref = FETCH(1);             /* method ref */
        vdst = FETCH(2);            /* 4 regs -or- first reg */

        /*
         * The object against which we are executing a method is always
         * in the first argument.
         */
        if (methodCallRange) {
            assert(vsrc1 > 0);
            ILOGV("|invoke-virtual-range args=%d @0x%04x {regs=v%d-v%d}",
                vsrc1, ref, vdst, vdst+vsrc1-1);
            thisPtr = (Object*) GET_REGISTER(vdst);
        } else {
            assert((vsrc1>>4) > 0);
            ILOGV("|invoke-virtual args=%d @0x%04x {regs=0x%04x %x}",
                vsrc1 >> 4, ref, vdst, vsrc1 & 0x0f);
            thisPtr = (Object*) GET_REGISTER(vdst & 0x0f);
        }

        if (!checkForNull(thisPtr))
            GOTO_exceptionThrown();

        /*
         * Resolve the method.  This is the correct method for the static
         * type of the object.  We also verify access permissions here.
         */
        baseMethod = dvmDexGetResolvedMethod(methodClassDex, ref);
        if (baseMethod == NULL) {
            baseMethod = dvmResolveMethod(curMethod->clazz, ref,METHOD_VIRTUAL);
            if (baseMethod == NULL) {
                ILOGV("+ unknown method or access denied\n");
                GOTO_exceptionThrown();
            }
        }

        /*
         * Combine the object we found with the vtable offset in the
         * method.
         */
        assert(baseMethod->methodIndex < thisPtr->clazz->vtableCount);
        methodToCall = thisPtr->clazz->vtable[baseMethod->methodIndex];

#if defined(WITH_JIT) && (INTERP_TYPE == INTERP_DBG)
        callsiteClass = thisPtr->clazz;
#endif

#if 0
        if (dvmIsAbstractMethod(methodToCall)) {
            /*
             * This can happen if you create two classes, Base and Sub, where
             * Sub is a sub-class of Base.  Declare a protected abstract
             * method foo() in Base, and invoke foo() from a method in Base.
             * Base is an "abstract base class" and is never instantiated
             * directly.  Now, Override foo() in Sub, and use Sub.  This
             * Works fine unless Sub stops providing an implementation of
             * the method.
             */
            dvmThrowException("Ljava/lang/AbstractMethodError;",
                "abstract method not implemented");
            GOTO_exceptionThrown();
        }
#else
        assert(!dvmIsAbstractMethod(methodToCall) ||
            methodToCall->nativeFunc != NULL);
#endif

        LOGVV("+++ base=%s.%s virtual[%d]=%s.%s\n",
            baseMethod->clazz->descriptor, baseMethod->name,
            (u4) baseMethod->methodIndex,
            methodToCall->clazz->descriptor, methodToCall->name);
        assert(methodToCall != NULL);

#if 0
        if (vsrc1 != methodToCall->insSize) {
            LOGW("WRONG METHOD: base=%s.%s virtual[%d]=%s.%s\n",
                baseMethod->clazz->descriptor, baseMethod->name,
                (u4) baseMethod->methodIndex,
                methodToCall->clazz->descriptor, methodToCall->name);
            //dvmDumpClass(baseMethod->clazz);
            //dvmDumpClass(methodToCall->clazz);
            dvmDumpAllClasses(0);
        }
#endif

        GOTO_invokeMethod(methodCallRange, methodToCall, vsrc1, vdst);
    }
GOTO_TARGET_END

GOTO_TARGET(invokeSuper, bool methodCallRange)
    {
        Method* baseMethod;
        u2 thisReg;

        EXPORT_PC();

        vsrc1 = INST_AA(inst);      /* AA (count) or BA (count + arg 5) */
        ref = FETCH(1);             /* method ref */
        vdst = FETCH(2);            /* 4 regs -or- first reg */

        if (methodCallRange) {
            ILOGV("|invoke-super-range args=%d @0x%04x {regs=v%d-v%d}",
                vsrc1, ref, vdst, vdst+vsrc1-1);
            thisReg = vdst;
        } else {
            ILOGV("|invoke-super args=%d @0x%04x {regs=0x%04x %x}",
                vsrc1 >> 4, ref, vdst, vsrc1 & 0x0f);
            thisReg = vdst & 0x0f;
        }
        /* impossible in well-formed code, but we must check nevertheless */
        if (!checkForNull((Object*) GET_REGISTER(thisReg)))
            GOTO_exceptionThrown();

        /*
         * Resolve the method.  This is the correct method for the static
         * type of the object.  We also verify access permissions here.
         * The first arg to dvmResolveMethod() is just the referring class
         * (used for class loaders and such), so we don't want to pass
         * the superclass into the resolution call.
         */
        baseMethod = dvmDexGetResolvedMethod(methodClassDex, ref);
        if (baseMethod == NULL) {
            baseMethod = dvmResolveMethod(curMethod->clazz, ref,METHOD_VIRTUAL);
            if (baseMethod == NULL) {
                ILOGV("+ unknown method or access denied\n");
                GOTO_exceptionThrown();
            }
        }

        /*
         * Combine the object we found with the vtable offset in the
         * method's class.
         *
         * We're using the current method's class' superclass, not the
         * superclass of "this".  This is because we might be executing
         * in a method inherited from a superclass, and we want to run
         * in that class' superclass.
         */
        if (baseMethod->methodIndex >= curMethod->clazz->super->vtableCount) {
            /*
             * Method does not exist in the superclass.  Could happen if
             * superclass gets updated.
             */
            dvmThrowException("Ljava/lang/NoSuchMethodError;",
                baseMethod->name);
            GOTO_exceptionThrown();
        }
        methodToCall = curMethod->clazz->super->vtable[baseMethod->methodIndex];
#if 0
        if (dvmIsAbstractMethod(methodToCall)) {
            dvmThrowException("Ljava/lang/AbstractMethodError;",
                "abstract method not implemented");
            GOTO_exceptionThrown();
        }
#else
        assert(!dvmIsAbstractMethod(methodToCall) ||
            methodToCall->nativeFunc != NULL);
#endif
        LOGVV("+++ base=%s.%s super-virtual=%s.%s\n",
            baseMethod->clazz->descriptor, baseMethod->name,
            methodToCall->clazz->descriptor, methodToCall->name);
        assert(methodToCall != NULL);

        GOTO_invokeMethod(methodCallRange, methodToCall, vsrc1, vdst);
    }
GOTO_TARGET_END

GOTO_TARGET(invokeInterface, bool methodCallRange)
    {
        Object* thisPtr;
        ClassObject* thisClass;

        EXPORT_PC();

        vsrc1 = INST_AA(inst);      /* AA (count) or BA (count + arg 5) */
        ref = FETCH(1);             /* method ref */
        vdst = FETCH(2);            /* 4 regs -or- first reg */

        /*
         * The object against which we are executing a method is always
         * in the first argument.
         */
        if (methodCallRange) {
            assert(vsrc1 > 0);
            ILOGV("|invoke-interface-range args=%d @0x%04x {regs=v%d-v%d}",
                vsrc1, ref, vdst, vdst+vsrc1-1);
            thisPtr = (Object*) GET_REGISTER(vdst);
        } else {
            assert((vsrc1>>4) > 0);
            ILOGV("|invoke-interface args=%d @0x%04x {regs=0x%04x %x}",
                vsrc1 >> 4, ref, vdst, vsrc1 & 0x0f);
            thisPtr = (Object*) GET_REGISTER(vdst & 0x0f);
        }
        if (!checkForNull(thisPtr))
            GOTO_exceptionThrown();

        thisClass = thisPtr->clazz;

#if defined(WITH_JIT) && (INTERP_TYPE == INTERP_DBG)
        callsiteClass = thisClass;
#endif

        /*
         * Given a class and a method index, find the Method* with the
         * actual code we want to execute.
         */
        methodToCall = dvmFindInterfaceMethodInCache(thisClass, ref, curMethod,
                        methodClassDex);
        if (methodToCall == NULL) {
            assert(dvmCheckException(self));
            GOTO_exceptionThrown();
        }

        GOTO_invokeMethod(methodCallRange, methodToCall, vsrc1, vdst);
    }
GOTO_TARGET_END

GOTO_TARGET(invokeDirect, bool methodCallRange)
    {
        u2 thisReg;

        vsrc1 = INST_AA(inst);      /* AA (count) or BA (count + arg 5) */
        ref = FETCH(1);             /* method ref */
        vdst = FETCH(2);            /* 4 regs -or- first reg */

        EXPORT_PC();

        if (methodCallRange) {
            ILOGV("|invoke-direct-range args=%d @0x%04x {regs=v%d-v%d}",
                vsrc1, ref, vdst, vdst+vsrc1-1);
            thisReg = vdst;
        } else {
            ILOGV("|invoke-direct args=%d @0x%04x {regs=0x%04x %x}",
                vsrc1 >> 4, ref, vdst, vsrc1 & 0x0f);
            thisReg = vdst & 0x0f;
        }
        if (!checkForNull((Object*) GET_REGISTER(thisReg)))
            GOTO_exceptionThrown();

        methodToCall = dvmDexGetResolvedMethod(methodClassDex, ref);
        if (methodToCall == NULL) {
            methodToCall = dvmResolveMethod(curMethod->clazz, ref,
                            METHOD_DIRECT);
            if (methodToCall == NULL) {
                ILOGV("+ unknown direct method\n");     // should be impossible
                GOTO_exceptionThrown();
            }
        }
        GOTO_invokeMethod(methodCallRange, methodToCall, vsrc1, vdst);
    }
GOTO_TARGET_END

GOTO_TARGET(invokeStatic, bool methodCallRange)
    vsrc1 = INST_AA(inst);      /* AA (count) or BA (count + arg 5) */
    ref = FETCH(1);             /* method ref */
    vdst = FETCH(2);            /* 4 regs -or- first reg */

    EXPORT_PC();

    if (methodCallRange)
        ILOGV("|invoke-static-range args=%d @0x%04x {regs=v%d-v%d}",
            vsrc1, ref, vdst, vdst+vsrc1-1);
    else
        ILOGV("|invoke-static args=%d @0x%04x {regs=0x%04x %x}",
            vsrc1 >> 4, ref, vdst, vsrc1 & 0x0f);

    methodToCall = dvmDexGetResolvedMethod(methodClassDex, ref);
    if (methodToCall == NULL) {
        methodToCall = dvmResolveMethod(curMethod->clazz, ref, METHOD_STATIC);
        if (methodToCall == NULL) {
            ILOGV("+ unknown method\n");
            GOTO_exceptionThrown();
        }

        /*
         * The JIT needs dvmDexGetResolvedMethod() to return non-null.
         * Since we use the portable interpreter to build the trace, this extra
         * check is not needed for mterp.
         */
        if (dvmDexGetResolvedMethod(methodClassDex, ref) == NULL) {
            /* Class initialization is still ongoing */
            ABORT_JIT_TSELECT();
        }
    }
    GOTO_invokeMethod(methodCallRange, methodToCall, vsrc1, vdst);
GOTO_TARGET_END

GOTO_TARGET(invokeVirtualQuick, bool methodCallRange)
    {
        Object* thisPtr;

        EXPORT_PC();

        vsrc1 = INST_AA(inst);      /* AA (count) or BA (count + arg 5) */
        ref = FETCH(1);             /* vtable index */
        vdst = FETCH(2);            /* 4 regs -or- first reg */

        /*
         * The object against which we are executing a method is always
         * in the first argument.
         */
        if (methodCallRange) {
            assert(vsrc1 > 0);
            ILOGV("|invoke-virtual-quick-range args=%d @0x%04x {regs=v%d-v%d}",
                vsrc1, ref, vdst, vdst+vsrc1-1);
            thisPtr = (Object*) GET_REGISTER(vdst);
        } else {
            assert((vsrc1>>4) > 0);
            ILOGV("|invoke-virtual-quick args=%d @0x%04x {regs=0x%04x %x}",
                vsrc1 >> 4, ref, vdst, vsrc1 & 0x0f);
            thisPtr = (Object*) GET_REGISTER(vdst & 0x0f);
        }

        if (!checkForNull(thisPtr))
            GOTO_exceptionThrown();

#if defined(WITH_JIT) && (INTERP_TYPE == INTERP_DBG)
        callsiteClass = thisPtr->clazz;
#endif

        /*
         * Combine the object we found with the vtable offset in the
         * method.
         */
        assert(ref < thisPtr->clazz->vtableCount);
        methodToCall = thisPtr->clazz->vtable[ref];

#if 0
        if (dvmIsAbstractMethod(methodToCall)) {
            dvmThrowException("Ljava/lang/AbstractMethodError;",
                "abstract method not implemented");
            GOTO_exceptionThrown();
        }
#else
        assert(!dvmIsAbstractMethod(methodToCall) ||
            methodToCall->nativeFunc != NULL);
#endif

        LOGVV("+++ virtual[%d]=%s.%s\n",
            ref, methodToCall->clazz->descriptor, methodToCall->name);
        assert(methodToCall != NULL);

        GOTO_invokeMethod(methodCallRange, methodToCall, vsrc1, vdst);
    }
GOTO_TARGET_END

GOTO_TARGET(invokeSuperQuick, bool methodCallRange)
    {
        u2 thisReg;

        EXPORT_PC();

        vsrc1 = INST_AA(inst);      /* AA (count) or BA (count + arg 5) */
        ref = FETCH(1);             /* vtable index */
        vdst = FETCH(2);            /* 4 regs -or- first reg */

        if (methodCallRange) {
            ILOGV("|invoke-super-quick-range args=%d @0x%04x {regs=v%d-v%d}",
                vsrc1, ref, vdst, vdst+vsrc1-1);
            thisReg = vdst;
        } else {
            ILOGV("|invoke-super-quick args=%d @0x%04x {regs=0x%04x %x}",
                vsrc1 >> 4, ref, vdst, vsrc1 & 0x0f);
            thisReg = vdst & 0x0f;
        }
        /* impossible in well-formed code, but we must check nevertheless */
        if (!checkForNull((Object*) GET_REGISTER(thisReg)))
            GOTO_exceptionThrown();

#if 0   /* impossible in optimized + verified code */
        if (ref >= curMethod->clazz->super->vtableCount) {
            dvmThrowException("Ljava/lang/NoSuchMethodError;", NULL);
            GOTO_exceptionThrown();
        }
#else
        assert(ref < curMethod->clazz->super->vtableCount);
#endif

        /*
         * Combine the object we found with the vtable offset in the
         * method's class.
         *
         * We're using the current method's class' superclass, not the
         * superclass of "this".  This is because we might be executing
         * in a method inherited from a superclass, and we want to run
         * in the method's class' superclass.
         */
        methodToCall = curMethod->clazz->super->vtable[ref];

#if 0
        if (dvmIsAbstractMethod(methodToCall)) {
            dvmThrowException("Ljava/lang/AbstractMethodError;",
                "abstract method not implemented");
            GOTO_exceptionThrown();
        }
#else
        assert(!dvmIsAbstractMethod(methodToCall) ||
            methodToCall->nativeFunc != NULL);
#endif
        LOGVV("+++ super-virtual[%d]=%s.%s\n",
            ref, methodToCall->clazz->descriptor, methodToCall->name);
        assert(methodToCall != NULL);

        GOTO_invokeMethod(methodCallRange, methodToCall, vsrc1, vdst);
    }
GOTO_TARGET_END


    /*
     * General handling for return-void, return, and return-wide.  Put the
     * return value in "retval" before jumping here.
     */
GOTO_TARGET(returnFromMethod)
    {
        StackSaveArea* saveArea;

        /*
         * We must do this BEFORE we pop the previous stack frame off, so
         * that the GC can see the return value (if any) in the local vars.
         *
         * Since this is now an interpreter switch point, we must do it before
         * we do anything at all.
         */
        PERIODIC_CHECKS(kInterpEntryReturn, 0);

        ILOGV("> retval=0x%llx (leaving %s.%s %s)",
            retval.j, curMethod->clazz->descriptor, curMethod->name,
            curMethod->shorty);
        //DUMP_REGS(curMethod, fp);

        saveArea = SAVEAREA_FROM_FP(fp);

#ifdef EASY_GDB
        debugSaveArea = saveArea;
#endif
#if (INTERP_TYPE == INTERP_DBG)
        TRACE_METHOD_EXIT(self, curMethod);
#endif

        /* back up to previous frame and see if we hit a break */
        fp = saveArea->prevFrame;
        assert(fp != NULL);
        if (dvmIsBreakFrame(fp)) {
            /* bail without popping the method frame from stack */
            LOGVV("+++ returned into break frame\n");
#if defined(WITH_JIT)
            /* Let the Jit know the return is terminating normally */
            CHECK_JIT_VOID();
#endif
            GOTO_bail();
        }

        /* update thread FP, and reset local variables */
        self->curFrame = fp;
        curMethod = SAVEAREA_FROM_FP(fp)->method;
        //methodClass = curMethod->clazz;
        methodClassDex = curMethod->clazz->pDvmDex;
        pc = saveArea->savedPc;
        ILOGD("> (return to %s.%s %s)", curMethod->clazz->descriptor,
            curMethod->name, curMethod->shorty);

        /* use FINISH on the caller's invoke instruction */
        //u2 invokeInstr = INST_INST(FETCH(0));
        if (true /*invokeInstr >= OP_INVOKE_VIRTUAL &&
            invokeInstr <= OP_INVOKE_INTERFACE*/)
        {
            FINISH(3);
        } else {
            //LOGE("Unknown invoke instr %02x at %d\n",
            //    invokeInstr, (int) (pc - curMethod->insns));
            assert(false);
        }
    }
GOTO_TARGET_END


    /*
     * Jump here when the code throws an exception.
     *
     * By the time we get here, the Throwable has been created and the stack
     * trace has been saved off.
     */
GOTO_TARGET(exceptionThrown)
    {
        Object* exception;
        int catchRelPc;

        /*
         * Since this is now an interpreter switch point, we must do it before
         * we do anything at all.
         */
        PERIODIC_CHECKS(kInterpEntryThrow, 0);

#if defined(WITH_JIT)
        // Something threw during trace selection - abort the current trace
        ABORT_JIT_TSELECT();
#endif
        /*
         * We save off the exception and clear the exception status.  While
         * processing the exception we might need to load some Throwable
         * classes, and we don't want class loader exceptions to get
         * confused with this one.
         */
        assert(dvmCheckException(self));
        exception = dvmGetException(self);
        dvmAddTrackedAlloc(exception, self);
        dvmClearException(self);

        LOGV("Handling exception %s at %s:%d\n",
            exception->clazz->descriptor, curMethod->name,
            dvmLineNumFromPC(curMethod, pc - curMethod->insns));

#if (INTERP_TYPE == INTERP_DBG)
        /*
         * Tell the debugger about it.
         *
         * TODO: if the exception was thrown by interpreted code, control
         * fell through native, and then back to us, we will report the
         * exception at the point of the throw and again here.  We can avoid
         * this by not reporting exceptions when we jump here directly from
         * the native call code above, but then we won't report exceptions
         * that were thrown *from* the JNI code (as opposed to *through* it).
         *
         * The correct solution is probably to ignore from-native exceptions
         * here, and have the JNI exception code do the reporting to the
         * debugger.
         */
        if (gDvm.debuggerActive) {
            void* catchFrame;
            catchRelPc = dvmFindCatchBlock(self, pc - curMethod->insns,
                        exception, true, &catchFrame);
            dvmDbgPostException(fp, pc - curMethod->insns, catchFrame,
                catchRelPc, exception);
        }
#endif

        /*
         * We need to unroll to the catch block or the nearest "break"
         * frame.
         *
         * A break frame could indicate that we have reached an intermediate
         * native call, or have gone off the top of the stack and the thread
         * needs to exit.  Either way, we return from here, leaving the
         * exception raised.
         *
         * If we do find a catch block, we want to transfer execution to
         * that point.
         *
         * Note this can cause an exception while resolving classes in
         * the "catch" blocks.
         */
        catchRelPc = dvmFindCatchBlock(self, pc - curMethod->insns,
                    exception, false, (void*)&fp);

        /*
         * Restore the stack bounds after an overflow.  This isn't going to
         * be correct in all circumstances, e.g. if JNI code devours the
         * exception this won't happen until some other exception gets
         * thrown.  If the code keeps pushing the stack bounds we'll end
         * up aborting the VM.
         *
         * Note we want to do this *after* the call to dvmFindCatchBlock,
         * because that may need extra stack space to resolve exception
         * classes (e.g. through a class loader).
         *
         * It's possible for the stack overflow handling to cause an
         * exception (specifically, class resolution in a "catch" block
         * during the call above), so we could see the thread's overflow
         * flag raised but actually be running in a "nested" interpreter
         * frame.  We don't allow doubled-up StackOverflowErrors, so
         * we can check for this by just looking at the exception type
         * in the cleanup function.  Also, we won't unroll past the SOE
         * point because the more-recent exception will hit a break frame
         * as it unrolls to here.
         */
        if (self->stackOverflowed)
            dvmCleanupStackOverflow(self, exception);

        if (catchRelPc < 0) {
            /* falling through to JNI code or off the bottom of the stack */
#if DVM_SHOW_EXCEPTION >= 2
            LOGD("Exception %s from %s:%d not caught locally\n",
                exception->clazz->descriptor, dvmGetMethodSourceFile(curMethod),
                dvmLineNumFromPC(curMethod, pc - curMethod->insns));
#endif
            dvmSetException(self, exception);
            dvmReleaseTrackedAlloc(exception, self);
            GOTO_bail();
        }

#if DVM_SHOW_EXCEPTION >= 3
        {
            const Method* catchMethod = SAVEAREA_FROM_FP(fp)->method;
            LOGD("Exception %s thrown from %s:%d to %s:%d\n",
                exception->clazz->descriptor, dvmGetMethodSourceFile(curMethod),
                dvmLineNumFromPC(curMethod, pc - curMethod->insns),
                dvmGetMethodSourceFile(catchMethod),
                dvmLineNumFromPC(catchMethod, catchRelPc));
        }
#endif

        /*
         * Adjust local variables to match self->curFrame and the
         * updated PC.
         */
        //fp = (u4*) self->curFrame;
        curMethod = SAVEAREA_FROM_FP(fp)->method;
        //methodClass = curMethod->clazz;
        methodClassDex = curMethod->clazz->pDvmDex;
        pc = curMethod->insns + catchRelPc;
        ILOGV("> pc <-- %s.%s %s", curMethod->clazz->descriptor,
            curMethod->name, curMethod->shorty);
        DUMP_REGS(curMethod, fp, false);            // show all regs

        /*
         * Restore the exception if the handler wants it.
         *
         * The Dalvik spec mandates that, if an exception handler wants to
         * do something with the exception, the first instruction executed
         * must be "move-exception".  We can pass the exception along
         * through the thread struct, and let the move-exception instruction
         * clear it for us.
         *
         * If the handler doesn't call move-exception, we don't want to
         * finish here with an exception still pending.
         */
        if (INST_INST(FETCH(0)) == OP_MOVE_EXCEPTION)
            dvmSetException(self, exception);

        dvmReleaseTrackedAlloc(exception, self);
        FINISH(0);
    }
GOTO_TARGET_END



    /*
     * General handling for invoke-{virtual,super,direct,static,interface},
     * including "quick" variants.
     *
     * Set "methodToCall" to the Method we're calling, and "methodCallRange"
     * depending on whether this is a "/range" instruction.
     *
     * For a range call:
     *  "vsrc1" holds the argument count (8 bits)
     *  "vdst" holds the first argument in the range
     * For a non-range call:
     *  "vsrc1" holds the argument count (4 bits) and the 5th argument index
     *  "vdst" holds four 4-bit register indices
     *
     * The caller must EXPORT_PC before jumping here, because any method
     * call can throw a stack overflow exception.
     */
GOTO_TARGET(invokeMethod, bool methodCallRange, const Method* _methodToCall,
    u2 count, u2 regs)
    {
        STUB_HACK(vsrc1 = count; vdst = regs; methodToCall = _methodToCall;);

        //printf("range=%d call=%p count=%d regs=0x%04x\n",
        //    methodCallRange, methodToCall, count, regs);
        //printf(" --> %s.%s %s\n", methodToCall->clazz->descriptor,
        //    methodToCall->name, methodToCall->shorty);

        u4* outs;
        int i;

        /*
         * Copy args.  This may corrupt vsrc1/vdst.
         */
        if (methodCallRange) {
            // could use memcpy or a "Duff's device"; most functions have
            // so few args it won't matter much
            assert(vsrc1 <= curMethod->outsSize);
            assert(vsrc1 == methodToCall->insSize);
            outs = OUTS_FROM_FP(fp, vsrc1);
            for (i = 0; i < vsrc1; i++)
                outs[i] = GET_REGISTER(vdst+i);
        } else {
            u4 count = vsrc1 >> 4;

            assert(count <= curMethod->outsSize);
            assert(count == methodToCall->insSize);
            assert(count <= 5);

            outs = OUTS_FROM_FP(fp, count);
#if 0
            if (count == 5) {
                outs[4] = GET_REGISTER(vsrc1 & 0x0f);
                count--;
            }
            for (i = 0; i < (int) count; i++) {
                outs[i] = GET_REGISTER(vdst & 0x0f);
                vdst >>= 4;
            }
#else
            // This version executes fewer instructions but is larger
            // overall.  Seems to be a teensy bit faster.
            assert((vdst >> 16) == 0);  // 16 bits -or- high 16 bits clear
            switch (count) {
            case 5:
                outs[4] = GET_REGISTER(vsrc1 & 0x0f);
            case 4:
                outs[3] = GET_REGISTER(vdst >> 12);
            case 3:
                outs[2] = GET_REGISTER((vdst & 0x0f00) >> 8);
            case 2:
                outs[1] = GET_REGISTER((vdst & 0x00f0) >> 4);
            case 1:
                outs[0] = GET_REGISTER(vdst & 0x0f);
            default:
                ;
            }
#endif
        }
    }

    /*
     * (This was originally a "goto" target; I've kept it separate from the
     * stuff above in case we want to refactor things again.)
     *
     * At this point, we have the arguments stored in the "outs" area of
     * the current method's stack frame, and the method to call in
     * "methodToCall".  Push a new stack frame.
     */
    {
        StackSaveArea* newSaveArea;
        u4* newFp;

        ILOGV("> %s%s.%s %s",
            dvmIsNativeMethod(methodToCall) ? "(NATIVE) " : "",
            methodToCall->clazz->descriptor, methodToCall->name,
            methodToCall->shorty);

        newFp = (u4*) SAVEAREA_FROM_FP(fp) - methodToCall->registersSize;
        newSaveArea = SAVEAREA_FROM_FP(newFp);

        /* verify that we have enough space */
        if (true) {
            u1* bottom;
            bottom = (u1*) newSaveArea - methodToCall->outsSize * sizeof(u4);
            if (bottom < self->interpStackEnd) {
                /* stack overflow */
                LOGV("Stack overflow on method call (start=%p end=%p newBot=%p(%d) size=%d '%s')\n",
                    self->interpStackStart, self->interpStackEnd, bottom,
                    (u1*) fp - bottom, self->interpStackSize,
                    methodToCall->name);
                dvmHandleStackOverflow(self, methodToCall);
                assert(dvmCheckException(self));
                GOTO_exceptionThrown();
            }
            //LOGD("+++ fp=%p newFp=%p newSave=%p bottom=%p\n",
            //    fp, newFp, newSaveArea, bottom);
        }

#ifdef LOG_INSTR
        if (methodToCall->registersSize > methodToCall->insSize) {
            /*
             * This makes valgrind quiet when we print registers that
             * haven't been initialized.  Turn it off when the debug
             * messages are disabled -- we want valgrind to report any
             * used-before-initialized issues.
             */
            memset(newFp, 0xcc,
                (methodToCall->registersSize - methodToCall->insSize) * 4);
        }
#endif

#ifdef EASY_GDB
        newSaveArea->prevSave = SAVEAREA_FROM_FP(fp);
#endif
        newSaveArea->prevFrame = fp;
        newSaveArea->savedPc = pc;
#if defined(WITH_JIT)
        newSaveArea->returnAddr = 0;
#endif
        newSaveArea->method = methodToCall;

        if (!dvmIsNativeMethod(methodToCall)) {
            /*
             * "Call" interpreted code.  Reposition the PC, update the
             * frame pointer and other local state, and continue.
             */
            curMethod = methodToCall;
            methodClassDex = curMethod->clazz->pDvmDex;
            pc = methodToCall->insns;
            fp = self->curFrame = newFp;
#ifdef EASY_GDB
            debugSaveArea = SAVEAREA_FROM_FP(newFp);
#endif
#if INTERP_TYPE == INTERP_DBG
            debugIsMethodEntry = true;              // profiling, debugging
#endif
            ILOGD("> pc <-- %s.%s %s", curMethod->clazz->descriptor,
                curMethod->name, curMethod->shorty);
            DUMP_REGS(curMethod, fp, true);         // show input args
            FINISH(0);                              // jump to method start
        } else {
            /* set this up for JNI locals, even if not a JNI native */
#ifdef USE_INDIRECT_REF
            newSaveArea->xtra.localRefCookie = self->jniLocalRefTable.segmentState.all;
#else
            newSaveArea->xtra.localRefCookie = self->jniLocalRefTable.nextEntry;
#endif

            self->curFrame = newFp;

            DUMP_REGS(methodToCall, newFp, true);   // show input args

#if (INTERP_TYPE == INTERP_DBG)
            if (gDvm.debuggerActive) {
                dvmDbgPostLocationEvent(methodToCall, -1,
                    dvmGetThisPtr(curMethod, fp), DBG_METHOD_ENTRY);
            }
#endif
#if (INTERP_TYPE == INTERP_DBG)
            TRACE_METHOD_ENTER(self, methodToCall);
#endif

            {
                ILOGD("> native <-- %s.%s %s", methodToCall->clazz->descriptor,
                        methodToCall->name, methodToCall->shorty);
            }

#if defined(WITH_JIT)
            /* Allow the Jit to end any pending trace building */
            CHECK_JIT_VOID();
#endif

            /*
             * Jump through native call bridge.  Because we leave no
             * space for locals on native calls, "newFp" points directly
             * to the method arguments.
             */
            (*methodToCall->nativeFunc)(newFp, &retval, methodToCall, self);

#if (INTERP_TYPE == INTERP_DBG)
            if (gDvm.debuggerActive) {
                dvmDbgPostLocationEvent(methodToCall, -1,
                    dvmGetThisPtr(curMethod, fp), DBG_METHOD_EXIT);
            }
#endif
#if (INTERP_TYPE == INTERP_DBG)
            TRACE_METHOD_EXIT(self, methodToCall);
#endif

            /* pop frame off */
            dvmPopJniLocals(self, newSaveArea);
            self->curFrame = fp;

            /*
             * If the native code threw an exception, or interpreted code
             * invoked by the native call threw one and nobody has cleared
             * it, jump to our local exception handling.
             */
            if (dvmCheckException(self)) {
                LOGV("Exception thrown by/below native code\n");
                GOTO_exceptionThrown();
            }

            ILOGD("> retval=0x%llx (leaving native)", retval.j);
            ILOGD("> (return from native %s.%s to %s.%s %s)",
                methodToCall->clazz->descriptor, methodToCall->name,
                curMethod->clazz->descriptor, curMethod->name,
                curMethod->shorty);

            //u2 invokeInstr = INST_INST(FETCH(0));
            if (true /*invokeInstr >= OP_INVOKE_VIRTUAL &&
                invokeInstr <= OP_INVOKE_INTERFACE*/)
            {
                FINISH(3);
            } else {
                //LOGE("Unknown invoke instr %02x at %d\n",
                //    invokeInstr, (int) (pc - curMethod->insns));
                assert(false);
            }
        }
    }
    assert(false);      // should not get here
GOTO_TARGET_END
