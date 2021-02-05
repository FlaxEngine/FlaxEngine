// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

@0// Tessellation: Defines
#define TessalationProjectOntoPlane(planeNormal, planePosition, pointToProject) pointToProject - dot(pointToProject - planePosition, planeNormal) * planeNormal
@1// Tessellation: Includes
@2// Tessellation: Constants
@3// Tessellation: Resources
@4// Tessellation: Utilities
@5// Tessellation: Shaders
#if USE_TESSELLATION

// Interpolants passed from the hull shader to the domain shader
struct TessalationHSToDS
{
	float4 Position : SV_Position;
	GeometryData Geometry;
#if USE_CUSTOM_VERTEX_INTERPOLATORS
	float4 CustomVSToPS[CUSTOM_VERTEX_INTERPOLATORS_COUNT] : TEXCOORD9;
#endif
	float TessellationMultiplier : TESS;
};

// Interpolants passed from the domain shader and to the pixel shader
struct TessalationDSToPS
{
	float4 Position : SV_Position;
	GeometryData Geometry;
#if USE_CUSTOM_VERTEX_INTERPOLATORS
	float4 CustomVSToPS[CUSTOM_VERTEX_INTERPOLATORS_COUNT] : TEXCOORD9;
#endif
};

MaterialInput GetMaterialInput(TessalationDSToPS input)
{
	MaterialInput output = GetGeometryMaterialInput(input.Geometry);
	output.SvPosition = input.Position;
	output.TwoSidedSign = WorldDeterminantSign;
#if USE_CUSTOM_VERTEX_INTERPOLATORS
	output.CustomVSToPS = input.CustomVSToPS;
#endif
	return output;
}

struct TessalationPatch
{
	float EdgeTessFactor[3] : SV_TessFactor;
	float InsideTessFactor : SV_InsideTessFactor;
#if MATERIAL_TESSELLATION == MATERIAL_TESSELLATION_PN
	float3 B210 : POSITION4;
	float3 B120 : POSITION5;
	float3 B021 : POSITION6;
	float3 B012 : POSITION7;
	float3 B102 : POSITION8;
	float3 B201 : POSITION9;
	float3 B111 : CENTER;
#endif
};

TessalationPatch HS_PatchConstant(InputPatch<VertexOutput, 3> input)
{
	TessalationPatch output;

	// Average tess factors along edges, and pick an edge tess factor for the interior tessellation
	float4 tessellationMultipliers;
	tessellationMultipliers.x = 0.5f * (input[1].TessellationMultiplier + input[2].TessellationMultiplier);
	tessellationMultipliers.y = 0.5f * (input[2].TessellationMultiplier + input[0].TessellationMultiplier);
	tessellationMultipliers.z = 0.5f * (input[0].TessellationMultiplier + input[1].TessellationMultiplier);
	tessellationMultipliers.w = 0.333f * (input[0].TessellationMultiplier + input[1].TessellationMultiplier + input[2].TessellationMultiplier);
	tessellationMultipliers = clamp(tessellationMultipliers, 1, MAX_TESSELLATION_FACTOR);

	output.EdgeTessFactor[0] = tessellationMultipliers.x; // 1->2 edge
	output.EdgeTessFactor[1] = tessellationMultipliers.y; // 2->0 edge
	output.EdgeTessFactor[2] = tessellationMultipliers.z; // 0->1 edge
	output.InsideTessFactor  = tessellationMultipliers.w;

#if MATERIAL_TESSELLATION == MATERIAL_TESSELLATION_PN
	// Calculate PN Triangle control points
	// Reference: [Vlachos 2001]
	float3 p1 = input[0].Geometry.WorldPosition;
	float3 p2 = input[1].Geometry.WorldPosition;
	float3 p3 = input[2].Geometry.WorldPosition;
	float3 n1 = input[0].Geometry.WorldNormal;
	float3 n2 = input[1].Geometry.WorldNormal;
	float3 n3 = input[2].Geometry.WorldNormal;
	output.B210 = (2.0f * p1 + p2 - dot((p2 - p1), n1) * n1) / 3.0f;
	output.B120 = (2.0f * p2 + p1 - dot((p1 - p2), n2) * n2) / 3.0f;
	output.B021 = (2.0f * p2 + p3 - dot((p3 - p2), n2) * n2) / 3.0f;
	output.B012 = (2.0f * p3 + p2 - dot((p2 - p3), n3) * n3) / 3.0f;
	output.B102 = (2.0f * p3 + p1 - dot((p1 - p3), n3) * n3) / 3.0f;
	output.B201 = (2.0f * p1 + p3 - dot((p3 - p1), n1) * n1) / 3.0f;
	float3 e = (output.B210 + output.B120 + output.B021 + output.B012 + output.B102 + output.B201) / 6.0f;
	float3 v = (p1 + p2 + p3) / 3.0f;
	output.B111 = e + ((e - v) / 2.0f);
#endif

	return output;
}

