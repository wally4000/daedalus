// SCPUState.h

#ifndef SCPUSTATE_H_
#define SCPUSTATE_H_

#include <array>
#include "Core/R4300OpCode.h"
#include "System/SpinLock.h"
#include "Core/TLB.h"

using register_32x64 = REG64[32];
using register_16x64 = REG64[16];
using register_32x32 = std::array<REG32, 32>;

enum ECPUEventType
{
    CPU_EVENT_VBL = 1,
    CPU_EVENT_COMPARE,
    CPU_EVENT_AUDIO,
    CPU_EVENT_SPINT,
};

#define MAX_CPU_EVENTS 4

struct CPUEvent
{
    s32 mCount;
    ECPUEventType mEventType;
};

class alignas(CACHE_ALIGN) SCPUState
{
public:
    register_32x64 CPU;
    register_32x32 CPUControl;
    union
    {
        register_32x32 FPU;
        register_16x64 FPUD;
    };
    register_32x32 FPUControl;
    u32 CurrentPC;
    u32 TargetPC;
    u32 Delay;
    volatile u32 StuffToDo;

    REG64 MultLo;
    REG64 MultHi;

    REG32 Temp1;
    REG32 Temp2;
    REG32 Temp3;
    REG32 Temp4;

    std::array<CPUEvent, MAX_CPU_EVENTS> Events;
    u32 NumEvents;

    void AddJob(u32 job);
    void ClearJob(u32 job);
    u32 GetStuffToDo() const { return StuffToDo; }
    bool IsJobSet(u32 job) const { return (StuffToDo & job) != 0; }
    void ClearStuffToDo();
#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
    void Dump();
#endif
};

#endif // SCPUSTATE_H_
