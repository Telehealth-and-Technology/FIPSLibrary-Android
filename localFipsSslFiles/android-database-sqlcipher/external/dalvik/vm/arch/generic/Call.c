/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * This uses the FFI (Foreign Function Interface) library to abstract away
 * the system-dependent stuff.  The FFI code is slower than a custom
 * assembly version, but has the distinct advantage of having been
 * written already for several platforms.
 */
#include "Dalvik.h"
#include "ffi.h"

/*
 * Convert a signature type character to an FFI type.
 */
static ffi_type* getFfiType(char sigType)
{
    switch (sigType) {
    case 'V': return &ffi_type_void;
    case 'F': return &ffi_type_float;
    case 'D': return &ffi_type_double;
    case 'J': return &ffi_type_sint64;
    case '[':
    case 'L': return &ffi_type_pointer;
    default:  return &ffi_type_uint32;
    }
}

/*
 * Call "func" with the specified arguments.
 *
 * The second argument to JNI native functions is either the object (the
 * "this" pointer) or, for static functions, a pointer to the class object.
 * The Dalvik instructions will push "this" into argv[0], but it's up to
 * us to insert the class object.
 *
 * Because there is no such thing in as a null "this" pointer, we use
 * the non-NULL state of "clazz" to determine whether or not it's static.
 *
 * For maximum efficiency we should compute the CIF once and save it with
 * the method.  However, this requires storing the data with every native
 * method.  Since the goal is to have custom assembly versions of this
 * on the platforms where performance matters, I'm recomputing the CIF on
 * every call.
 */
void dvmPlatformInvoke(void* pEnv, ClassObject* clazz, int argInfo, int argc,
    const u4* argv, const char* shorty, void* func, JValue* pReturn)
{
    const int kMaxArgs = argc+2;    /* +1 for env, maybe +1 for clazz */
    ffi_cif cif;
    ffi_type* types[kMaxArgs];
    void* values[kMaxArgs];
    ffi_type* retType;
    const char* sig;
    char sigByte;
    int srcArg, dstArg;

    types[0] = &ffi_type_pointer;
    values[0] = &pEnv;

    types[1] = &ffi_type_pointer;
    if (clazz != NULL) {
        values[1] = &clazz;
        srcArg = 0;
    } else {
        values[1] = (void*) argv++;
        srcArg = 1;
    }
    dstArg = 2;

    /*
     * Scan the types out of the short signature.  Use them to fill out the
     * "types" array.  Store the start address of the argument in "values".
     */
    retType = getFfiType(*shorty);
    while ((sigByte = *++shorty) != '\0') {
        types[dstArg] = getFfiType(sigByte);
        values[dstArg++] = (void*) argv++;
        if (sigByte == 'D' || sigByte == 'J')
            argv++;
    }

    /*
     * Prep the CIF (Call InterFace object).
     */
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, dstArg, retType, types) != FFI_OK) {
        LOGE("ffi_prep_cif failed\n");
        dvmAbort();
    }

    ffi_call(&cif, FFI_FN(func), pReturn, values);
}