META_HS(USE_TESSELLATION, FEATURE_LEVEL_SM5)
META_HS_PATCH(TESSELLATION_IN_CONTROL_POINTS)
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[maxtessfactor(MAX_TESSELLATION_FACTOR)]
[outputcontrolpoints(3)]
[patchconstantfunc("HS_PatchConstant")]
TessalationHSToDS HS(InputPatch<VertexOutput, TESSELLATION_IN_CONTROL_POINTS> input, uint ControlPointID : SV_OutputControlPointID)
{
	TessalationHSToDS output;

	// Pass through shader
#define COPY(thing) output.thing = input[ControlPointID].thing;
	COPY(Position);
	COPY(Geometry);
	COPY(TessellationMultiplier);
#if USE_CUSTOM_VERTEX_INTERPOLATORS
	COPY(CustomVSToPS);
#endif
#undef COPY

	return output;
}

META_DS(USE_TESSELLATION, FEATURE_LEVEL_SM5)
[domain("tri")]
TessalationDSToPS DS(TessalationPatch constantData, float3 barycentricCoords : SV_DomainLocation, const OutputPatch<TessalationHSToDS, 3> input)
{
	TessalationDSToPS output;

	// Get the barycentric coords
	float U = barycentricCoords.x;
	float V = barycentricCoords.y;
	float W = barycentricCoords.z;

	// Interpolate patch attributes to generated vertices
	output.Geometry = InterpolateGeometry(input[0].Geometry, U, input[1].Geometry, V, input[2].Geometry, W);
#define INTERPOLATE(thing) output.thing = U * input[0].thing + V * input[1].thing + W * input[2].thing
	INTERPOLATE(Position);
#if MATERIAL_TESSELLATION == MATERIAL_TESSELLATION_PN
	// Interpolate using barycentric coordinates and PN Triangle control points
	float UU = U * U;
	float VV = V * V;
	float WW = W * W;
	float UU3 = UU * 3.0f;
	float VV3 = VV * 3.0f;
	float WW3 = WW * 3.0f;
	float3 offset = 
		constantData.B210 * UU3 * V +
		constantData.B120 * VV3 * U +
		constantData.B021 * VV3 * W +
		constantData.B012 * WW3 * V +
		constantData.B102 * WW3 * U +
		constantData.B201 * UU3 * W +
		constantData.B111 * 6.0f * W * U * V;
	InterpolateGeometryPositions(output.Geometry, input[0].Geometry, UU * U, input[1].Geometry, VV * V, input[2].Geometry, WW * W, offset);
#else
	InterpolateGeometryPositions(output.Geometry, input[0].Geometry, U, input[1].Geometry, V, input[2].Geometry, W, float3(0, 0, 0));
#endif
#if USE_CUSTOM_VERTEX_INTERPOLATORS
	UNROLL
	for (int i = 0; i < CUSTOM_VERTEX_INTERPOLATORS_COUNT; i++)
		INTERPOLATE(CustomVSToPS[i]);
#endif
#undef INTERPOLATE

#if MATERIAL_TESSELLATION == MATERIAL_TESSELLATION_PHONG
	// Orthogonal projection in the tangent planes with interpolation
	ApplyGeometryPositionsPhongTess(output.Geometry, input[0].Geometry, input[1].Geometry, input[2].Geometry, U, V, W);
#endif

	// Perform displacement mapping
#if USE_DISPLACEMENT
	MaterialInput materialInput = GetMaterialInput(output);
	Material material = GetMaterialDS(materialInput);
	OffsetGeometryPositions(output.Geometry, material.WorldDisplacement);
#endif

	// Recalculate the clip space position
	output.Position = mul(float4(output.Geometry.WorldPosition, 1), ViewProjectionMatrix);

	return output;
}

#endif
