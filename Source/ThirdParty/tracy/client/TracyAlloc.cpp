#include "../common/TracyAlloc.hpp"

#ifdef TRACY_USE_RPMALLOC

#include <atomic>

#include "../common/TracyYield.hpp"

#if PLATFORM_WINDOWS
extern void CheckInstructionSet();
#endif

namespace tracy
{

extern thread_local bool RpThreadInitDone;
extern std::atomic<int> RpInitDone;
extern std::atomic<int> RpInitLock;

tracy_no_inline static void InitRpmallocPlumbing()
{
    const auto done = RpInitDone.load( std::memory_order_acquire );
    if( !done )
    {
#if PLATFORM_WINDOWS
        // Check instruction set before executing any code (Tracy init static vars before others)
        CheckInstructionSet();
#endif
        int expected = 0;
        while( !RpInitLock.compare_exchange_weak( expected, 1, std::memory_order_release, std::memory_order_relaxed ) ) { expected = 0; YieldThread(); }
        const auto done = RpInitDone.load( std::memory_order_acquire );
        if( !done )
        {
            rpmalloc_initialize();
            RpInitDone.store( 1, std::memory_order_release );
        }
        RpInitLock.store( 0, std::memory_order_release );
    }
    rpmalloc_thread_initialize();
    RpThreadInitDone = true;
}

TRACY_API void InitRpmalloc()
{
    if( !RpThreadInitDone ) InitRpmallocPlumbing();
}

}

#endif
