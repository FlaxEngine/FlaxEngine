// This function is designed to only work with half/half depth at the moment - there's a couple of hardcoded paths that expect pixel/texel size, so it will not work for full res
void GenerateGTAOShadowsInternal(out float outShadowTerm, out float4 outEdges, out float outWeight, const float2 SVPos, const float2 normalizedScreenPos, int qualityLevel)
{
	float2 SVPosRounded = trunc(SVPos);
	int2 SVPosui = int2(SVPosRounded); // same as uint2(SVPos)
	
	// Load this pixel's viewspace normal
	int2 fullResCoord = SVPosui * 2 + PerPassFullResCoordOffset.xy;
	float3 pixelNormal = LoadNormal(fullResCoord);

	float pixZ, pixLZ, pixTZ, pixRZ, pixBZ;

	float4 valuesUL = TextureGatherRed(g_DepthSource, SamplerPointWrap, SVPosRounded * HalfViewportPixelSize);
	float4 valuesBR = TextureGatherRed(g_DepthSource, SamplerPointWrap, SVPosRounded * HalfViewportPixelSize, int2(1, 1));

	// Get this pixel's viewspace depth
	pixZ = valuesUL.y;
	// Get left right top bottom neighbouring pixels for edge detection (gets compiled out on qualityLevel == 0)
	pixLZ = valuesUL.x;
	pixTZ = valuesUL.z;
	pixRZ = valuesBR.z;
	pixBZ = valuesBR.x;

	// Edge mask for between this and left/right/top/bottom neighbour pixels - not used in quality level 0 so initialize to "no edge" (1 is no edge, 0 is edge)
	float4 edgesLRTB = float4(1.0, 1.0, 1.0, 1.0);

	if (qualityLevel >= SSAO_DEPTH_BASED_EDGES_ENABLE_AT_QUALITY_PRESET)
	{
		edgesLRTB = CalculateEdges(pixZ, pixLZ, pixRZ, pixTZ, pixBZ);
	}

	// Sharp normals also create edges - but this adds to the cost as well
	if (qualityLevel >= SSAO_NORMAL_BASED_EDGES_ENABLE_AT_QUALITY_PRESET)
	{
		float3 neighbourNormalL = LoadNormal(fullResCoord, int2(-2, 0));
		float3 neighbourNormalR = LoadNormal(fullResCoord, int2(2, 0));
		float3 neighbourNormalT = LoadNormal(fullResCoord, int2(0, -2));
		float3 neighbourNormalB = LoadNormal(fullResCoord, int2(0, 2));
		
		const float dotThreshold = SSAO_NORMAL_BASED_EDGES_DOT_THRESHOLD;
		
		float4 normalEdgesLRTB;
		normalEdgesLRTB.x = saturate((dot(pixelNormal, neighbourNormalL) + dotThreshold));
		normalEdgesLRTB.y = saturate((dot(pixelNormal, neighbourNormalR) + dotThreshold));
		normalEdgesLRTB.z = saturate((dot(pixelNormal, neighbourNormalT) + dotThreshold));
		normalEdgesLRTB.w = saturate((dot(pixelNormal, neighbourNormalB) + dotThreshold));
		
		edgesLRTB *= normalEdgesLRTB;
	}

	float deviceDepth = SampleDeviceDepth(normalizedScreenPos, 0.0);
	const float gtaoRadius = 0.5;
	const float gtaoThickness = 0.1;

	float stepRadius = 60 * gtaoRadius / LinearizeZ(GetGBufferData(), deviceDepth);
	stepRadius /= (float(GTAO_SAMPLE_COUNT) + 1.0f); // Divide by steps + 1 so that the farthest samples are not fully attenuated
    const float attenFactor = 2.0 / max(gtaoRadius * gtaoRadius, 0.001);

    float occlusion = 0.0;
	float3 viewPos = GetViewPos(GetGBufferData(), normalizedScreenPos, deviceDepth);
	float3 viewDir = normalize(viewPos);

	// Unreal speedup
	const float deltaAngle = PI / GTAO_SLICE_COUNT;
	const float sinDeltaAngle = sin(deltaAngle), cosDeltaAngle = cos(deltaAngle);

	// Slice direction, normalized to pixel size
	float2 sliceDir = float2(1, 0);

	for(int slice = 0; slice < GTAO_SLICE_COUNT; slice++){
		float2 bestAng = float2(-1.0, -1.0);

		for(int samp = 1; samp <= GTAO_SAMPLE_COUNT; samp++){
			float2 uvOffset = sliceDir * (stepRadius * samp) * HalfViewportPixelSize;
			uvOffset.y *= -1;
			float4 samplePos = normalizedScreenPos.xyxy + float4(uvOffset.xy, -uvOffset.xy);

			float mipmap = 0;
			if(samp == 2){
				mipmap += 1;
			}
			if(samp > 3){
				mipmap += 1;
			}

			float3 h1 = GetViewPos(gBufferData, samplePos.xy, SampleDeviceDepth(samplePos.xy, mipmap)) - viewPos;
			float3 h2 = GetViewPos(GetGBufferData(), samplePos.zw, SampleDeviceDepth(samplePos.zw, mipmap)) - viewPos;

			float h1LenSq = dot(h1, h1);
			float falloffH1 = saturate(h1LenSq * attenFactor);
			if(falloffH1 < 1.0)
			{
				float angH1 = dot(h1, viewDir) * 1 / sqrt(h1LenSq + 0.0001);
				angH1 = lerp(angH1, bestAng.x, falloffH1);
				bestAng.x = (angH1 > bestAng.x) ? angH1 : lerp(angH1, bestAng.x, gtaoThickness);
			}

			float h2LenSq = dot(h2, h2);
			float falloffH2 = saturate(h2LenSq * attenFactor);
			if(falloffH2 < 1.0)
			{
				float angH2 = dot(h2, viewDir) * 1 / sqrt(h2LenSq + 0.0001);
				angH2 = lerp(angH2, bestAng.y, falloffH2);
				bestAng.y = (angH2 > bestAng.y) ? angH2 : lerp(angH2, bestAng.y, gtaoThickness);
			}
		}

		bestAng.x = acos(clamp(bestAng.x, -1.0, 1.0));
        bestAng.y = acos(clamp(bestAng.y, -1.0, 1.0));

		// Compute inner integral.
		{
			// Given the angles found in the search plane we need to project the View Space Normal onto the plane defined by the search axis and the View Direction and perform the inner integrate
			float3 planeNormal = normalize(cross(float3(sliceDir, 0.0), viewDir));
			float3 perp = cross(viewDir, planeNormal);
			float3 projNormal = pixelNormal - planeNormal * dot(pixelNormal, planeNormal);

			float lenProjNormal = length(projNormal) + 0.000001f;
			float recipMag = 1.0 / lenProjNormal;

			float cosAng = dot(projNormal, perp) * recipMag;	
			float gamma = acos(cosAng) - 0.5 * PI;				
			float cosGamma = dot(projNormal, viewDir) * recipMag;
			float sinGamma = cosAng * (-2.0);					

			// clamp to normal hemisphere 
			bestAng.x = gamma + max(-bestAng.x - gamma, -(0.5 * PI));
			bestAng.y = gamma + min( bestAng.y - gamma,  (0.5 * PI));

			// See Activision GTAO paper: https://www.activision.com/cdn/research/s2016_pbs_activision_occlusion.pptx
			// Integrate arcCos weight.
			float ao = ((lenProjNormal) * 0.25 * 
				((bestAng.x * sinGamma + cosGamma - cos(2.0 * bestAng.x - gamma)) +
					(bestAng.y * sinGamma + cosGamma - cos(2.0 * bestAng.y - gamma))));

			occlusion += ao;
		}

		float2 tmpDir = sliceDir;
		sliceDir.x = tmpDir.x * cosDeltaAngle - tmpDir.y * sinDeltaAngle;
		sliceDir.y = tmpDir.x * sinDeltaAngle + tmpDir.y * cosDeltaAngle;
	}

	occlusion = occlusion / float(GTAO_SLICE_COUNT);
    occlusion *= 2.0 / PI;

	outShadowTerm = 1 - occlusion;
	outEdges = edgesLRTB;
	outWeight = 1;
}

