// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

#ifndef SORT_MODE
#define SORT_MODE 0
#endif

// Primary constant buffer
META_CB_BEGIN(0, Data)
float3 ViewPosition;
uint ParticleCounterOffset;
uint ParticleStride;
uint ParticleCapacity;
uint PositionOffset;
uint CustomOffset;
float4x4 PositionTransform;
META_CB_END

// Particles data buffer
ByteAddressBuffer ParticlesData : register(t0);

// Output sorting keys buffer (index + key)
struct Item
{
	float Key;
	uint Value;
};
RWStructuredBuffer<Item> SortingKeys : register(u0);

float GetParticleFloat(uint particleIndex, int offset)
{
	return asfloat(ParticlesData.Load(particleIndex * ParticleStride + offset));
}

float3 GetParticleVec3(uint particleIndex, int offset)
{
	return asfloat(ParticlesData.Load3(particleIndex * ParticleStride + offset));
}

// Sorting keys generation shader
META_CS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_1(SORT_MODE=0)
META_PERMUTATION_1(SORT_MODE=1)
META_PERMUTATION_1(SORT_MODE=2)
[numthreads(1024, 1, 1)]
void CS_Sort(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint index = dispatchThreadId.x;
	uint particlesCount = min(ParticlesData.Load(ParticleCounterOffset), ParticleCapacity);
	if (index >= particlesCount)
		return;

	// TODO: maybe process more than 1 particle at once and pre-sort them?

#if SORT_MODE == 0

	// Sort particles by depth to the view's near plane
	float3 particlePosition = GetParticleVec3(index, PositionOffset);
	float4 position = mul(float4(particlePosition, 1.0f), PositionTransform);
	float sortKey = position.w;

#elif SORT_MODE == 1

	// Sort particles by distance to the view's origin
	float3 particlePosition = GetParticleVec3(index, PositionOffset);
	float4 position = mul(float4(particlePosition, 1.0f), PositionTransform);
	float3 direction = ViewPosition - position.xyz;
	float sortKey = dot(direction, direction);

#elif SORT_MODE == 2

	// Sort particles by the custom attribute
	float sortKey = GetParticleFloat(index, CustomOffset);

#else

#error Unknown sorting mode!

#endif

	// Write sorting index-key pair
	Item item;
	item.Key = sortKey;
	item.Value = index;
	SortingKeys[index] = item;
}
