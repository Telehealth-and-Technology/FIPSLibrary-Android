/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef ANDROID_RS_PROGRAM_FRAGMENT_H
#define ANDROID_RS_PROGRAM_FRAGMENT_H

#include "rsProgram.h"

// ---------------------------------------------------------------------------
namespace android {
namespace renderscript {

class ProgramFragmentState;

class ProgramFragment : public Program {
public:
    ProgramFragment(Context *rsc, const char * shaderText,
                             uint32_t shaderLength, const uint32_t * params,
                             uint32_t paramLength);
    virtual ~ProgramFragment();

    virtual void setup(Context *, ProgramFragmentState *);

    virtual void serialize(OStream *stream) const;
    virtual RsA3DClassID getClassId() const { return RS_A3D_CLASS_ID_PROGRAM_FRAGMENT; }
    static ProgramFragment *createFromStream(Context *rsc, IStream *stream);

    void setConstantColor(Context *, float, float, float, float);

protected:
    float mConstantColor[4];
    int32_t mTextureUniformIndexStart;
};

class ProgramFragmentState {
public:
    ProgramFragmentState();
    ~ProgramFragmentState();

    ProgramFragment *mPF;
    void init(Context *rsc);
    void deinit(Context *rsc);

    ObjectBaseRef<ProgramFragment> mDefault;
    Vector<ProgramFragment *> mPrograms;

    ObjectBaseRef<ProgramFragment> mLast;
};

}
}
#endif




