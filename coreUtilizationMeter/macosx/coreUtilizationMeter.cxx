#include <vector>
#include <cassert>
#include <mach/mach_types.h>
#include <mach/processor_info.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <unistd.h>

class CoreUtilizationMeter {
public:
    CoreUtilizationMeter();
    void beginWindow();
    void endWindow();
    void printResults();

private:
    void gather (int sampleNum);

    size_t _processorCount;
    std::vector<int64_t> _tickArr;
};

#define tickIndex(sample,cpustate,corenum) \
    ((sample * (CPU_STATE_MAX * _processorCount)) +     \
     (cpustate * (_processorCount)) +                    \
     corenum)

CoreUtilizationMeter::CoreUtilizationMeter()
{
    int processorCount = sysconf(_SC_NPROCESSORS_ONLN);
    _processorCount = (size_t) processorCount;
    _tickArr.resize (_processorCount * CPU_STATE_MAX * 2);
}

void
CoreUtilizationMeter::gather (int sampleNum)
{
    host_t machHost = mach_host_self();
    unsigned nProcs;
    processor_cpu_load_info_t loadInfo;
    mach_msg_type_number_t msgCount;
    kern_return_t kerr = host_processor_info (machHost, PROCESSOR_CPU_LOAD_INFO,
                                              &nProcs,
                                              (processor_info_array_t *) &loadInfo,
                                              &msgCount);
    if (kerr != KERN_SUCCESS) {
        printf("host_processor_info failed (%d)\n", kerr);
        assert(0);
    }

    for (unsigned procNum = 0; procNum < _processorCount; procNum ++) {
        for (unsigned i = 0; i < CPU_STATE_MAX; i++) {
            int idx = tickIndex (sampleNum, i, procNum);
            _tickArr[idx] = loadInfo[procNum].cpu_ticks[i];
        }
    }

    for (unsigned procNum = 0; procNum < _processorCount; procNum ++) {
        for (unsigned i = 0; i < CPU_STATE_MAX; i++) {
            const char *state;
            switch (i) {
            case CPU_STATE_USER:   state = "USER"; break;
            case CPU_STATE_SYSTEM: state = "SYSTEM"; break;
            case CPU_STATE_IDLE:   state = "IDLE"; break;
            case CPU_STATE_NICE:   state = "NICE"; break;
            default:
                assert(0);
            }
                
            printf("gather proc %d state %d(%s) value %lld\n", 
                   procNum, 
                   i,
                   state,
                   (int64_t)_tickArr[tickIndex(sampleNum, i, procNum)]);
        }
    }
}

void
CoreUtilizationMeter::beginWindow()
{
    printf ("Begin core utilization window\n");
    gather (0);
}

void
CoreUtilizationMeter::endWindow()
{
    gather (1);
    printf ("End core utilization window\n");
}

void
CoreUtilizationMeter::printResults()
{
    for (size_t procNum = 0; procNum < _processorCount; procNum++) {
        int64_t beginTotalTicks = 0;
        int64_t beginIdleTicks = _tickArr[tickIndex (0, CPU_STATE_IDLE, procNum)];
        for (unsigned i = 0; i < CPU_STATE_MAX; i++) {
            beginTotalTicks += _tickArr[tickIndex (0, i, procNum)];
        }

        int64_t endTotalTicks = 0;
        int64_t endIdleTicks = _tickArr[tickIndex (1, CPU_STATE_IDLE, procNum)];
        for (unsigned i = 0; i < CPU_STATE_MAX; i++) {
            endTotalTicks += _tickArr[tickIndex (1, i, procNum)];
        }

        int64_t deltaTotalTicks = endTotalTicks - beginTotalTicks;
        int64_t deltaIdleTicks = endIdleTicks - beginIdleTicks;

        double utilization = 0;
        if (deltaTotalTicks != 0) {
            utilization = (1.0 - ((double)deltaIdleTicks / (double)deltaTotalTicks)) * 100.0;
        }
        printf ("Processor %2d utilization percentage: %4.1lf\n", (int)procNum, utilization);
    }
}

int main(int argc, char **argv)
{
    CoreUtilizationMeter m;
    m.beginWindow();
    sleep(5);
    m.endWindow();
    m.printResults();
    return 0;
}

