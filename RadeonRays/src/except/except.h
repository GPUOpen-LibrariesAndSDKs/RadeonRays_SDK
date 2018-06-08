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
#ifndef EXCEPT_H
#define EXCEPT_H

#include "radeon_rays.h"
#include <string>

namespace RadeonRays
{
    // Base class for RadeonRays exception handling mechanism
    class ExceptionImpl : public Exception
    {
    public:
        ExceptionImpl(std::string const& s)
            : message_(s)
        {
        }

        char const* what() const override
        {
            return message_.c_str();
        }

        std::string message_;
    };

    inline void ThrowIf(bool condition, std::string const& message)
    {
        if (condition)
        {
            throw ExceptionImpl(message);
        }
    }

    inline void Throw(std::string const& message)
    {
        throw ExceptionImpl(message);
    }
}


#endif // EXCEPT_H
