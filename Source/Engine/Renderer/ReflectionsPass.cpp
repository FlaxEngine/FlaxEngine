// Copyright (c) Wojciech Figat. All rights reserved.

#include "ReflectionsPass.h"
#include "GBufferPass.h"
#include "RenderList.h"
#include "ScreenSpaceReflectionsPass.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Level/Actors/EnvironmentProbe.h"

GPU_CB_STRUCT(Data {
    ShaderEnvProbeData PData;
    Matrix WVP;
    ShaderGBufferData GBuffer;
    });

#if GENERATE_GF_CACHE

// This code below (PreIntegratedGF namespace) is based on many Siggraph presentations about BRDF shading:
// https://blog.selfshadow.com/publications/s2015-shading-course/
// https://blog.selfshadow.com/publications/s2012-shading-course/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>

namespace PreIntegratedGF
{
	static const int Resolution = 128;
	static const int NumSamples = 512;

	struct vec2 {
		double x, y;
		vec2(double _x, double _y) :x(_x), y(_y) { };

		vec2& operator /=(const double& b)
		{
			x /= b;
			y /= b;
			return *this;
		}
	};

	struct ivec2 {
		int x, y;
	};

	struct vec3 {
		double x, y, z;
		vec3(double _x, double _y, double _z) :x(_x), y(_y), z(_z) { };

		double dot(const vec3& b)
		{
			return x*b.x + y*b.y + z*b.z;
		}
	};

	vec3 operator*(const double& a, const vec3& b)
	{
		return vec3(b.x * a, b.y * a, b.z * a);
	}
	vec3 operator-(const vec3& a, const vec3& b)
	{
		return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
	}

	inline double saturate(double x)
	{
		if (x < 0) x = 0;
		if (x > 1) x = 1;
		return x;
	}

