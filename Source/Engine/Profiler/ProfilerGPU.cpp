// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PROFILER

#include "ProfilerGPU.h"
#include "ProfilerMemory.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Memory/ArenaAllocation.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUContext.h"

RenderStatsData RenderStatsData::Counter;

int32 ProfilerGPU::_depth = 0;
bool ProfilerGPU::Enabled = false;
bool ProfilerGPU::EventsEnabled = false;
int32 ProfilerGPU::CurrentBuffer = 0;
ProfilerGPU::EventBuffer ProfilerGPU::Buffers[PROFILER_GPU_EVENTS_FRAMES];

bool ProfilerGPU::EventBuffer::HasData() const
{
    return _isResolved && _data.HasItems();
}

void ProfilerGPU::EventBuffer::EndAllQueries()
{
    auto context = GPUDevice::Instance->GetMainContext();
    auto queries = _data.Get();
    for (int32 i = 0; i < _data.Count(); i++)
    {
        auto& e = queries[i];
        if (e.QueryActive)
        {
            e.QueryActive = false;
            context->EndQuery(e.Query);
        }
    }
}

void ProfilerGPU::EventBuffer::TryResolve()
{
    if (_isResolved || _data.IsEmpty())
        return;

    // Collect queries results
    PROFILE_MEM(Profiler);
    auto device = GPUDevice::Instance;
    auto queries = _data.Get();
    for (int32 i = 0; i < _data.Count(); i++)
    {
        auto& e = queries[i];
        ASSERT_LOW_LAYER(!e.QueryActive);
        uint64 time;
        if (device->GetQueryResult(e.Query, time, false))
        {
            e.Time = (float)time * 0.001f; // Convert to milliseconds
        }
        else
            return; // Skip if one of the queries is not yet ready (frame still in-flight)
    }

    _isResolved = true;
}

int32 ProfilerGPU::EventBuffer::Add(const Event& e)
{
    PROFILE_MEM(Profiler);
    const int32 index = _data.Count();
    _data.Add(e);
    return index;
}

void ProfilerGPU::EventBuffer::Extract(Array<Event>& data) const
{
    // Don't use unresolved data
    ASSERT(_isResolved);
    data = _data;
}

void ProfilerGPU::EventBuffer::Clear()
{
    _data.Clear();
    _isResolved = false;
    FrameIndex = 0;
    PresentTime = 0.0f;
}

int32 ProfilerGPU::BeginEvent(const Char* name)
{
    auto context = GPUDevice::Instance->GetMainContext();
#if GPU_ALLOW_PROFILE_EVENTS
    if (EventsEnabled)
        context->EventBegin(name);
#endif
    if (!Enabled)
        return -1;

    Event e;
    e.Name = name;
    e.Stats = RenderStatsData::Counter;
    e.Query = context->BeginQuery(GPUQueryType::Timer);
    e.Depth = _depth++;
    e.QueryActive = true;

    auto& buffer = Buffers[CurrentBuffer];
    const auto index = buffer.Add(e);
    return index;
}

void ProfilerGPU::EndEvent(int32 index)
{
    auto context = GPUDevice::Instance->GetMainContext();
#if GPU_ALLOW_PROFILE_EVENTS
    if (EventsEnabled)
        context->EventEnd();
#endif
    if (index == -1)
        return;
    _depth--;

    auto& buffer = Buffers[CurrentBuffer];
    auto e = buffer.Get(index);
    e->QueryActive = false;
    e->Stats.Mix(RenderStatsData::Counter);
    context->EndQuery(e->Query);
}

void ProfilerGPU::BeginFrame()
{
    // Clear stats
    RenderStatsData::Counter = RenderStatsData();
    _depth = 0;
    auto& buffer = Buffers[CurrentBuffer];
    buffer.FrameIndex = Engine::FrameCount;
    buffer.PresentTime = 0.0f;

    // Try to resolve previous frames
    for (int32 i = 0; i < PROFILER_GPU_EVENTS_FRAMES; i++)
    {
        Buffers[i].TryResolve();
    }
}

