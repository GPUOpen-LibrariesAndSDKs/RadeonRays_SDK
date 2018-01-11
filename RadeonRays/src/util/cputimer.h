#pragma once

#include <chrono>

class CpuTimer
{
public:
    CpuTimer(const char *name);
    ~CpuTimer();

private:
    const char *m_name;
    const std::chrono::time_point<std::chrono::steady_clock> m_start;
};

#define CPU_TIMED_SECTION(NAME)     \
    const CpuTimer __cpuTimer__##NAME(#NAME)
