// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Collections/Array.h"
#include "Types.h"

// The maximum amount of particle attributes to use
#define PARTICLE_ATTRIBUTES_MAX_COUNT 32

// The maximum amount of particle ribbon modules used per context
#define PARTICLE_EMITTER_MAX_RIBBONS 4

class ParticleEmitter;
class Particles;
class GPUBuffer;
class DynamicIndexBuffer;
class DynamicVertexBuffer;

/// <summary>
/// The particle attribute that defines a single particle layout component.
/// </summary>
struct ParticleAttribute
{
    enum class ValueTypes
    {
        Float,
        Float2,
        Float3,
        Float4,
        Int,
        Uint,
    };

    /// <summary>
    /// The attribute value container type.
    /// </summary>
    ValueTypes ValueType;

    /// <summary>
    /// The attribute offset from particle data start (in bytes).
    /// </summary>
    int32 Offset;

    /// <summary>
    /// The attribute name.
    /// </summary>
    String Name;

    /// <summary>
    /// Gets the size of the attribute (in bytes).
    /// </summary>
    int32 GetSize() const;
};

/// <summary>
/// The particle attributes layout descriptor.
/// </summary>
class ParticleLayout
{
public:
    /// <summary>
    /// The total particle data stride size (in bytes). Defines the required memory amount for a single particle.
    /// </summary>
    int32 Size;

    /// <summary>
    /// The attributes set.
    /// </summary>
    Array<ParticleAttribute, FixedAllocation<PARTICLE_ATTRIBUTES_MAX_COUNT>> Attributes;

public:
    /// <summary>
    /// Clears the layout data.
    /// </summary>
    void Clear();

    /// <summary>
    /// Updates the attributes layout (calculates offset) and updates the total size of the layout.
    /// </summary>
    void UpdateLayout();

    /// <summary>
    /// Finds the attribute by the name.
    /// </summary>
    /// <param name="name">The name.</param>
    /// <returns>The attribute index or -1 if cannot find it.</returns>
    int32 FindAttribute(const StringView& name) const;

    /// <summary>
    /// Finds the attribute by the name and type.
    /// </summary>
    /// <param name="name">The name.</param>
    /// <param name="valueType">The type.</param>
    /// <returns>The attribute index or -1 if cannot find it.</returns>
    int32 FindAttribute(const StringView& name, ParticleAttribute::ValueTypes valueType) const;

    /// <summary>
    /// Finds the attribute offset by the name.
    /// </summary>
    /// <param name="name">The name.</param>
    /// <param name="fallbackValue">The fallback value to return if attribute is missing.</param>
    /// <returns>The attribute offset or fallback value if cannot find it.</returns>
    int32 FindAttributeOffset(const StringView& name, int32 fallbackValue = 0) const;

    /// <summary>
    /// Finds the attribute offset by the name.
    /// </summary>
    /// <param name="name">The name.</param>
    /// <param name="valueType">The type.</param>
    /// <param name="fallbackValue">The fallback value to return if attribute is missing.</param>
    /// <returns>The attribute offset or fallback value if cannot find it.</returns>
    int32 FindAttributeOffset(const StringView& name, ParticleAttribute::ValueTypes valueType, int32 fallbackValue = 0) const;

    /// <summary>
    /// Gets the attribute offset by the attribute index.
    /// </summary>
    /// <param name="index">The attribute index.</param>
    /// <param name="fallbackValue">The fallback value to return if attribute is missing.</param>
    /// <returns>The attribute offset or fallback value if cannot find it.</returns>
    FORCE_INLINE int32 GetAttributeOffset(int32 index, int32 fallbackValue = -1) const
    {
        return index != -1 ? Attributes[index].Offset : fallbackValue;
    }

    /// <summary>
    /// Adds the attribute with the given name and value type.
    /// </summary>
    /// <param name="name">The name.</param>
    /// <param name="valueType">The value type.</param>
    /// <returns>The attribute index or -1 if cannot find it.</returns>
    int32 AddAttribute(const StringView& name, ParticleAttribute::ValueTypes valueType);
};

/// <summary>
/// The particles simulation data container. Can allocated and used many times. It's designed to hold CPU or GPU particles data for the simulation and rendering.
/// </summary>
class FLAXENGINE_API ParticleBuffer
{
public:
    /// <summary>
    /// The emitter graph version (cached on Init). Used to discard pooled buffers that has been created for older emitter graph.
    /// </summary>
    uint32 Version;

    /// <summary>
    /// The buffer capacity (maximum amount of particles it can hold).
    /// </summary>
    int32 Capacity;

    /// <summary>
    /// The stride (size of the particle structure data).
    /// </summary>
    int32 Stride;

    /// <summary>
    /// The simulation mode.
    /// </summary>
    ParticlesSimulationMode Mode;

    /// <summary>
    /// The emitter.
    /// </summary>
    ParticleEmitter* Emitter = nullptr;

    /// <summary>
    /// The layout descriptor.
    /// </summary>
    ParticleLayout* Layout = nullptr;

    struct
    {
        /// <summary>
        /// The active particles count.
        /// </summary>
        int32 Count;

        /// <summary>
        /// The particles data buffer (CPU side).
        /// </summary>
        Array<byte> Buffer;

        /// <summary>
        /// The sorted ribbon particles indices (CPU side). Cached after system update and reused during rendering (batched for all ribbon modules).
        /// </summary>
        Array<int32> RibbonOrder;
    } CPU;