void ProfilerGPU::OnPresent()
{
    // End all current frame queries to prevent invalid event duration values
    auto& buffer = Buffers[CurrentBuffer];
    buffer.EndAllQueries();
}

void ProfilerGPU::OnPresentTime(float time)
{
    auto& buffer = Buffers[CurrentBuffer];
    buffer.PresentTime += time;
}

void ProfilerGPU::EndFrame()
{
    if (_depth)
    {
        LOG(Warning, "GPU Profiler events start/end mismatch");
    }

    // Move frame
    CurrentBuffer = (CurrentBuffer + 1) % PROFILER_GPU_EVENTS_FRAMES;

    // Prepare current frame buffer
    auto& buffer = Buffers[CurrentBuffer];
    buffer.Clear();
}

bool ProfilerGPU::GetLastFrameData(float& drawTimeMs, float& presentTimeMs, RenderStatsData& statsData)
{
    uint64 maxFrame = 0;
    int32 maxFrameIndex = -1;
    auto& frames = Buffers;
    for (uint32 i = 0; i < ARRAY_COUNT(frames); i++)
    {
        if (frames[i].HasData() && frames[i].FrameIndex > maxFrame)
        {
            maxFrame = frames[i].FrameIndex;
            maxFrameIndex = i;
        }
    }
    if (maxFrameIndex != -1)
    {
        auto& frame = frames[maxFrameIndex];
        const auto root = frame.Get(0);
        drawTimeMs = root->Time;
        presentTimeMs = frame.PresentTime;
        statsData = root->Stats;
        return true;
    }

    // No data
    drawTimeMs = 0.0f;
    presentTimeMs = 0.0f;
    Platform::MemoryClear(&statsData, sizeof(statsData));
    return false;
}

struct GraphicsDumping
{
    struct Item
    {
        StringView Name;
        StringView FullName;
        uint16 Count;
        uint16 Depth;
        float Time;
        RenderStatsData Stats;
    };

    int32 FramesLeft;
    bool WasProfilerGPUEnabled;
    uint64 NextFrame;
    ArenaAllocator Allocator;
    Array<ProfilerGPU::Event> FrameData;
    Array<Item, ArenaAllocation> Items;
    Char* NameBuffers[2];
    constexpr static int32 BufferSize = 500;

    GraphicsDumping(int32 frames);
    ~GraphicsDumping();
    void OnDraw();
    void Add(Array<ProfilerGPU::Event>& frame);
    void Print();

    static void AppendName(Char*& name, Char*& other, StringView text)
    {
        int32 nameLen = StringUtils::Length(name);
        ASSERT_LOW_LAYER(nameLen + text.Length() < BufferSize);
        Platform::MemoryCopy(other, name, nameLen * sizeof(Char));
        Platform::MemoryCopy(other + nameLen, text.Get(), text.Length() * sizeof(Char));
        other[nameLen + text.Length()] = 0;
        Swap(name, other);
    }

    static const Char* FormatValue(Char* buffer, int64 value)
    {
        // Format value with thousands separators (fmt has disabled FMT_USE_LOCALE_GROUPING)
        fmt_flax::allocator allocator;
        fmt_flax::memory_buffer fmtBuffer(allocator);
        fmt_flax::format(fmtBuffer, TEXT("{}"), value);
        const StringView str(fmtBuffer.data(), (int32)fmtBuffer.size());
        int32 step = 0;
        int32 size = str.Length() + (str.Length() - 1) / 3;
        buffer[size--] = 0;
        for (int32 i = str.Length() - 1; i >= 0; i--)
        {
            buffer[size--] = str[i];
            if (++step == 3 && i != 0)
            {
                step = 0;
                buffer[size--] = ',';
            }
        }
        return buffer;
    }
};

