/**********************************************************************
Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
    CLWParallelPrimitives(CLWContext context);
    CLWParallelPrimitives(){}
    ~CLWParallelPrimitives();

    ///  TODO: Make these templates at some point
    CLWEvent ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output);
    CLWEvent SegmentedScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output);

    CLWEvent ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output);
    //CLWEvent ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_uint> input, CLWBuffer<cl_uint> output);
    //CLWEvent ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_short> input, CLWBuffer<cl_short> output);
    //CLWEvent ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_ushort> input, CLWBuffer<cl_ushort> output);

    CLWEvent SortRadix(unsigned int deviceIdx, CLWBuffer<cl_int> inputKeys, CLWBuffer<cl_int> outputKeys,
                       CLWBuffer<cl_int> inputValues, CLWBuffer<cl_int> outputValues, int numElems);

	CLWEvent SortRadix(unsigned int deviceIdx, CLWBuffer<char> inputKeys, CLWBuffer<char> outputKeys,
		CLWBuffer<char> inputValues, CLWBuffer<char> outputValues, int numElems);

    CLWEvent SortRadix(unsigned int deviceIdx, CLWBuffer<cl_int> inputKeys, CLWBuffer<cl_int> outputKeys);

    CLWEvent Compact(unsigned int deviceIdx, CLWBuffer<cl_int> predicate, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, cl_int& newSize);
	CLWEvent Compact(unsigned int deviceIdx, CLWBuffer<cl_int> predicate, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, CLWBuffer<cl_int> newSize);
    CLWEvent Copy(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output);

    void ReclaimDeviceMemory();

protected:
    CLWEvent ScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output);
    CLWEvent SegmentedScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output);
    
    CLWEvent ScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output);
    CLWEvent SegmentedScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output);
    
    CLWEvent ScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output);
    CLWEvent SegmentedScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output);

    CLWEvent SegmentedScanExclusiveAddFourLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output);

    CLWEvent ScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output);
    CLWEvent ScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output);
    CLWEvent ScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output);

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