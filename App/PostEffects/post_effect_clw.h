/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/
#pragma once

#include "post_effect.h"

#include "CLW.h"
#include "../CLW/clwoutput.h"

namespace Baikal
{
    /**
    \brief Post effects partial implementation based on CLW framework.
    */
    class PostEffectClw: public PostEffect
    {
    public:
        // Constructor, receives CLW context
        PostEffectClw(CLWContext context);

        // Check output compatibility, CLW effects are only compatible
        // with ClwOutput.
        bool IsCompatible(Output const& output) const override;

    protected:
        CLWContext GetContext() const;

    private:
        CLWContext m_context;
    };

    inline PostEffectClw::PostEffectClw(CLWContext context)
        : m_context(context)
    {
    }

    inline bool PostEffectClw::IsCompatible(Output const& output) const
    {
        auto tmp = dynamic_cast<ClwOutput const*>(&output);

        return tmp ? true : false;
    }

    inline CLWContext PostEffectClw::GetContext() const
    {
        return m_context;
    }
}
