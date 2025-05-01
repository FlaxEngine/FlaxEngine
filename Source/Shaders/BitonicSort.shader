// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Math.hlsl"

struct Item
{
	float Key;
	uint Value;
};

META_CB_BEGIN(0, Data)
float NullItemKey;
uint NullItemValue;
uint CounterOffset;
uint MaxIterations;
uint LoopK;
float KeySign;
uint LoopJ;
float Dummy0;
META_CB_END

// Buffer with counter of items to sort (accessed via uint load at CounterOffset address)
ByteAddressBuffer CounterBuffer : register(t0);

// Takes value and widens it by one bit at the location of the bit in the mask.
// A one is inserted in the space. The oneBitMask must have one and only one bit set.
uint InsertOneBit(uint value, uint oneBitMask)
{
	uint mask = oneBitMask - 1;
	return (value & ~mask) << 1 | (value & mask) | oneBitMask;
}

// Determines if two sort keys should be swapped in the list. KeySign is
// either 1 or -1. Multiplication with the KeySign will either invert the sign
// (effectively a negation) or leave the value alone. When the KeySign is
// 1, we are sorting descending, so when A < B, they should swap. For an
// ascending sort, -A < -B should swap.
bool ShouldSwap(Item a, Item b)
{
	//return (a ^ NullItem) < (b ^ NullItem);

	//return (a.Key) < (b.Key);
	return (a.Key * KeySign) < (b.Key * KeySign);
	//return asfloat(a) < asfloat(b);
	//return (asfloat(a) * KeySign) < (asfloat(b) * KeySign);
}

#ifdef _CS_IndirectArgs

RWByteAddressBuffer IndirectArgsBuffer : register(u0);

META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(22, 1, 1)]
void CS_IndirectArgs(uint groupIndex : SV_GroupIndex)
{
	if (groupIndex >= MaxIterations)
		return;

	uint count = CounterBuffer.Load(CounterOffset);
	uint k = 2048 << groupIndex;

	// We need one more iteration every time the number of thread groups doubles
	if (k > NextPow2((count + 2047) & ~2047))
		count = 0;

	uint prevDispatches = groupIndex * (groupIndex + 1) / 2;
	uint offset = 12 * prevDispatches;

	// Generate outer sort dispatch arguments
	for (uint j = k / 2; j > 1024; j /= 2)
	{
		// All of the groups of size 2j that are full
		uint completeGroups = (count & ~(2 * j - 1)) / 2048;

		// Remaining items must only be sorted if there are more than j of them
		uint partialGroups = ((uint)max(int(count - completeGroups * 2048 - j), 0) + 1023) / 1024;

		IndirectArgsBuffer.Store3(offset, uint3(completeGroups + partialGroups, 1, 1));

		offset += 12;
	}

	// The inner sort always sorts all groups (rounded up to multiples of 2048)
	IndirectArgsBuffer.Store3(offset, uint3((count + 2047) / 2048, 1, 1));
}

#endif

#if defined(_CS_PreSort) || defined(_CS_InnerSort)

RWStructuredBuffer<Item> SortBuffer : register(u0);

groupshared Item SortData[2048];

void LoadItem(uint element, uint count)
{
	// Unused elements must sort to the end
	Item item;
	if (element < count)
	{
		item = SortBuffer[element];
	}
	else
	{
		item.Key = NullItemKey;
		item.Value = NullItemValue;
	}
	SortData[element & 2047] = item;
}

void StoreItem(uint element, uint count)
{
	if (element < count)
	{
		SortBuffer[element] = SortData[element & 2047];
	}
}

#endif

#ifdef _CS_PreSort

META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(1024, 1, 1)]
void CS_PreSort(uint3 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	// Item index of the start of this group
	const uint groupStart = groupID.x * 2048;

	// Actual number of items that need sorting
	const uint count = CounterBuffer.Load(CounterOffset);

	LoadItem(groupStart + groupIndex, count);
	LoadItem(groupStart + groupIndex + 1024, count);

	GroupMemoryBarrierWithGroupSync();

	UNROLL
	for (uint k = 2; k <= 2048; k <<= 1)
	{
		for (uint j = k / 2; j > 0; j /= 2)
		{
			uint index2 = InsertOneBit(groupIndex, j);
			uint index1 = index2 ^ (k == 2 * j ? k - 1 : j);

			Item A = SortData[index1];
			Item B = SortData[index2];

			if (ShouldSwap(A, B))
			{
				// Swap the items
				SortData[index1] = B;
				SortData[index2] = A;
			}

			GroupMemoryBarrierWithGroupSync();
		}
	}

	// Write sorted results to memory
	StoreItem(groupStart + groupIndex, count);
	StoreItem(groupStart + groupIndex + 1024, count);
}

#endif

#ifdef _CS_InnerSort

META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(1024, 1, 1)]
void CS_InnerSort(uint3 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint count = CounterBuffer.Load(CounterOffset);

	// Item index of the start of this group
	const uint groupStart = groupID.x * 2048;

	// Load from memory into LDS to prepare sort
	LoadItem(groupStart + groupIndex, count);
	LoadItem(groupStart + groupIndex + 1024, count);

	GroupMemoryBarrierWithGroupSync();

	UNROLL
	for (uint j = 1024; j > 0; j /= 2)
	{
		uint index2 = InsertOneBit(groupIndex, j);
		uint index1 = index2 ^ j;

		Item A = SortData[index1];
		Item B = SortData[index2];

		if (ShouldSwap(A, B))
		{
			// Swap the items
			SortData[index1] = B;
			SortData[index2] = A;
		}

		GroupMemoryBarrierWithGroupSync();
	}

	StoreItem(groupStart + groupIndex, count);
	StoreItem(groupStart + groupIndex + 1024, count);
}

#endif

#ifdef _CS_OuterSort

RWStructuredBuffer<Item> SortBuffer : register(u0);

META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(1024, 1, 1)]
void CS_OuterSort(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	const uint count = CounterBuffer.Load(CounterOffset);

	// Form unique index pair from dispatch thread ID
	uint index2 = InsertOneBit(dispatchThreadId.x, LoopJ);
	uint index1 = index2 ^ (LoopK == 2 * LoopJ ? LoopK - 1 : LoopJ);

	if (index2 >= count)
		return;

	Item A = SortBuffer[index1];
	Item B = SortBuffer[index2];

	if (ShouldSwap(A, B))
	{
		// Swap the items
		SortBuffer[index1] = B;
		SortBuffer[index2] = A;
	}
}

#endif

#ifdef _CS_CopyIndices

StructuredBuffer<Item> SortBuffer : register(t1);
RWBuffer<uint> SortedIndices : register(u0);

META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(1024, 1, 1)]
void CS_CopyIndices(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	const uint count = CounterBuffer.Load(CounterOffset);
	uint index = dispatchThreadId.x;

	if (index >= count)
		return;

	Item element = SortBuffer[index];

	SortedIndices[index] = element.Value;
}

#endif
