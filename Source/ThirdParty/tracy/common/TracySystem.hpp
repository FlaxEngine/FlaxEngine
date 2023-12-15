#ifndef __TRACYSYSTEM_HPP__
#define __TRACYSYSTEM_HPP__

#include <stdint.h>

// Tracy -> Flax integration:
// - use LZ4 from Flax
// - use engine symbols export
// - use engine types and macros
// - remove AddVectoredExceptionHandler from win32 to prevent messing with Flax crashes reporting
// - hide implementation from includers to reduce compilation overhead
// - optimize includes (faster compilation)
// - remove some features (colors, frame image, dx1 compression)
#include "Engine/Core/Types/BaseTypes.h"
#define TRACY_API FLAXENGINE_API
#define tracy_force_inline FORCE_INLINE
#define tracy_no_inline FORCE_NOINLINE

#ifndef TracyConcat
#  define TracyConcat(x,y) TracyConcatIndirect(x,y)
#endif
#ifndef TracyConcatIndirect
#  define TracyConcatIndirect(x,y) x##y
#endif

namespace tracy
{
typedef void(*ParameterCallback)( void* data, uint32_t idx, int32_t val );

struct TRACY_API SourceLocationData
{
    const char* name;
    const char* function;
    const char* file;
    uint32_t line;
    uint32_t color;
};

class TRACY_API ScopedZone
{
public:
    static bool Begin( const SourceLocationData* srcloc );
    static void End();

    ScopedZone( const ScopedZone& ) = delete;
    ScopedZone( ScopedZone&& ) = delete;
    ScopedZone& operator=( const ScopedZone& ) = delete;
    ScopedZone& operator=( ScopedZone&& ) = delete;

    ScopedZone( const SourceLocationData* srcloc, bool is_active = true );
    ScopedZone( const SourceLocationData* srcloc, int depth, bool is_active = true );
    ScopedZone( uint32_t line, const char* source, size_t sourceSz, const char* function, size_t functionSz, const char* name, size_t nameSz, bool is_active = true );
    ScopedZone( uint32_t line, const char* source, size_t sourceSz, const char* function, size_t functionSz, const char* name, size_t nameSz, int depth, bool is_active = true );

    ~ScopedZone();

    void Text( const char* txt, size_t size );
    void Text( const Char* txt, size_t size );
    void Name( const char* txt, size_t size );
    void Name( const Char* txt, size_t size );
    void Color( uint32_t color );
    void Value( uint64_t value );

private:
    const bool m_active;

#ifdef TRACY_ON_DEMAND
    uint64_t m_connectionId;
#endif
};

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

TRACY_API const char* GetEnvVar(const char* name);

}

#endif
