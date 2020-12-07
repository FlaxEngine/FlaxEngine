// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#ifndef __RANDOM__
#define __RANDOM__

// @param xy should be a integer position (e.g. pixel position on the screen), repeats each 128x128 pixels similar to a texture lookup but is only ALU
float PseudoRandom(float2 xy)
{
	float2 pos = frac(xy / 128.0f) * 128.0f + float2(-64.340622f, -72.465622f);
	return frac(dot(pos.xyx * pos.xyy, float3(20.390625f, 60.703125f, 2.4281209f)));
}

// Find good arbitrary axis vectors to represent U and V axes of a plane, given just the normal
void FindBestAxisVectors(float3 input, out float3 axis1, out float3 axis2)
{
	const float3 N = abs(input);

	// Find best basis vectors
	if( N.z > N.x && N.z > N.y )
	{
		axis1 = float3(1, 0, 0);
	}
	else
	{
		axis1 = float3(0, 0, 1);
	}

	axis1 = normalize(axis1 - input * dot(axis1, input));
	axis2 = cross(axis1, input);
}

// References for noise:
//
// Improved Perlin noise
//   http://mrl.nyu.edu/~perlin/noise/
//   http://http.developer.nvidia.com/GPUGems/gpugems_ch05.html
// Modified Noise for Evaluation on Graphics Hardware
//   http://www.csee.umbc.edu/~olano/papers/mNoise.pdf
// Perlin Noise
//   http://mrl.nyu.edu/~perlin/doc/oscar.html
// Fast Gradient Noise
//   http://prettyprocs.wordpress.com/2012/10/20/fast-perlin-noise


// -------- ALU based method ---------

/*
 * Pseudo random number generator, based on "TEA, a tiny Encrytion Algorithm"
 * http://citeseer.ist.psu.edu/viewdoc/download?doi=10.1.1.45.281&rep=rep1&type=pdf
 * @param v - old seed (full 32bit range)
 * @param iterationCount - >=1, bigger numbers cost more performance but improve quality
 * @return new seed
 */
uint2 ScrambleTEA(uint2 v, uint iterationCount = 3)
{
	// Start with some random data (numbers can be arbitrary but those have been used by others and seem to work well)
	uint k[4] ={ 0xA341316Cu , 0xC8013EA4u , 0xAD90777Du , 0x7E95761Eu };

	uint y = v[0];
	uint z = v[1];
	uint sum = 0;

	UNROLL
	for (uint i = 0; i < iterationCount; i++)
	{
		sum += 0x9e3779b9;
		y += (z << 4u) + k[0] ^ z + sum ^ (z >> 5u) + k[1];
		z += (y << 4u) + k[2] ^ y + sum ^ (y >> 5u) + k[3];
	}

	return uint2(y, z);
}

// Computes a pseudo random number for a given integer 2D position
// @param v - old seed (full 32bit range)
// @return random number in the range -1 .. 1
float ComputeRandomFrom2DPosition(uint2 v)
{
	return (ScrambleTEA(v).x & 0xffff ) / (float)(0xffff) * 2 - 1;
}

// Computes a pseudo random number for a given integer 2D position
// @param v - old seed (full 32bit range)
// @return random number in the range -1 .. 1
float ComputeRandomFrom3DPosition(int3 v) 
{
	// numbers found by experimentation
	return ComputeRandomFrom2DPosition(v.xy ^ (uint2(0x123456, 0x23446) * v.zx) );
}

// Evaluate polynomial to get smooth transitions for Perlin noise (2 add, 5 mul)
float PerlinRamp(in float t)
{
	return t * t * t * (t * (t * 6 - 15) + 10); 
}
float2 PerlinRamp(in float2 t)
{
	return t * t * t * (t * (t * 6 - 15) + 10); 
}
float3 PerlinRamp(in float3 t)
{
	return t * t * t * (t * (t * 6 - 15) + 10); 
}
float4 PerlinRamp(in float4 t)
{
	return t * t * t * (t * (t * 6 - 15) + 10); 
}

#endif