GraphicsDumping* Dumping = nullptr;

GraphicsDumping::GraphicsDumping(int32 frames)
    : Allocator(16 * 1024) // 16kB
    , Items(&Allocator)
{
    NameBuffers[0] = (Char*)Allocator.Allocate(BufferSize * sizeof(Char));
    NameBuffers[1] = (Char*)Allocator.Allocate(BufferSize * sizeof(Char));
    FramesLeft = frames;
    NextFrame = Engine::FrameCount + 1; // Start from the next frame
    WasProfilerGPUEnabled = ProfilerGPU::Enabled;
    ProfilerGPU::Enabled = true;
    Engine::Draw.Bind<GraphicsDumping, &GraphicsDumping::OnDraw>(this);
}

GraphicsDumping::~GraphicsDumping()
{
    Engine::Draw.Unbind<GraphicsDumping, &GraphicsDumping::OnDraw>(this);
    ProfilerGPU::Enabled = WasProfilerGPUEnabled;
}

void GraphicsDumping::OnDraw()
{
    PROFILE_MEM(Profiler);

    // Process only frames in the profiling range that have data
    for (auto& frame : ProfilerGPU::Buffers)
    {
        if (frame.FrameIndex == NextFrame && frame.HasData())
        {
            // Extract events from the frame
            FrameData.Clear();
            frame.Extract(FrameData);

            // Put events into the current items hierarchy
            Add(FrameData);

            // Move to the next frame
            NextFrame++;
            FramesLeft--;
            if (FramesLeft == 0)
            {
                // Done!
                Print();
                Delete(Dumping);
                Dumping = nullptr;
                return;
            }
        }
    }
}

void GraphicsDumping::Add(Array<ProfilerGPU::Event>& events)
{
    if (Items.IsEmpty())
        Items.EnsureCapacity(events.Count());
    for (int32 i = 0; i < events.Count(); i++)
    {
        auto& e = events[i];

        // Build full name of the event (used to merge events from different frames)
        auto nameBuffer = NameBuffers[0];
        auto otherBuffer = NameBuffers[1];
        nameBuffer[0] = otherBuffer[0] = 0;
        AppendName(nameBuffer, otherBuffer, e.Name);
        for (int32 depth = e.Depth; depth != 0; depth--)
        {
            // Find parent event
            for (int32 j = i - 1; j >= 0; j--)
            {
                if (events[j].Depth == depth - 1)
                {
                    auto& parent = events[j];
                    AppendName(nameBuffer, otherBuffer, StringView(TEXT("/"), 1));
                    AppendName(nameBuffer, otherBuffer, parent.Name);
                    break;
                }
            }
        }
        StringView name(nameBuffer);

        // Find that item in the list
        int32 itemIndex = 0;
        for (; itemIndex < Items.Count(); itemIndex++)
        {
            if (Items[itemIndex].FullName == name)
                break;
        }
        if (itemIndex == Items.Count())
        {
            // Add a new item
            auto& newItem = Items.AddOne();
            newItem.Name = e.Name;
            newItem.Count = 0;
            newItem.Depth = (uint16)e.Depth;
            newItem.Time = 0.0f;
            newItem.Stats = RenderStatsData();
            auto nameLen = (name.Length() + 1) * sizeof(Char);
            auto nameMem = (Char*)Allocator.Allocate(nameLen);
            Platform::MemoryCopy(nameMem, name.Get(), nameLen);
            newItem.FullName = StringView(nameMem, name.Length());
        }

        // Insert event data into the item
        auto& item = Items[itemIndex];
        item.Count++;
        item.Time += e.Time;
        item.Stats += e.Stats;
    }
}

