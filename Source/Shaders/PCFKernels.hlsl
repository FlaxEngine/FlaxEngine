// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#ifndef __PCF_KERNELS__
#define __PCF_KERNELS__

// Cascades Shadow Mapping

#if FilterSizeCSM == 2

#elif FilterSizeCSM == 3

static const float CSMFilterWeightsSum = 7;
static const float CSMFilterWeights[3][3] =
{
    { 0.5,1.0,0.5 },
    { 1.0,1.0,1.0 },
    { 0.5,1.0,0.5 }
};

#elif FilterSizeCSM == 5

static const float CSMFilterWeightsSum = 17;
static const float CSMFilterWeights[5][5] =
{
    { 0.0,0.5,1.0,0.5,0.0 },
    { 0.5,1.0,1.0,1.0,0.5 },
    { 1.0,1.0,1.0,1.0,1.0 },
    { 0.5,1.0,1.0,1.0,0.5 },
    { 0.0,0.5,1.0,0.5,0.0 }
};

#elif FilterSizeCSM == 7

static const float CSMFilterWeightsSum = 33;
static const float CSMFilterWeights[7][7] =
{
    { 0.0,0.0,0.5,1.0,0.5,0.0,0.0 },
    { 0.0,1.0,1.0,1.0,1.0,1.0,0.0 },
    { 0.5,1.0,1.0,1.0,1.0,1.0,0.5 },
    { 1.0,1.0,1.0,1.0,1.0,1.0,1.0 },
    { 0.5,1.0,1.0,1.0,1.0,1.0,0.5 },
    { 0.0,1.0,1.0,1.0,1.0,1.0,0.0 },
    { 0.0,0.0,0.5,1.0,0.5,0.0,0.0 }
};

#elif FilterSizeCSM == 9

static const float CSMFilterWeightsSum = 53;
static const float CSMFilterWeights[9][9] =
{
    { 0.0,0.0,0.0,0.5,1.0,0.5,0.0,0.0,0.0 },
    { 0.0,0.0,1.0,1.0,1.0,1.0,1.0,0.0,0.0 },
    { 0.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,0.0 },
    { 0.5,1.0,1.0,1.0,1.0,1.0,1.0,1.0,0.5 },
    { 1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0 },
    { 0.5,1.0,1.0,1.0,1.0,1.0,1.0,1.0,0.5 },
    { 0.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,0.0 },
    { 0.0,0.0,1.0,1.0,1.0,1.0,1.0,0.0,0.0 },
    { 0.0,0.0,0.0,0.5,1.0,0.5,0.0,0.0,0.0 }
};

#endif

// Cube Map Shadows

#if FilterSizeCube == 5

// 5 random points in disc with radius 2.5
static const float2 PCFDiscSamples[5] =
{
	float2(0.000000, 2.500000),
	float2(2.377641, 0.772542),
	float2(1.469463, -2.022543),
	float2(-1.469463, -2.022542),
	float2(-2.377641, 0.772543),
};

#elif FilterSizeCube == 12

// 12 random points in disc with radius 2.5
static const float2 PCFDiscSamples[12] =
{
	float2(0.000000, 2.500000),
	float2(1.767767, 1.767767),
	float2(2.500000, -0.000000),
	float2(1.767767, -1.767767),
	float2(-0.000000, -2.500000),
	float2(-1.767767, -1.767767),
	float2(-2.500000, 0.000000),
	float2(-1.767766, 1.767768),
	float2(-1.006119, -0.396207),
	float2(1.000015, 0.427335),
	float2(0.416807, -1.006577),
	float2(-0.408872, 1.024430),
};

#elif FilterSizeCube == 29

// 29 random points in disc with radius 2.5
static const float2 PCFDiscSamples[29] =
{
	float2(0.000000, 2.500000),
	float2(1.016842, 2.283864),
	float2(1.857862, 1.672826),
	float2(2.377641, 0.772542),
	float2(2.486305, -0.261321),
	float2(2.165063, -1.250000),
	float2(1.469463, -2.022543),
	float2(0.519779, -2.445369),
	float2(-0.519779, -2.445369),
	float2(-1.469463, -2.022542),
	float2(-2.165064, -1.250000),
	float2(-2.486305, -0.261321),
	float2(-2.377641, 0.772543),
	float2(-1.857862, 1.672827),
	float2(-1.016841, 2.283864),
	float2(0.091021, -0.642186),
	float2(0.698035, 0.100940),
	float2(0.959731, -1.169393),
	float2(-1.053880, 1.180380),
	float2(-1.479156, -0.606937),
	float2(-0.839488, -1.320002),
	float2(1.438566, 0.705359),
	float2(0.067064, -1.605197),
	float2(0.728706, 1.344722),
	float2(1.521424, -0.380184),
	float2(-0.199515, 1.590091),
	float2(-1.524323, 0.364010),
	float2(-0.692694, -0.086749),
	float2(-0.082476, 0.654088),
};

#endif

#endif
