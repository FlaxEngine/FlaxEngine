#ifndef __TRACYSCOPED_HPP__
#define __TRACYSCOPED_HPP__

#include <limits>
#include <stdint.h>
#include <string.h>

#include "../common/TracySystem.hpp"
#include "../common/TracyAlign.hpp"
#include "../common/TracyAlloc.hpp"
#include "TracyProfiler.hpp"

namespace tracy
{
void ScopedZone::Begin(const SourceLocationData* srcloc)
{
#ifdef TRACY_ON_DEMAND
    if (!GetProfiler().IsConnected()) return;
#endif
    TracyLfqPrepare( QueueType::ZoneBegin );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, (uint64_t)srcloc );
    TracyLfqCommit;
}

void ScopedZone::Begin(uint32_t line, const char* source, size_t sourceSz, const char* function, size_t functionSz, const Char* name, size_t nameSz)
{
#ifdef TRACY_ON_DEMAND
    if (!GetProfiler().IsConnected()) return;
#endif
    TracyLfqPrepare( QueueType::ZoneBeginAllocSrcLoc );
    const auto srcloc = Profiler::AllocSourceLocation( line, source, sourceSz, function, functionSz, name, nameSz );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, srcloc );
    TracyLfqCommit;
}

void ScopedZone::End()
{
#ifdef TRACY_ON_DEMAND
    if (!GetProfiler().IsConnected()) return;
#endif
    TracyLfqPrepare( QueueType::ZoneEnd );
    MemWrite( &item->zoneEnd.time, Profiler::GetTime() );
    TracyLfqCommit;
}

ScopedZone::ScopedZone( const SourceLocationData* srcloc, bool is_active )
#ifdef TRACY_ON_DEMAND
    : m_active( is_active && GetProfiler().IsConnected() )
#else
    : m_active( is_active )
#endif
{
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    m_connectionId = GetProfiler().ConnectionId();
#endif
    TracyLfqPrepare( QueueType::ZoneBegin );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, (uint64_t)srcloc );
    TracyLfqCommit;
}

ScopedZone::ScopedZone( const SourceLocationData* srcloc, int depth, bool is_active )
#ifdef TRACY_ON_DEMAND
    : m_active( is_active && GetProfiler().IsConnected() )
#else
    : m_active( is_active )
#endif
{
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    m_connectionId = GetProfiler().ConnectionId();
#endif
    GetProfiler().SendCallstack( depth );

    TracyLfqPrepare( QueueType::ZoneBeginCallstack );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, (uint64_t)srcloc );
    TracyLfqCommit;
}

ScopedZone::ScopedZone( uint32_t line, const char* source, size_t sourceSz, const char* function, size_t functionSz, const char* name, size_t nameSz, bool is_active )
#ifdef TRACY_ON_DEMAND
    : m_active( is_active && GetProfiler().IsConnected() )
#else
    : m_active( is_active )
#endif
{
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    m_connectionId = GetProfiler().ConnectionId();
#endif
    TracyLfqPrepare( QueueType::ZoneBeginAllocSrcLoc );
    const auto srcloc = Profiler::AllocSourceLocation( line, source, sourceSz, function, functionSz, name, nameSz );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, srcloc );
    TracyLfqCommit;
}

ScopedZone::ScopedZone( uint32_t line, const char* source, size_t sourceSz, const char* function, size_t functionSz, const char* name, size_t nameSz, int depth, bool is_active )
#ifdef TRACY_ON_DEMAND
    : m_active( is_active && GetProfiler().IsConnected() )
#else
    : m_active( is_active )
#endif
{
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    m_connectionId = GetProfiler().ConnectionId();
#endif
    GetProfiler().SendCallstack( depth );

    TracyLfqPrepare( QueueType::ZoneBeginAllocSrcLocCallstack );
    const auto srcloc = Profiler::AllocSourceLocation( line, source, sourceSz, function, functionSz, name, nameSz );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, srcloc );
    TracyLfqCommit;
}

ScopedZone::~ScopedZone()
{
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    TracyLfqPrepare( QueueType::ZoneEnd );
    MemWrite( &item->zoneEnd.time, Profiler::GetTime() );
    TracyLfqCommit;
}

void ScopedZone::Text( const char* txt, size_t size )
{
    assert( size < std::numeric_limits<uint16_t>::max() );
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    auto ptr = (char*)tracy_malloc( size );
    memcpy( ptr, txt, size );
    TracyLfqPrepare( QueueType::ZoneText );
    MemWrite( &item->zoneTextFat.text, (uint64_t)ptr );
    MemWrite( &item->zoneTextFat.size, (uint16_t)size );
    TracyLfqCommit;
}

void ScopedZone::Text(const Char* txt, size_t size)
{
    assert( size < std::numeric_limits<uint16_t>::max() );
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    auto ptr = (char*)tracy_malloc( size );
    for( int i = 0; i < size; i++)
        ptr[i] = (char)txt[i];
    TracyLfqPrepare( QueueType::ZoneText );
    MemWrite( &item->zoneTextFat.text, (uint64_t)ptr );
    MemWrite( &item->zoneTextFat.size, (uint16_t)size );
    TracyLfqCommit;
}

void ScopedZone::Name( const char* txt, size_t size )
{
    assert( size < std::numeric_limits<uint16_t>::max() );
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    auto ptr = (char*)tracy_malloc( size );
    memcpy( ptr, txt, size );
    TracyLfqPrepare( QueueType::ZoneName );
    MemWrite( &item->zoneTextFat.text, (uint64_t)ptr );
    MemWrite( &item->zoneTextFat.size, (uint16_t)size );
    TracyLfqCommit;
}

void ScopedZone::Name( const Char* txt, size_t size )
{
    assert( size < std::numeric_limits<uint16_t>::max() );
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    auto ptr = (char*)tracy_malloc( size );
    for( int i = 0; i < size; i++)
        ptr[i] = (char)txt[i];
    TracyLfqPrepare( QueueType::ZoneName );
    MemWrite( &item->zoneTextFat.text, (uint64_t)ptr );
    MemWrite( &item->zoneTextFat.size, (uint16_t)size );
    TracyLfqCommit;
}

void ScopedZone::Color( uint32_t color )
{
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    TracyLfqPrepare( QueueType::ZoneColor );
    MemWrite( &item->zoneColor.r, uint8_t( ( color       ) & 0xFF ) );
    MemWrite( &item->zoneColor.g, uint8_t( ( color >> 8  ) & 0xFF ) );
    MemWrite( &item->zoneColor.b, uint8_t( ( color >> 16 ) & 0xFF ) );
    TracyLfqCommit;
}

void ScopedZone::Value( uint64_t value )
{
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    TracyLfqPrepare( QueueType::ZoneValue );
    MemWrite( &item->zoneValue.value, value );
    TracyLfqCommit;
}

bool ScopedZone::IsActive() const { return m_active; }

}

#endif
