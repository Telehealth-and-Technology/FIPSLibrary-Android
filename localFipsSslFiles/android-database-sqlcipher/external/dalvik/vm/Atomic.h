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
 * Atomic operations
 */
#ifndef _DALVIK_ATOMIC
#define _DALVIK_ATOMIC

#include <cutils/atomic.h>          /* use common Android atomic ops */
#include <cutils/atomic-inline.h>   /* and some uncommon ones */

/*
 * NOTE: Two "quasiatomic" operations on the exact same memory address
 * are guaranteed to operate atomically with respect to each other,
 * but no guarantees are made about quasiatomic operations mixed with
 * non-quasiatomic operations on the same address, nor about
 * quasiatomic operations that are performed on partially-overlapping
 * memory.
 *
 * None of these provide a memory barrier.
 */

/*
 * Swap the 64-bit value at "addr" with "value".  Returns the previous
 * value.
 */
int64_t dvmQuasiAtomicSwap64(int64_t value, volatile int64_t* addr);

/*
 * Read the 64-bit value at "addr".
 */
int64_t dvmQuasiAtomicRead64(volatile const int64_t* addr);

/*
 * If the value at "addr" is equal to "oldvalue", replace it with "newvalue"
 * and return 0.  Otherwise, don't swap, and return nonzero.
 */
int dvmQuasiAtomicCas64(int64_t oldvalue, int64_t newvalue,
        volatile int64_t* addr);

#endif /*_DALVIK_ATOMIC*/
