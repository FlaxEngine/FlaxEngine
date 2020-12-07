// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Core/NonCopyable.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Scripting/ScriptingType.h"

#if COMPILE_WITH_PROFILER

// Profiler events buffers capacity (tweaked manually)
#define PROFILER_CPU_EVENTS_FRAMES 10
#define PROFILER_CPU_EVENTS_PER_FRAME 1000

/// <summary>
/// Provides CPU performance measuring methods.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API ProfilerCPU
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(ProfilerCPU);
public:

    /// <summary>
    /// Represents single CPU profiling event data.
    /// </summary>
    API_STRUCT() struct Event
    {
    DECLARE_SCRIPTING_TYPE_MINIMAL(Event);

        /// <summary>
        /// The start time (in milliseconds).
        /// </summary>
        API_FIELD() double Start;

        /// <summary>
        /// The end time (in milliseconds).
        /// </summary>
        API_FIELD() double End;

        /// <summary>
        /// The event depth. Value 0 is used for the root event.
        /// </summary>
        API_FIELD() int32 Depth;

        /// <summary>
        /// The native dynamic memory allocation size during this event (excluding the child events). Given value is in bytes.
        /// </summary>
        API_FIELD() int32 NativeMemoryAllocation;

        /// <summary>
        /// The managed memory allocation size during this event (excluding the child events). Given value is in bytes.
        /// </summary>
        API_FIELD() int32 ManagedMemoryAllocation;

        /// <summary>
        /// The name of the event.
        /// </summary>
        API_FIELD() const Char* Name;
    };

    /// <summary>
    /// Implements simple profiling events ring-buffer.
    /// </summary>
    class EventBuffer : public NonCopyable
    {
    private:

        Event* _data;
        int32 _capacity;
        int32 _capacityMask;
        int32 _head;
        int32 _count;

    public:

        EventBuffer()
        {
            _capacity = Math::RoundUpToPowerOf2(PROFILER_CPU_EVENTS_FRAMES * PROFILER_CPU_EVENTS_PER_FRAME);
            _capacityMask = _capacity - 1;
            _data = NewArray<Event>(_capacity);
            _head = 0;
            _count = 0;
        }

        ~EventBuffer()
        {
            DeleteArray(_data, _capacity);
        }

    public:

        /// <summary>
        /// Gets the amount of the events in the buffer.
        /// </summary>
        /// <returns>The events count.</returns>
        FORCE_INLINE int32 GetCount() const
        {
            return _count;
        }

        /// <summary>
        /// Gets the event at the specified index.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The event</returns>
        Event& Get(int32 index) const
        {
            ASSERT(index >= 0 && index < _capacity);
            return _data[index];
        }

        /// <summary>
        /// Adds new event to the buffer.
        /// </summary>
        /// <returns>The event index.</returns>
        int32 Add()
        {
            const int32 index = _head;
            _head = (_head + 1) & _capacityMask;
            _count = Math::Min(_count + 1, _capacity);
            return index;
        }

        /// <summary>
        /// Extracts the buffer data (only ended events starting from the root level with depth=0).
        /// </summary>
        /// <param name="data">The output data.</param>
        /// <param name="withRemove">True if also remove extracted events to prevent double-gather, false if don't modify the buffer data.</param>
        void Extract(Array<Event, HeapAllocation>& data, bool withRemove);

    public:

        /// <summary>
        /// Ring buffer iterator
        /// </summary>
        struct Iterator
        {
            friend EventBuffer;

        private:

            EventBuffer* _buffer;
            int32 _index;

            Iterator(EventBuffer* buffer, const int32 index)
                : _buffer(buffer)
                , _index(index)
            {
            }

            Iterator(const Iterator& i) = default;

        public:

            FORCE_INLINE int32 Index() const
            {
                return _index;
            }

            FORCE_INLINE Event& Event() const
            {
                ASSERT(_buffer && _index >= 0 && _index < _buffer->_capacity);
                return _buffer->Get(_index);
            }

        public:

            /// <summary>
            /// Checks if iterator is in the end of the collection
            /// </summary>
            /// <returns>True if is in the end, otherwise false</returns>
            bool IsEnd() const
            {
                ASSERT(_buffer);
                return _index == _buffer->_head;
            }

            /// <summary>
            /// Checks if iterator is not in the end of the collection
            /// </summary>
            /// <returns>True if is not in the end, otherwise false</returns>
            bool IsNotEnd() const
            {
                ASSERT(_buffer);
                return _index != _buffer->_head;
            }

        public:

            FORCE_INLINE bool operator==(const Iterator& v) const
            {
                return _buffer == v._buffer && _index == v._index;
            }

            FORCE_INLINE bool operator!=(const Iterator& v) const
            {
                return _buffer != v._buffer || _index != v._index;
            }

        public:

            Iterator& operator++()
            {
                ASSERT(_buffer);
                _index = (_index + 1) & _buffer->_capacityMask;
                return *this;
            }

            Iterator operator++(int)
            {
                ASSERT(_buffer);
                Iterator temp = *this;
                _index = (_index + 1) & _buffer->_capacityMask;
                return temp;
            }

            Iterator& operator--()
            {
                ASSERT(_buffer);
                _index = (_index - 1) & _buffer->_capacityMask;
                return *this;
            }

            Iterator operator--(int)
            {
                ASSERT(_buffer);
                Iterator temp = *this;
                _index = (_index - 1) & _buffer->_capacityMask;
                return temp;
            }
        };

    public:

        FORCE_INLINE Iterator Begin()
        {
            return Iterator(this, (_head - _count) & _capacityMask);
        }

        FORCE_INLINE Iterator Last()
        {
            ASSERT(_count > 0);
            return Iterator(this, (_head - 1) & _capacityMask);
        }

        FORCE_INLINE Iterator End()
        {
            return Iterator(this, _head);
        }
    };

    /// <summary>
    /// Thread registered for profiling. Holds events data with read/write support.
    /// </summary>
    class Thread
    {
    private:

        String _name;
        int32 _depth = 0;

    public:

        Thread(const Char* name)
        {
            _name = name;
        }

        Thread(const String& name)
        {
            _name = name;
        }

    public:

        /// <summary>
        /// The current thread.
        /// </summary>
        static THREADLOCAL Thread* Current;

    public:

        /// <summary>
        /// Gets the name.
        /// </summary>
        /// <returns>The name.</returns>
        FORCE_INLINE const String& GetName() const
        {
            return _name;
        }

        /// <summary>
        /// The events buffer.
        /// </summary>
        EventBuffer Buffer;

    public:

        /// <summary>
        /// Begins the event running on a this thread. Call EndEvent with index parameter equal to the returned value by BeginEvent function.
        /// </summary>
        /// <param name="name">The event name.</param>
        /// <returns>The event token.</returns>
        int32 BeginEvent(const Char* name);

        /// <summary>
        /// Ends the event running on a this thread.
        /// </summary>
        /// <param name="index">The event index returned by the BeginEvent method.</param>
        void EndEvent(int32 index);
    };