	unsigned int ReverseBits32(unsigned int bits)
	{
		bits = (bits << 16) | (bits >> 16);
		bits = ((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >> 8);
		bits = ((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >> 4);
		bits = ((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >> 2);
		bits = ((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >> 1);
		return bits;
	}

	inline double rand_0_1()
	{
		return 1.0 * rand() / RAND_MAX;
	}

	inline unsigned int rand_32bit()
	{
		unsigned int x = rand() & 0xff;
		x |= (rand() & 0xff) << 8;
		x |= (rand() & 0xff) << 16;
		x |= (rand() & 0xff) << 24;
		return x;
	}

	// using uniform randomness :(
	double t1 = rand_0_1();
	unsigned int t2 = rand_32bit();
	vec2 Hammersley(int Index, int NumSamples)
	{
		double E1 = 1.0 * Index / NumSamples + t1;
		E1 = E1 - int(E1);
		double E2 = double(ReverseBits32(Index) ^ t2) * 2.3283064365386963e-10;
		return vec2(E1, E2);
	}

	vec3 ImportanceSampleGGX(vec2 E, double Roughness)
	{
		double m = Roughness * Roughness;
		double m2 = m * m;

		double phi = 2 * PI * E.x;
		double cosTheta = sqrt((1 - E.y) / (1 + (m2 - 1) * E.y));
		double sinTheta = sqrt(1 - cosTheta * cosTheta);

		vec3 H(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

		double d = (cosTheta * m2 - cosTheta) * cosTheta + 1;
		double D = m2 / (M_PI*d*d);
		double PDF = D * cosTheta;

		return H;
	}

	double Vis_SmithJointApprox(double Roughness, double NoV, double NoL)
	{
		double a = Roughness * Roughness;
		double Vis_SmithV = NoL * (NoV * (1 - a) + a);
		double Vis_SmithL = NoV * (NoL * (1 - a) + a);
		return 0.5 / (Vis_SmithV + Vis_SmithL);
	}

	vec2 IntegrateBRDF(double Roughness, double NoV)
	{
		if (Roughness < 0.04) Roughness = 0.04;

		vec3 V(sqrt(1 - NoV*NoV), 0, NoV);
		double A = 0, B = 0;
		for (int i = 0; i < NumSamples; i++)
		{
			vec2 E = Hammersley(i, NumSamples);
			vec3 H = ImportanceSampleGGX(E, Roughness);
			vec3 L = 2 * V.dot(H) * H - V;

			double NoL = saturate(L.z);
			double NoH = saturate(H.z);
			double VoH = saturate(V.dot(H));

			if (NoL > 0)
			{
				double Vis = Vis_SmithJointApprox(Roughness, NoV, NoL);

				double a = Roughness * Roughness;
				double a2 = a*a;
				double Vis_SmithV = NoL * sqrt(NoV * (NoV - NoV * a2) + a2);
				double Vis_SmithL = NoV * sqrt(NoL * (NoL - NoL * a2) + a2);

				double NoL_Vis_PDF = NoL * Vis * (4 * VoH / NoH);

				double Fc = pow(1 - VoH, 5);
				A += (1 - Fc) * NoL_Vis_PDF;
				B += Fc * NoL_Vis_PDF;
			}
		}
		vec2 res(A, B);
		res /= NumSamples;
		return res;
	}

	void Generate()
	{
		String path = Globals::TemporaryFolder / TEXT("PreIntegratedGF.bmp");

		FILE* pFile = fopen(path.ToSTD().c_str(), "wb");

		byte data[Resolution * 3 * Resolution];
		int c = 0;
		for (int x = 0; x < Resolution; x++)
		{
			for (int y = 0; y < Resolution; y++)
			{
				vec2 brdf = IntegrateBRDF(1 - 1.0 * x / (Resolution - 1), 1.0 * y / (Resolution - 1));
				data[c + 2] = byte(brdf.x * 255);
				data[c + 1] = byte(brdf.y * 255);
				data[c + 0] = 0;
				c += 3;
			}
		}

		BITMAPINFOHEADER BMIH;
		BMIH.biSize = sizeof(BITMAPINFOHEADER);
		BMIH.biSizeImage = Resolution * Resolution * 3;
		BMIH.biSize = sizeof(BITMAPINFOHEADER);
		BMIH.biWidth = Resolution;
		BMIH.biHeight = Resolution;
		BMIH.biPlanes = 1;
		BMIH.biBitCount = 24;
		BMIH.biCompression = BI_RGB;
		BMIH.biSizeImage = Resolution * Resolution * 3;

		BITMAPFILEHEADER bmfh;
		int nBitsOffset = sizeof(BITMAPFILEHEADER) + BMIH.biSize;
		LONG lImageSize = BMIH.biSizeImage;
		LONG lFileSize = nBitsOffset + lImageSize;

		bmfh.bfType = 'B' + ('M' << 8);
		bmfh.bfOffBits = nBitsOffset;
		bmfh.bfSize = lFileSize;
		bmfh.bfReserved1 = bmfh.bfReserved2 = 0;

		//Write the bitmap file header
		UINT nWrittenFileHeaderSize = fwrite(&bmfh, 1, sizeof(BITMAPFILEHEADER), pFile);

		//And then the bitmap info header
		UINT nWrittenInfoHeaderSize = fwrite(&BMIH, 1, sizeof(BITMAPINFOHEADER), pFile);

		//Finally, write the image data itself 

		//-- the data represents our drawing
		UINT nWrittenDIBDataSize = fwrite(data, 1, lImageSize, pFile);

		fclose(pFile);

		Guid id;
		Importers::TextureImportArgument arg;
		arg.Options.Type = FormatType::HdrRGB;
		arg.Options.IndependentChannels = true;
		arg.Options.IsAtlas = false;
		arg.Options.IsSRGB = false;
		arg.Options.NeverStream = true;
		Content::Import(path, Globals:... + PRE_INTEGRATED_GF_ASSET_NAME, &id, &arg);
	}
};

#endif

class Model;

String ReflectionsPass::ToString() const
{
    return TEXT("ReflectionsPass");
}

bool ReflectionsPass::Init()
{
#if GENERATE_GF_CACHE
	// Generate cache
	PreIntegratedGF::Generate();
#endif

    // Create pipeline states
    _psProbe = GPUDevice::Instance->CreatePipelineState();
    _psProbeInside = GPUDevice::Instance->CreatePipelineState();
    _psCombinePass = GPUDevice::Instance->CreatePipelineState();

    // Load assets
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Reflections"));
    _sphereModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/Sphere"));
    _preIntegratedGF = Content::LoadAsyncInternal<Texture>(PRE_INTEGRATED_GF_ASSET_NAME);
    if (_shader == nullptr || _sphereModel == nullptr || _preIntegratedGF == nullptr)
        return true;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<ReflectionsPass, &ReflectionsPass::OnShaderReloading>(this);
#endif

    return false;
}

bool ReflectionsPass::setupResources()
{
    // Wait for the assets
    if (!_sphereModel->CanBeRendered() || !_preIntegratedGF->IsLoaded() || !_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();

    // Validate shader constant buffer size
    if (shader->GetCB(0)->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }

    // Create pipeline stages
    GPUPipelineState::Description psDesc;
    if (!_psProbe->IsValid())
    {
        psDesc = GPUPipelineState::Description::DefaultNoDepth;
        psDesc.BlendMode = BlendingMode::AlphaBlend;
        psDesc.VS = shader->GetVS("VS_Model");
        psDesc.PS = shader->GetPS("PS_EnvProbe");
        psDesc.CullMode = CullMode::Normal;
        psDesc.DepthEnable = true;
        if (_psProbe->Init(psDesc))
            return true;
        psDesc.DepthFunc = ComparisonFunc::Greater;
        psDesc.CullMode = CullMode::Inverted;
        if (_psProbeInside->Init(psDesc))
            return true;
    }
    psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
    if (!_psCombinePass->IsValid())
    {
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        psDesc.PS = shader->GetPS("PS_CombinePass");
        if (_psCombinePass->Init(psDesc))
            return true;
    }

    return false;
}

void ReflectionsPass::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_psProbe);
    SAFE_DELETE_GPU_RESOURCE(_psProbeInside);
    SAFE_DELETE_GPU_RESOURCE(_psCombinePass);
    _shader = nullptr;
    _sphereModel = nullptr;
    _preIntegratedGF = nullptr;
}

bool SortProbes(RenderEnvironmentProbeData const& p1, RenderEnvironmentProbeData const& p2)
{
    // Compare by Sort Order
    int32 res = p1.SortOrder - p2.SortOrder;
    if (res == 0)
    {
        // Compare by radius
        res = static_cast<int32>(p2.Radius - p1.Radius);
        if (res == 0)
        {
            // Compare by ID to prevent flickering
            res = p2.HashID - p1.HashID;
        }
    }
    return res < 0;
}

void ReflectionsPass::Render(RenderContext& renderContext, GPUTextureView* lightBuffer)
{
    auto device = GPUDevice::Instance;
    auto context = device->GetMainContext();
    if (checkIfSkipPass())
    {
        // Skip pass (just clear buffer when doing debug preview)
        if (renderContext.View.Mode == ViewMode::Reflections)
            context->Clear(lightBuffer, Color::Black);
        return;
    }

    // Cache data
    auto& view = renderContext.View;
    bool useReflections = EnumHasAnyFlags(view.Flags, ViewFlags::Reflections);
    bool useSSR = EnumHasAnyFlags(view.Flags, ViewFlags::SSR) && renderContext.List->Settings.ScreenSpaceReflections.Intensity > ZeroTolerance;
    int32 probesCount = renderContext.List->EnvironmentProbes.Count();
    bool renderProbes = probesCount > 0;
    auto shader = _shader->GetShader();
    auto cb = shader->GetCB(0);

    // Check if no need to render reflection environment
    if (!useReflections || !(renderProbes || useSSR))
        return;

    // Setup data
    Data data;
    GBufferPass::SetInputs(view, data.GBuffer);

    // Bind GBuffer inputs
    context->BindSR(0, renderContext.Buffers->GBuffer0);
    context->BindSR(1, renderContext.Buffers->GBuffer1);
    context->BindSR(2, renderContext.Buffers->GBuffer2);
    context->BindSR(3, renderContext.Buffers->DepthBuffer);

    auto tempDesc = GPUTextureDescription::New2D(renderContext.Buffers->GetWidth(), renderContext.Buffers->GetHeight(), PixelFormat::R11G11B10_Float);
    auto reflectionsBuffer = RenderTargetPool::Get(tempDesc);
    RENDER_TARGET_POOL_SET_NAME(reflectionsBuffer, "Reflections");
    context->Clear(*reflectionsBuffer, Color::Black);

    // Reflection Probes pass
    if (renderProbes)
    {
        PROFILE_GPU_CPU("Env Probes");

        context->SetRenderTarget(*reflectionsBuffer);

        // Sort probes by the radius
        Sorting::QuickSort(renderContext.List->EnvironmentProbes.Get(), renderContext.List->EnvironmentProbes.Count(), &SortProbes);

        // Render all env probes
        auto& sphereMesh = _sphereModel->LODs.Get()[0].Meshes.Get()[0];
        for (int32 i = 0; i < probesCount; i++)
        {
            const RenderEnvironmentProbeData& probe = renderContext.List->EnvironmentProbes.Get()[i];

            // Calculate world view projection matrix for the light sphere
            Matrix world, wvp;
            bool isViewInside;
            RenderTools::ComputeSphereModelDrawMatrix(renderContext.View, probe.Position, probe.Radius, world, isViewInside);
            Matrix::Multiply(world, view.ViewProjection(), wvp);

            // Pack probe properties buffer
            probe.SetShaderData(data.PData);
            Matrix::Transpose(wvp, data.WVP);

            // Render reflections
            context->UpdateCB(cb, &data);
            context->BindCB(0, cb);
            context->BindSR(4, probe.Texture);
            context->SetState(isViewInside ? _psProbeInside : _psProbe);
            sphereMesh.Render(context);
        }

        context->UnBindSR(4);
        context->ResetRenderTarget();
    }

    // Screen Space Reflections pass
    if (useSSR)
    {
        ScreenSpaceReflectionsPass::Instance()->Render(renderContext, *reflectionsBuffer, lightBuffer);
        context->SetViewportAndScissors(renderContext.Task->GetViewport());

        /*
        // DEBUG_CODE
        context->RestoreViewport();
        context->SetRenderTarget(output);
        context->Draw(reflectionsRT);
        return;
        // DEBUG_CODE
        */
    }

    if (renderContext.View.Mode == ViewMode::Reflections)
    {
        // Override light buffer with the reflections buffer
        context->SetRenderTarget(lightBuffer);
        context->Draw(reflectionsBuffer);
    }
    else
    {
        // Combine reflections and light buffer (additive mode)
        context->SetRenderTarget(lightBuffer);
        context->BindCB(0, cb);
        if (probesCount == 0 || !renderProbes)
        {
            context->UpdateCB(cb, &data);
        }
        if (useSSR)
        {
            context->BindSR(0, renderContext.Buffers->GBuffer0);
            context->BindSR(1, renderContext.Buffers->GBuffer1);
            context->BindSR(2, renderContext.Buffers->GBuffer2);
            context->BindSR(3, renderContext.Buffers->DepthBuffer);
        }
        context->BindSR(5, reflectionsBuffer);
        context->BindSR(6, _preIntegratedGF->GetTexture());
        context->SetState(_psCombinePass);
        context->DrawFullscreenTriangle();
    }

    RenderTargetPool::Release(reflectionsBuffer);
}