    struct
    {
        /// <summary>
        /// The particles data buffer (GPU side).
        /// </summary>
        /// <remarks>
        /// Contains particles data for GPU simulation and rendering. If simulation mode is GPU then this buffer is dynamic and updated before rendering, otherwise it's in default allocation mode.
        /// </remarks>
        GPUBuffer* Buffer = nullptr;

        /// <summary>
        /// The particles secondary data buffer (GPU side). Used as a write destination for GPU particles and swapped with Buffer after simulation.
        /// </summary>
        GPUBuffer* BufferSecondary = nullptr;

        /// <summary>
        /// The indirect draw command arguments buffer used by the GPU particles to invoke drawing on a GPU based on the particles amount (instances count).
        /// </summary>
        GPUBuffer* IndirectDrawArgsBuffer = nullptr;

        /// <summary>
        /// The GPU particles sorting buffer. Contains structure of particle index and the sorting key for every particle. Used to sort particles.
        /// </summary>
        GPUBuffer* SortingKeysBuffer = nullptr;

        /// <summary>
        /// The particles indices buffer (GPU side).
        /// </summary>
        /// <remarks>
        /// Contains sorted particles indices for GPU rendering. Each sorting module from the emitter uses a dedicated range of this buffer.
        /// </remarks>
        GPUBuffer* SortedIndices = nullptr;

        /// <summary>
        /// The ribbon particles rendering index buffer (dynamic GPU access).
        /// </summary>
        DynamicIndexBuffer* RibbonIndexBufferDynamic = nullptr;

        /// <summary>
        /// The ribbon particles rendering vertex buffer (dynamic GPU access).
        /// </summary>
        DynamicVertexBuffer* RibbonVertexBufferDynamic = nullptr;

        /// <summary>
        /// The flag used to indicate that GPU buffers data should be cleared before next simulation.
        /// </summary>
        bool PendingClear;

        /// <summary>
        /// The flag used to indicate that GPU buffers data contains a valid particles count.
        /// </summary>
        bool HasValidCount;

        /// <summary>
        /// The particle counter (uint type) offset. Located in Buffer and BufferSecondary if GPU particles simulation is used to store the particles count in the buffer. Placed after the particle attributes.
        /// </summary>
        uint32 ParticleCounterOffset;

        /// <summary>
        /// The maximum amount of particles that 'might' be in the buffer. During every simulation update we spawn a certain amount of particles and update existing ones. We can estimate limit for the current particles count to dispatch less threads for particles update.
        /// </summary>
        int32 ParticlesCountMax;
    } GPU;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ParticleBuffer"/> class.
    /// </summary>
    ParticleBuffer();

    /// <summary>
    /// Finalizes an instance of the <see cref="ParticleBuffer"/> class.
    /// </summary>
    ~ParticleBuffer();

    /// <summary>
    /// Initializes the particle buffer for the specified emitter.
    /// </summary>
    /// <param name="emitter">The emitter.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool Init(ParticleEmitter* emitter);

    /// <summary>
    /// Allocates the particles sorting indices buffer.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    bool AllocateSortBuffer();

    /// <summary>
    /// Clears the particles from the buffer (prepares for the simulation).
    /// </summary>
    void Clear();

    /// <summary>
    /// Gets the pointer to the particle data.
    /// </summary>
    /// <param name="particleIndex">Index of the particle.</param>
    /// <returns>The particle data start address.</returns>
    FORCE_INLINE byte* GetParticleCPU(int32 particleIndex)
    {
        return CPU.Buffer.Get() + particleIndex * Stride;
    }

    /// <summary>
    /// Gets the pointer to the particle data.
    /// </summary>
    /// <param name="particleIndex">Index of the particle.</param>
    /// <returns>The particle data start address.</returns>
    FORCE_INLINE const byte* GetParticleCPU(int32 particleIndex) const
    {
        return CPU.Buffer.Get() + particleIndex * Stride;
    }
};

struct ParticleBufferCPUDataAccessorBase
{
protected:
    ParticleBuffer* _buffer;
    int32 _offset;

public:
    ParticleBufferCPUDataAccessorBase()
        : _buffer(nullptr)
        , _offset(-1)
    {
    }

    ParticleBufferCPUDataAccessorBase(ParticleBuffer* buffer, int32 offset)
        : _buffer(buffer)
        , _offset(offset)
    {
    }

public:
    FORCE_INLINE bool IsValid() const
    {
        return _buffer != nullptr && _offset != -1;
    }
};

template<typename T>
struct ParticleBufferCPUDataAccessor : ParticleBufferCPUDataAccessorBase
{
public:
    ParticleBufferCPUDataAccessor<T>()
    {
    }

    ParticleBufferCPUDataAccessor(ParticleBuffer* buffer, int32 offset)
        : ParticleBufferCPUDataAccessorBase(buffer, offset)
    {
    }

public:
    FORCE_INLINE T operator[](int32 index) const
    {
        return Get(index);
    }

    FORCE_INLINE T Get(int32 index) const
    {
        ASSERT(IsValid() && index >= 0 && index < _buffer->CPU.Count);
        return *(T*)(_buffer->CPU.Buffer.Get() + _offset + index * _buffer->Stride);
    }

    FORCE_INLINE T Get(int32 index, const T& defaultValue) const
    {
        if (IsValid())
            return Get(index);
        return defaultValue;
    }
};
