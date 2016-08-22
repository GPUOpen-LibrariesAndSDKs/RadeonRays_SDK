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

namespace Calc
{
    extern void LogError( const char* in_assertion_text, const char* in_condition, const char* in_file_name, int in_line, const char* in_comment = nullptr );
    extern void Abort();

#ifdef _DEBUG
    #define Assert(e)                if(!(e)) { LogError( "Assertion: %s in %s at %i\n", #e, __FILE__, __LINE__ ); Abort(); }
    #define AssertEx(e, comment)    if(!(e)) { LogError( "Assertion: %s in %s at %i - %s\n", #e, __FILE__, __LINE__, comment ); Abort(); }
#else
    #define Assert(e) {} // do not remove brackets!
    #define AssertEx(e, comment) {} // do not remove brackets!
#endif

#define IS_VULKAN_CALL_SUCCESSFULL( result ) ( ( result == VK_SUCCESS ) || ( result == VK_ERROR_VALIDATION_FAILED_EXT ) )
#define VK_EMPTY_IMPLEMENTATION { AssertEx( false, "Not Implemented" ); }

    template< class T, class S >
    inline T* Cast( S* in_pointer )
    {
#ifdef _DEBUG
        T* to_return = dynamic_cast<T*>( in_pointer );
        Assert( nullptr != to_return );
        return to_return;
#else
        return (T*)in_pointer;
#endif
    }

    template< class T, class S >
    inline T* ConstCast( S const * in_pointer )
    {
#ifdef _DEBUG
        T const * to_return = dynamic_cast<T const *>( in_pointer );
        Assert( nullptr != to_return );
        return const_cast<T*>(to_return);
#else
        return (T*)in_pointer;
#endif
    }
}