public:

    /// <summary>
    /// The registered threads.
    /// </summary>
    static Array<Thread*> Threads;

    /// <summary>
    /// The profiling tools usage flag. Can be used to disable profiler. Engine turns it down before the exit and before platform startup.
    /// </summary>
    static bool Enabled;

public:

    /// <summary>
    /// Determines whether the current (calling) thread is being profiled by the service (it may has no active profile block but is registered).
    /// </summary>
    /// <returns><c>true</c> if service is profiling the current thread; otherwise, <c>false</c>.</returns>
    static bool IsProfilingCurrentThread();

    /// <summary>
    /// Gets the current thread (profiler service shadow object).
    /// </summary>
    /// <returns>The current thread object or null if not profiled yet.</returns>
    static Thread* GetCurrentThread();

    /// <summary>
    /// Begins the event. Call EndEvent with index parameter equal to the returned value by BeginEvent function.
    /// </summary>
    /// <param name="name">The event name.</param>
    /// <returns>The event token.</returns>
    static int32 BeginEvent(const Char* name);

    /// <summary>
    /// Ends the event.
    /// </summary>
    /// <param name="index">The event index returned by the BeginEvent method.</param>
    static void EndEvent(int32 index);

    /// <summary>
    /// Releases resources. Calls to the profiling API after Dispose are not valid
    /// </summary>
    static void Dispose();
};

/// <summary>
/// Helper structure used to call BeginEvent/EndEvent within single code block.
/// </summary>
struct ScopeProfileBlockCPU
{
    /// <summary>
    /// The event token index.
    /// </summary>
    int32 Index;

    /// <summary>
    /// Initializes a new instance of the <see cref="ScopeProfileBlockCPU"/> struct.
    /// </summary>
    /// <param name="name">The event name.</param>
    ScopeProfileBlockCPU(const Char* name)
    {
        Index = ProfilerCPU::BeginEvent(name);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="ScopeProfileBlockCPU"/> class.
    /// </summary>
    ~ScopeProfileBlockCPU()
    {
        ProfilerCPU::EndEvent(Index);
    }
};

template<>
struct TIsPODType<ProfilerCPU::Event>
{
    enum { Value = true };
};

// Shortcut macros for profiling a single code block execution on CPU
#define PROFILE_CPU_NAMED(name) ScopeProfileBlockCPU ProfileBlockCPU(TEXT(name))

#if defined(_MSC_VER)
#define PROFILE_CPU() ScopeProfileBlockCPU ProfileBlockCPU(TEXT(__FUNCTION__))
#else
#define PROFILE_CPU() \
	const char* _functionName = __FUNCTION__; \
	const int32 _functionNameLength = ARRAY_COUNT(__FUNCTION__); \
	Char _functionNameBuffer[_functionNameLength + 1]; \
	StringUtils::ConvertANSI2UTF16(_functionName, _functionNameBuffer, _functionNameLength); \
	_functionNameBuffer[_functionNameLength] = 0; \
	ScopeProfileBlockCPU ProfileBlockCPU(_functionNameBuffer)
#endif

#else

// Empty macros for disabled profiler
#define PROFILE_CPU_NAMED(name)
#define PROFILE_CPU()

#endif
