#ifndef __TRACYSYSTEM_HPP__
#define __TRACYSYSTEM_HPP__

#include <stdint.h>

#include "TracyApi.h"

// Tracy -> Flax integration:
// - use LZ4 from Flax
// - use engine symbols export
// - use engine types and macros
// - remove AddVectoredExceptionHandler from win32 to prevent messing with Flax crashes reporting
// - hide implementation from includers to reduce compilation overhead
// - optimize includes (faster compilation)
// - remove some features (colors, frame image, dx1 compression)
// - tracy 0.10.0 has removed ZoneScope::Begin/End, we have added them back as they are in need

namespace tracy
{

namespace detail
{
TRACY_API uint32_t GetThreadHandleImpl();
}

#ifdef TRACY_ENABLE
TRACY_API uint32_t GetThreadHandle();
#else
static inline uint32_t GetThreadHandle()
{
    return detail::GetThreadHandleImpl();
}
#endif

TRACY_API void SetThreadName( const char* name );
TRACY_API const char* GetThreadName( uint32_t id );

TRACY_API const char* GetEnvVar( const char* name );

}

#endif