void GraphicsDumping::Print()
{
    if (Items.IsEmpty())
    {
        LOG(Warning, "No drawing found");
        return;
    }

    // Average results
    for (auto& item : Items)
    {
        item.Time /= (float)item.Count;
        item.Stats *= 1.0f / (float)item.Count;
    }

    // Print profiling hierarchy
    StringBuilder sb;
    sb.AppendLine(TEXT("GPU profiler summary:"));
    auto& draw = Items[0];
    {
        // The root item is always the drawing by engine
        if (draw.Count == 1)
            sb.AppendFormat(TEXT("  Frame time: {} ms ({} FPS)"), Utilities::RoundTo2DecimalPlaces(draw.Time), (int32)(1000.0f / draw.Time)).AppendLine();
        else
            sb.AppendFormat(TEXT("  Frame time: {} ms ({} FPS), avg of {} frames"), Utilities::RoundTo2DecimalPlaces(draw.Time), (int32)(1000.0f / draw.Time), draw.Count).AppendLine();
        sb.AppendFormat(TEXT("  Draws: {}, Dispatches: {}"), draw.Stats.DrawCalls, draw.Stats.DispatchCalls).AppendLine();
        sb.AppendFormat(TEXT("  Triangles: {}, Vertices: {}, PSO changes: {}"), FormatValue(NameBuffers[0], draw.Stats.Triangles), FormatValue(NameBuffers[1], draw.Stats.Vertices), draw.Stats.PipelineStateChanges).AppendLine();
    }
    //sb.AppendLine(TEXT("Hierarchy:"));
    for (int32 itemIndex = 1; itemIndex < Items.Count(); itemIndex++)
    {
        auto& item = Items[itemIndex];

        // Timing and percentage of the frame
        const float percentage = item.Time * 100.0f / draw.Time;
        sb.AppendFormat(TEXT("{:>5.2f}%  {:>5.2f}ms "), Utilities::RoundTo1DecimalPlace(percentage), Utilities::RoundTo2DecimalPlaces(item.Time));

        // Indent based on depth
        for (int32 depth = 1; depth < item.Depth; depth++)
            sb.Append(TEXT("   "));

        // Event name and stats
        if (item.Stats.DrawCalls + item.Stats.DispatchCalls != 0)
        {
            sb.AppendFormat(TEXT("{}: "), item.Name);
            if (item.Stats.DispatchCalls == 0)
                if (item.Stats.DrawCalls == 1)
                    sb.Append(TEXT("1 draw"));
                else
                    sb.AppendFormat(TEXT("{} draws"), item.Stats.DrawCalls);
            else if (item.Stats.DrawCalls == 0)
                if (item.Stats.DispatchCalls == 1)
                    sb.Append(TEXT("1 dispatch"));
                else
                    sb.AppendFormat(TEXT("{} dispatches"), item.Stats.DispatchCalls);
            else
                sb.AppendFormat(TEXT("{} draws, {} dispatches"), item.Stats.DrawCalls, item.Stats.DispatchCalls);
            if (item.Stats.Triangles == 1)
                sb.AppendFormat(TEXT(", 1 tri, {} verts"), FormatValue(NameBuffers[0], item.Stats.Vertices));
            else if (item.Stats.Triangles != 0)
                sb.AppendFormat(TEXT(", {} tris, {} verts"), FormatValue(NameBuffers[0], item.Stats.Triangles), FormatValue(NameBuffers[1], item.Stats.Vertices));
        }
        else
        {
            sb.Append(item.Name);
        }
        sb.AppendLine();
    }
    LOG_STR(Info, sb.ToStringView());
}

void ProfilerGPU::Dump(int32 frames)
{
    if (Dumping)
    {
        LOG(Warning, "Cannot start new profiling while one is active");
        return;
    }
    if (frames <= 0)
        frames = 4; // Default frames count
    frames = Math::Min(frames, 100);
    PROFILE_MEM(Profiler);

    Dumping = New<GraphicsDumping>(frames);
}

void ProfilerGPU::Dispose()
{
    SAFE_DELETE(Dumping);
}

#endif
