// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Atmosphere.hlsl"

const static int TransmittanceIntegralSamples = 500;
const static int InscatterIntegralSamples = 50;
const static int IrradianceIntegralSamples = 32;
const static int IrradianceIntegralSamplesHalf = IrradianceIntegralSamples / 2;
const static int InscatterSphericalIntegralSamples = 16;
const static float IrradianceDeltaPhi = PI / float(IrradianceIntegralSamples);
const static float IrradianceDeltaTheta = PI / float(IrradianceIntegralSamples);
const static float InscatterDeltaPhi = PI / float(InscatterSphericalIntegralSamples);
const static float InscatterDeltaTheta = PI / float(InscatterSphericalIntegralSamples);

META_CB_BEGIN(0, Data)
float FirstOrder;
float AtmosphereR;
int AtmosphereLayer;
float Dummy0;
float4 DhdH;
META_CB_END

Texture2D AtmosphereDeltaETexture  : register(t3);
Texture3D AtmosphereDeltaSRTexture : register(t4);
Texture3D AtmosphereDeltaSMTexture : register(t5);
Texture3D AtmosphereDeltaJTexture  : register(t6);

struct AtmosphereGSOutput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD0;
    //uint LayerIndex : SV_RenderTargetArrayIndex;
};

/*
META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION, 0, R32G32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 0, R32G32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
Quad_VS2PS VS(float2 Position : POSITION0, float2 TexCoord : TEXCOORD0)
{
	Quad_VS2PS output;
	
	output.Position = float4(Position, 0, 1);
	output.TexCoord = TexCoord;
	
	return output;
}

META_GS(true, FEATURE_LEVEL_SM4)
[maxvertexcount(3)]
void GS_Atmosphere(triangle Quad_VS2PS input[3], inout TriangleStream<AtmosphereGSOutput> output)
{
	AtmosphereGSOutput vertex;
	
	for(int i = 0; i < 3; i++)
	{
		vertex.Position = input[i].Position;
		vertex.TexCoord = input[i].TexCoord;
		vertex.LayerIndex = AtmosphereLayer;
		
		output.Append(vertex);
	}
}
*/
float OpticalDepth(float H, float radius, float Mu) 
{
    float result = 0.0;
    float Ti = Limit(radius, Mu) / float(TransmittanceIntegralSamples);
    float Xi = 0.0;
    float Yi = exp(-(radius - RadiusGround) / H);

	LOOP
    for (int i = 1; i <= TransmittanceIntegralSamples; i++) 
	{
        float Xj = float(i) * Ti;
        float Yj = exp(-(sqrt(radius * radius + Xj * Xj + 2.0 * Xj * radius * Mu) - RadiusGround) / H);
        result += (Yi + Yj) / 2.0 * Ti;
        Xi = Xj;
        Yi = Yj;
    }

    return Mu < -sqrt(1.0 - (RadiusGround / radius) * (RadiusGround / radius)) ? 1e9 : result;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Transmittance(Quad_VS2PS input) : SV_Target0
{
	float radius, MuS;
	GetTransmittanceRMuS(input.TexCoord, radius, MuS);
	float3 depth = BetaRayleighScattering * OpticalDepth(HeightScaleRayleigh, radius, MuS) + BetaMieExtinction * OpticalDepth(HeightScaleMie, radius, MuS);
	return float4(exp(-depth), 0.0f); // Eq (5)
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Irradiance1(Quad_VS2PS input) : SV_Target0
{
	float radius, MuS;
	GetIrradianceRMuS(input.TexCoord, radius, MuS);
	return float4(Transmittance(radius, MuS) * max(MuS, 0.0), 0.0);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_IrradianceN(Quad_VS2PS input) : SV_Target0
{
	float radius, MuS;
	GetIrradianceRMuS(input.TexCoord, radius, MuS);
	float3 S = float3(sqrt(max(1.0 - MuS * MuS, 0.0)), 0.0, MuS);
	float3 result = float3(0.0f, 0.0f, 0.0f);

	// Integral over 2.PI around x with two nested loops over W directions (theta, phi) -- Eq (15)
	for (int iPhi = 0; iPhi < 4 * IrradianceIntegralSamplesHalf; iPhi++) 
	{
		float phi = (float(iPhi) + 0.5) * IrradianceDeltaPhi;
		for (int iTheta = 0; iTheta < IrradianceIntegralSamplesHalf; iTheta++) 
		{
			float theta = (float(iTheta) + 0.5) * IrradianceDeltaTheta;
			float Dw = IrradianceDeltaTheta * IrradianceDeltaPhi * sin(theta);
			float3 W = float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
			float Nu = dot(S, W);

			if (FirstOrder == 1.0)
			{
				// First iteration is special because Rayleigh and Mie were stored separately,
				// without the phase functions factors; they must be reintroduced here
				float Pr1 = PhaseFunctionR(Nu);
				float Pm1 = PhaseFunctionM(Nu);
				float3 Ray1 = Texture4DSample(AtmosphereDeltaSRTexture, radius, W.z, MuS, Nu).rgb;
				float3 Mie1 = Texture4DSample(AtmosphereDeltaSMTexture, radius, W.z, MuS, Nu).rgb;

				result += (Ray1 * Pr1 + Mie1 * Pm1) * W.z * Dw;
			}
			else 
			{
				result += Texture4DSample(AtmosphereDeltaSRTexture, radius, W.z, MuS, Nu).rgb * W.z * Dw;
			}
		}
	}

	return float4(result, 0.0);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CopyIrradiance1(Quad_VS2PS input) : SV_Target0
{
	return AtmosphereDeltaETexture.Sample(SamplerLinearClamp, input.TexCoord);
}

void Integrand(float radius, float Mu, float MuS, float Nu, float T, out float3 Ray, out float3 Mie) 
{
    Ray = float3(0, 0, 0);
    Mie = float3(0, 0, 0);
    float Ri = sqrt(radius * radius + T * T + 2.0 * radius * Mu * T);
    float MuSi = (Nu * T + MuS * radius) / Ri;
	Ri = max(RadiusGround, Ri);
	if (MuSi >= -sqrt(1.0 - RadiusGround * RadiusGround / (Ri * Ri)) ) 
	{
		float3 Ti = TransmittanceWithDistance(radius, Mu, T) * Transmittance(Ri, MuSi);
		Ray = exp(-(Ri - RadiusGround) / HeightScaleRayleigh) * Ti;
		Mie = exp(-(Ri - RadiusGround) / HeightScaleMie) * Ti;
	}
}

// For Inscatter 1
void Inscatter(float radius, float Mu, float MuS, float Nu, out float3 Ray, out float3 Mie)
{
    Ray = float3(0, 0, 0);
    Mie = float3(0, 0, 0);
    float Dx = Limit(radius, Mu) / float(InscatterIntegralSamples);
    float Xi = 0.0;
    float3 Rayi;
    float3 Miei;
    Integrand(radius, Mu, MuS, Nu, 0.0, Rayi, Miei);
    for (int i = 1; i <= InscatterIntegralSamples; i++) 
	{
        float Xj = float(i) * Dx;
        float3 Rayj;
        float3 Miej;
        Integrand(radius, Mu, MuS, Nu, Xj, Rayj, Miej);
        Ray += (Rayi + Rayj) / 2.0 * Dx;
        Mie += (Miei + Miej) / 2.0 * Dx;
        Xi = Xj;
        Rayi = Rayj;
        Miei = Miej;
    }
    Ray *= BetaRayleighScattering;
    Mie *= BetaMieScattering;
}

struct Inscatter1Output
{
	float4 DeltaSR : SV_Target0;
	float4 DeltaSM : SV_Target1;
};

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Inscatter1_A(AtmosphereGSOutput input) : SV_Target
{
    float3 Ray;
    float3 Mie;
    float Mu, MuS, Nu;
    GetMuMuSNu(input.TexCoord, AtmosphereR, DhdH, Mu, MuS, Nu);
    Inscatter(AtmosphereR, Mu, MuS, Nu, Ray, Mie);

    // Store separately Rayleigh and Mie contributions, WITHOUT the phase function factor (cf "Angular precision")
    return float4(Ray, 1);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CopyInscatter1(AtmosphereGSOutput input) : SV_Target0
{
	float3 UVW = float3(input.TexCoord, (float(AtmosphereLayer) + 0.5f) / float(AtmosphericFogInscatterAltitudeSampleNum));
    float4 Ray = AtmosphereDeltaSRTexture.Sample(SamplerLinearClamp, UVW);
    float4 Mie = AtmosphereDeltaSRTexture.Sample(SamplerLinearClamp, UVW);
	return float4(Ray.rgb, Mie.r);
}

// For Inscatter S
void Inscatter(float Radius, float Mu, float MuS, float Nu, out float3 RayMie) 
{
	Radius = clamp(Radius, RadiusGround, RadiusAtmosphere);
	Mu = clamp(Mu, -1.0, 1.0);
	MuS = clamp(MuS, -1.0, 1.0);
	float variation = sqrt(1.0 - Mu * Mu) * sqrt(1.0 - MuS * MuS);
	Nu = clamp(Nu, MuS * Mu - variation, MuS * Mu + variation);

	float cThetaMin = -sqrt(1.0 - (RadiusGround / Radius) * (RadiusGround / Radius));

	float3 V = float3(sqrt(1.0 - Mu * Mu), 0.0, Mu);
	float Sx = V.x == 0.0 ? 0.0 : (Nu - MuS * Mu) / V.x;
	float3 S = float3(Sx, sqrt(max(0.0, 1.0 - Sx * Sx - MuS * MuS)), MuS);

	RayMie = float3(0.f, 0.f, 0.f);

	// Integral over 4.PI around x with two nested loops over W directions (theta, phi) - Eq (7)
	for (int iTheta = 0; iTheta < InscatterSphericalIntegralSamples; iTheta++)
	{
		float theta = (float(iTheta) + 0.5) * InscatterDeltaTheta;
		float cTheta = cos(theta);
		
		float GReflectance = 0.0;
		float DGround = 0.0;
		float3 GTransmittance = float3(0.f, 0.f, 0.f);
		if (cTheta < cThetaMin)
		{ 
			// If ground visible in direction W, Compute transparency GTransmittance between x and ground
			GReflectance = AverageGroundRelectance / PI;
			DGround = -Radius * cTheta - sqrt(Radius * Radius * (cTheta * cTheta - 1.0) + RadiusGround * RadiusGround);
			GTransmittance = TransmittanceWithDistance(RadiusGround, -(Radius * cTheta + DGround) / RadiusGround, DGround);
		}
		
		for (int iPhi = 0; iPhi < 2 * InscatterSphericalIntegralSamples; iPhi++)
		{
			float phi = (float(iPhi) + 0.5) * InscatterDeltaPhi;
			float Dw = InscatterDeltaTheta * InscatterDeltaPhi * sin(theta);
			float3 W = float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cTheta);

			float Nu1 = dot(S, W);
			float Nu2 = dot(V, W);
			float Pr2 = PhaseFunctionR(Nu2);
			float Pm2 = PhaseFunctionM(Nu2);

			// Compute irradiance received at ground in direction W (if ground visible) =deltaE
			float3 GNormal = (float3(0.0, 0.0, Radius) + DGround * W) / RadiusGround;
			float3 GIrradiance = Irradiance(AtmosphereDeltaETexture, RadiusGround, dot(GNormal, S));

			float3 RayMie1; // light arriving at x from direction W

			// First term = light reflected from the ground and attenuated before reaching x, =T.alpha/PI.deltaE
			RayMie1 = GReflectance * GIrradiance * GTransmittance;

			// Second term = inscattered light, =deltaS
			if (FirstOrder == 1.0) 
			{
				// First iteration is special because Rayleigh and Mie were stored separately,
				// without the phase functions factors; they must be reintroduced here
				float Pr1 = PhaseFunctionR(Nu1);
				float Pm1 = PhaseFunctionM(Nu1);
				float3 Ray1 = Texture4DSample(AtmosphereDeltaSRTexture, Radius, W.z, MuS, Nu1).rgb;
				float3 Mie1 = Texture4DSample(AtmosphereDeltaSMTexture, Radius, W.z, MuS, Nu1).rgb;
				RayMie1 += Ray1 * Pr1 + Mie1 * Pm1;
			} 
			else 
			{
				RayMie1 += Texture4DSample(AtmosphereDeltaSRTexture, Radius, W.z, MuS, Nu1).rgb;
			}

			// Light coming from direction W and scattered in direction V
			// = light arriving at x from direction W (RayMie1) * SUM(scattering coefficient * phaseFunction) - Eq (7)
			RayMie += RayMie1 * (BetaRayleighScattering * exp(-(Radius - RadiusGround) / HeightScaleRayleigh) * Pr2 + BetaMieScattering * exp(-(Radius - RadiusGround) / HeightScaleMie) * Pm2) * Dw;
		}
	}

	// output RayMie = J[T.alpha/PI.deltaE + deltaS] (line 7 in algorithm 4.1)
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_InscatterS(AtmosphereGSOutput input) : SV_Target0
{
	float3 RayMie;
	float Mu, MuS, Nu;
	GetMuMuSNu(input.TexCoord, AtmosphereR, DhdH, Mu, MuS, Nu);
	Inscatter(AtmosphereR, Mu, MuS, Nu, RayMie);
	return float4(RayMie, 0);
}

float3 Integrand(float Radius, float Mu, float MuS, float Nu, float T) 
{
    float Ri = sqrt(Radius * Radius + T * T + 2.0 * Radius * Mu * T);
    float Mui = (Radius * Mu + T) / Ri;
    float MuSi = (Nu * T + MuS * Radius) / Ri;
    return Texture4DSample(AtmosphereDeltaJTexture, Ri, Mui, MuSi, Nu).rgb * TransmittanceWithDistance(Radius, Mu, T);
}

// InscatterN
float3 Inscatter(float Radius, float Mu, float MuS, float Nu) 
{
	float3 RayMie = float3(0.f, 0.f, 0.f);
	float Dx = Limit(Radius, Mu) / float(InscatterIntegralSamples);
	float Xi = 0.0;
	float3 RayMiei = Integrand(Radius, Mu, MuS, Nu, 0.0);

	for (int i = 1; i <= InscatterIntegralSamples; i++) 
	{
		float Xj = float(i) * Dx;
		float3 RayMiej = Integrand(Radius, Mu, MuS, Nu, Xj);
		RayMie += (RayMiei + RayMiej) / 2.0 * Dx;
		Xi = Xj;
		RayMiei = RayMiej;
	}

	return RayMie;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_InscatterN(AtmosphereGSOutput input) : SV_Target0
{
	float Mu, MuS, Nu;
	GetMuMuSNu(input.TexCoord, AtmosphereR, DhdH, Mu, MuS, Nu);
	return float4(Inscatter(AtmosphereR, Mu, MuS, Nu), 0);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CopyInscatterN(AtmosphereGSOutput input) : SV_Target0
{
    float Mu, MuS, Nu;
    GetMuMuSNu(input.TexCoord, AtmosphereR, DhdH, Mu, MuS, Nu);
	float3 UVW = float3(input.TexCoord, (float(AtmosphereLayer) + 0.5f) / float(AtmosphericFogInscatterAltitudeSampleNum));
	float4 Ray = AtmosphereDeltaSRTexture.Sample(SamplerLinearClamp, UVW) / PhaseFunctionR(Nu);
	return float4(Ray.rgb, 0);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Inscatter1_B(AtmosphereGSOutput input) : SV_Target
{
    float3 Ray;
    float3 Mie;
    float Mu, MuS, Nu;
    GetMuMuSNu(input.TexCoord, AtmosphereR, DhdH, Mu, MuS, Nu);
    Inscatter(AtmosphereR, Mu, MuS, Nu, Ray, Mie);
    return float4(Mie, 1);
}
