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
#ifndef BVHSTRATEGY_H
#define BVHSTRATEGY_H

#include "calc.h"
#include "device.h"
#include "strategy.h"
#include <memory>


namespace RadeonRays
{
	class Bvh;
    
	class BvhStrategy : public Strategy
	{
	public:
		BvhStrategy(Calc::Device* device);

		void Preprocess(World const& world) override;
        
		void QueryIntersection(std::uint32_t queueidx,
                               Calc::Buffer const* rays,
                               std::uint32_t numrays,
                               Calc::Buffer* hits,
                               Calc::Event const* waitevent,
                               Calc::Event** event) const override;
        
		void QueryOcclusion(std::uint32_t queueidx,
                            Calc::Buffer const* rays,
                            std::uint32_t numrays,
                            Calc::Buffer* hits,
                            Calc::Event const* waitevent,
                            Calc::Event** event) const override;
        
		void QueryIntersection(std::uint32_t queueidx,
                               Calc::Buffer const* rays,
                               Calc::Buffer const* numrays,
                               std::uint32_t maxrays,
                               Calc::Buffer* hits,
                               Calc::Event const* waitevent,
                               Calc::Event** event) const override;
        
        void QueryOcclusion(std::uint32_t queueidx,
                            Calc::Buffer const* rays,
                            Calc::Buffer const* numrays,
                            std::uint32_t maxrays,
                            Calc::Buffer* hits,
                            Calc::Event const* waitevent,
                            Calc::Event** event) const override;

	private:
		struct GpuData;
		struct ShapeData;

		// Implementation data
		std::unique_ptr<GpuData> m_gpudata;
		// Bvh data structure
		std::unique_ptr<Bvh> m_bvh;
	};
}



#endif // BVHSTRATEGY_H
