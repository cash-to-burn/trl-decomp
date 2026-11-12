#include "game/pc/timer.h"

static LARGE_INTEGER s_freq = {};
static uint64_t s_baseTimeUS = 0;
static uint64_t s_curTimeUS = 0;

// 0x00402520
uint64_t WIN_TimeUS(void)
{
#ifdef _WIN32
    LARGE_INTEGER counter;
    if (s_freq.QuadPart == 0) {
        QueryPerformanceFrequency(&s_freq);
    }
    QueryPerformanceCounter(&counter);
    return (uint64_t)((counter.QuadPart * 1000000ULL) / s_freq.QuadPart);
#else
    return 0;
#endif
}

// 0x00402570
void TIMER_Init(void)
{
    s_baseTimeUS = WIN_TimeUS();
    s_curTimeUS = s_baseTimeUS;
}

// 0x00402590
uint64_t TIMER_GetTimeUS(void)
{
    s_curTimeUS = WIN_TimeUS();
    return s_curTimeUS - s_baseTimeUS;
}

// 0x00402ba0
uint32_t TIMER_GetTimeMS(void)
{
    return (uint32_t)(TIMER_GetTimeUS() / 1000ULL);
}

// 0x00402610
uint32_t TIMER_TimeDiff(uint32_t start, uint32_t end)
{
    return end - start;
}
