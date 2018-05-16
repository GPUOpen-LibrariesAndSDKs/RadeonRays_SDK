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
#ifndef __CLW__CLWParallelPrimitives__
#define __CLW__CLWParallelPrimitives__

#include "CLWContext.h"
#include "CLWProgram.h"
#include "CLWEvent.h"
#include "CLWBuffer.h"

class CLWParallelPrimitives
{
public:
    // Create primitive instances for the context
    CLWParallelPrimitives(CLWContext context, char const* buildopts = nullptr);
    CLWParallelPrimitives() = default;
    ~CLWParallelPrimitives();

    ///  TODO: Make these templates at some point
    CLWEvent ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems);
    CLWEvent SegmentedScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output);

    CLWEvent ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output, int numElems);

    CLWEvent SortRadix(unsigned int deviceIdx, CLWBuffer<cl_int> inputKeys, CLWBuffer<cl_int> outputKeys,
                       CLWBuffer<cl_int> inputValues, CLWBuffer<cl_int> outputValues, int numElems);

    CLWEvent SortRadix(unsigned int deviceIdx, CLWBuffer<char> inputKeys, CLWBuffer<char> outputKeys,
        CLWBuffer<char> inputValues, CLWBuffer<char> outputValues, int numElems);

    CLWEvent SortRadix(unsigned int deviceIdx, CLWBuffer<cl_int> inputKeys, CLWBuffer<cl_int> outputKeys);

    CLWEvent Compact(unsigned int deviceIdx, CLWBuffer<cl_int> predicate, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems, cl_int& newSize);
    CLWEvent Compact(unsigned int deviceIdx, CLWBuffer<cl_int> predicate, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems, CLWBuffer<cl_int> newSize);
    CLWEvent Copy(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems);

    void ReclaimDeviceMemory();

protected:
    CLWEvent ScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems);
    CLWEvent SegmentedScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output);
    
    CLWEvent ScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems);
    CLWEvent SegmentedScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output);
    
    CLWEvent ScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems);
    CLWEvent SegmentedScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output);

    CLWEvent SegmentedScanExclusiveAddFourLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output);

    CLWEvent ScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output, int numElems);
    CLWEvent ScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output, int numElems);
    CLWEvent ScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output, int numElems);

    CLWBuffer<cl_int> GetTempIntBuffer(size_t size);
    void              ReclaimTempIntBuffer(CLWBuffer<cl_int> buffer);
    CLWBuffer<char> GetTempCharBuffer(size_t size);
    void              ReclaimTempCharBuffer(CLWBuffer<char> buffer);
    CLWBuffer<cl_float> GetTempFloatBuffer(size_t size);
    void               ReclaimTempFloatBuffer(CLWBuffer<cl_float> buffer);

private:
    CLWContext context_;
    CLWProgram program_;

    std::map<size_t, CLWBuffer<cl_int> > intBufferCache_;
    std::map<size_t, CLWBuffer<char> > charBufferCache_;
    std::map<size_t, CLWBuffer<cl_float> > floatBufferCache_;
};


#endif /* defined(__CLW__CLWParallelPrimitives__) */