#ifndef __TRACYSCOPED_HPP__
#define __TRACYSCOPED_HPP__

#include <limits>
#include <stdint.h>
#include <string.h>

#include "../common/TracySystem.hpp"
#include "../common/TracyAlign.hpp"
#include "../common/TracyAlloc.hpp"
#include "../client/TracyLock.hpp"

namespace tracy
{
bool ScopedZone::Begin(const SourceLocationData* srcloc)
{
#ifdef TRACY_ON_DEMAND
    if (!GetProfiler().IsConnected()) return false;
#endif
    TracyLfqPrepare( QueueType::ZoneBegin );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, (uint64_t)srcloc );
    TracyQueueCommit( zoneBeginThread );
    return true;
}

void ScopedZone::End()
{
#ifdef TRACY_ON_DEMAND
    if (!GetProfiler().IsConnected()) return;
#endif
    TracyLfqPrepare( QueueType::ZoneEnd );
    MemWrite( &item->zoneEnd.time, Profiler::GetTime() );
    TracyQueueCommit( zoneEndThread );
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
    TracyQueuePrepare( QueueType::ZoneBegin );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, (uint64_t)srcloc );
    TracyQueueCommit( zoneBeginThread );
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

    TracyQueuePrepare( QueueType::ZoneBeginCallstack );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, (uint64_t)srcloc );
    TracyQueueCommit( zoneBeginThread );
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
    TracyQueuePrepare( QueueType::ZoneBeginAllocSrcLoc );
    const auto srcloc = Profiler::AllocSourceLocation( line, source, sourceSz, function, functionSz, name, nameSz );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, srcloc );
    TracyQueueCommit( zoneBeginThread );
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

    TracyQueuePrepare( QueueType::ZoneBeginAllocSrcLocCallstack );
    const auto srcloc = Profiler::AllocSourceLocation( line, source, sourceSz, function, functionSz, name, nameSz );
    MemWrite( &item->zoneBegin.time, Profiler::GetTime() );
    MemWrite( &item->zoneBegin.srcloc, srcloc );
    TracyQueueCommit( zoneBeginThread );
}

ScopedZone::~ScopedZone()
{
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    TracyQueuePrepare( QueueType::ZoneEnd );
    MemWrite( &item->zoneEnd.time, Profiler::GetTime() );
    TracyQueueCommit( zoneEndThread );
}

void ScopedZone::Text( const char* txt, size_t size )
{
    assert( size < (std::numeric_limits<uint16_t>::max)() );
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    auto ptr = (char*)tracy_malloc( size );
    memcpy( ptr, txt, size );
    TracyQueuePrepare( QueueType::ZoneText );
    MemWrite( &item->zoneTextFat.text, (uint64_t)ptr );
    MemWrite( &item->zoneTextFat.size, (uint16_t)size );
    TracyQueueCommit( zoneTextFatThread );
}

void ScopedZone::Text( const Char* txt, size_t size )
{
    assert( size < (std::numeric_limits<uint16_t>::max)() );
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    auto ptr = (char*)tracy_malloc( size );
    for( int i = 0; i < size; i++)
        ptr[i] = (char)txt[i];
    TracyQueuePrepare( QueueType::ZoneText );
    MemWrite( &item->zoneTextFat.text, (uint64_t)ptr );
    MemWrite( &item->zoneTextFat.size, (uint16_t)size );
    TracyQueueCommit( zoneTextFatThread );
}

void ScopedZone::Name( const char* txt, size_t size )
{
    assert( size < (std::numeric_limits<uint16_t>::max)() );
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    auto ptr = (char*)tracy_malloc( size );
    memcpy( ptr, txt, size );
    TracyQueuePrepare( QueueType::ZoneName );
    MemWrite( &item->zoneTextFat.text, (uint64_t)ptr );
    MemWrite( &item->zoneTextFat.size, (uint16_t)size );
    TracyQueueCommit( zoneTextFatThread );
}

void ScopedZone::Name( const Char* txt, size_t size )
{
    assert( size < (std::numeric_limits<uint16_t>::max)() );
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    auto ptr = (char*)tracy_malloc( size );
    for( int i = 0; i < size; i++)
        ptr[i] = (char)txt[i];
    TracyQueuePrepare( QueueType::ZoneName );
    MemWrite( &item->zoneTextFat.text, (uint64_t)ptr );
    MemWrite( &item->zoneTextFat.size, (uint16_t)size );
    TracyQueueCommit( zoneTextFatThread );
}

void ScopedZone::Color( uint32_t color )
{
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    TracyQueuePrepare( QueueType::ZoneColor );
    MemWrite( &item->zoneColor.b, uint8_t( ( color       ) & 0xFF ) );
    MemWrite( &item->zoneColor.g, uint8_t( ( color >> 8  ) & 0xFF ) );
    MemWrite( &item->zoneColor.r, uint8_t( ( color >> 16 ) & 0xFF ) );
    TracyQueueCommit( zoneColorThread );
}

void ScopedZone::Value( uint64_t value )
{
    if( !m_active ) return;
#ifdef TRACY_ON_DEMAND
    if( GetProfiler().ConnectionId() != m_connectionId ) return;
#endif
    TracyQueuePrepare( QueueType::ZoneValue );
    MemWrite( &item->zoneValue.value, value );
    TracyQueueCommit( zoneValueThread );
}
}

#endif
