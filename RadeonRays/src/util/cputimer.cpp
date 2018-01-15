#include "cputimer.h"

#include <assert.h>
#include <Windows.h>

CpuTimer::CpuTimer(const char *name)
    : m_name(name)
    , m_start(std::chrono::high_resolution_clock::now())
{
    assert(m_name);
}

CpuTimer::~CpuTimer()
{
    char result[1024];
    using namespace std::chrono;
    const auto elapsed = high_resolution_clock::now() - m_start;
    snprintf(result, sizeof(result), "%s: %.3fms\n", m_name, static_cast<double>(duration_cast<microseconds>(elapsed).count()) / 1000.0);
    OutputDebugStringA(result);
}
