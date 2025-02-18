// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ParticleEmitterGraph.CPU.h"
#include "Engine/Core/Random.h"
#include "Engine/Utilities/Noise.h"
#include "Engine/Core/Types/CommonValue.h"

// ReSharper disable CppCStyleCast
// ReSharper disable CppClangTidyClangDiagnosticCastAlign
// ReSharper disable CppDefaultCaseNotHandledInSwitchStatement
// ReSharper disable CppClangTidyCppcoreguidelinesMacroUsage
// ReSharper disable CppClangTidyClangDiagnosticOldStyleCast

#define RAND Random::Rand()
#define RAND2 Float2(RAND, RAND)
#define RAND3 Float3(RAND, RAND, RAND)
#define RAND4 Float4(RAND, RAND, RAND, RAND)

// Enable to insert CPU profiler events for particles modules
#define PARTICLE_EMITTER_MODULES_PROFILE 0
#if PARTICLE_EMITTER_MODULES_PROFILE
#include "Engine/Profiler/ProfilerCPU.h"
#define PARTICLE_EMITTER_MODULE(name) PROFILE_CPU_NAMED(name)
#else
#define PARTICLE_EMITTER_MODULE(name)
#endif

namespace
{
    VariantType::Types GetVariantType(ParticleAttribute::ValueTypes type)
    {
        switch (type)
        {
        case ParticleAttribute::ValueTypes::Float2:
            return VariantType::Float2;
        case ParticleAttribute::ValueTypes::Float3:
            return VariantType::Float3;
        case ParticleAttribute::ValueTypes::Float4:
            return VariantType::Float4;
        case ParticleAttribute::ValueTypes::Float:
            return VariantType::Float;
        case ParticleAttribute::ValueTypes::Int:
            return VariantType::Int;
        case ParticleAttribute::ValueTypes::Uint:
            return VariantType::Uint;
        default:
            return VariantType::Pointer;
        }
    }
}

int32 ParticleEmitterGraphCPUExecutor::ProcessSpawnModule(int32 index)
{
    const auto node = _graph.SpawnModules[index];
    auto& context = *Context.Get();
    auto& data = context.Data->SpawnModulesData[index];

    // Accumulate the previous frame fraction
    float spawnCount = data.SpawnCounter;

    // Calculate particles to spawn during this frame
    switch (node->TypeID)
    {
    // Constant Spawn Rate
    case 100:
    {
        const float rate = Math::Max((float)TryGetValue(node->GetBox(0), node->Values[2]), 0.0f);
        spawnCount += rate * context.DeltaTime;
        break;
    }
    // Single Burst
    case 101:
    {
        const bool isFirstUpdate = (context.Data->Time - context.DeltaTime) <= 0.0f;
        if (isFirstUpdate)
        {
            const float count = Math::Max((float)TryGetValue(node->GetBox(0), node->Values[2]), 0.0f);
            spawnCount += count;
        }
        break;
    }
    // Periodic
    case 102:
    {
        float& nextSpawnTime = data.NextSpawnTime;
        if (nextSpawnTime - context.Data->Time <= 0.0f)
        {
            const float count = Math::Max((float)TryGetValue(node->GetBox(0), node->Values[2]), 0.0f);
            const float delay = Math::Max((float)TryGetValue(node->GetBox(1), node->Values[3]), 0.0f);
            nextSpawnTime = context.Data->Time + delay;
            spawnCount += count;
        }
        break;
    }
    // Periodic Burst (range)
    case 103:
    {
        float& nextSpawnTime = data.NextSpawnTime;
        if (nextSpawnTime - context.Data->Time <= 0.0f)
        {
            const Float2 countMinMax = (Float2)TryGetValue(node->GetBox(0), node->Values[2]);
            const Float2 delayMinMax = (Float2)TryGetValue(node->GetBox(1), node->Values[3]);
            const float count = Math::Max(countMinMax.X + RAND * (countMinMax.Y - countMinMax.X), 0.0f);
            const float delay = Math::Max(delayMinMax.X + RAND * (delayMinMax.Y - delayMinMax.X), 0.0f);
            nextSpawnTime = context.Data->Time + delay;
            spawnCount += count;
        }
        break;
    }
    }

    // Calculate actual spawn amount
    spawnCount = Math::Max(spawnCount, 0.0f);
    const int32 result = Math::FloorToInt(spawnCount);
    spawnCount -= (float)result;
    data.SpawnCounter = spawnCount;

    return result;
}

void ParticleEmitterGraphCPUExecutor::ProcessModule(ParticleEmitterGraphCPUNode* node, int32 particlesStart, int32 particlesEnd)
{
    auto& context = *Context.Get();
    auto stride = context.Data->Buffer->Stride;
    auto start = context.Data->Buffer->GetParticleCPU(particlesStart);

    switch (node->TypeID)
    {
    // Orient Sprite
    case 201:
    case 303:
    {
        PARTICLE_EMITTER_MODULE("Orient Sprite");
        auto spriteFacingMode = node->Values[2].AsInt;
        {
            auto& attribute = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];
            byte* spriteFacingModePtr = start + attribute.Offset;
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                *((int32*)spriteFacingModePtr) = spriteFacingMode;
                spriteFacingModePtr += stride;
            }
        }
        if ((ParticleSpriteFacingMode)spriteFacingMode == ParticleSpriteFacingMode::CustomFacingVector ||
            (ParticleSpriteFacingMode)spriteFacingMode == ParticleSpriteFacingMode::FixedAxis)
        {
            auto& attribute = context.Data->Buffer->Layout->Attributes[node->Attributes[1]];
            byte* customFacingVectorPtr = start + attribute.Offset;
            auto box = node->GetBox(0);
            if (node->UsePerParticleDataResolve())
            {
                for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
                {
                    context.ParticleIndex = particleIndex;
                    const Float3 vector = (Float3)GetValue(box, 3);
                    *((Float3*)customFacingVectorPtr) = vector;
                    customFacingVectorPtr += stride;
                }
            }
            else
            {
                const Float3 vector = (Float3)GetValue(box, 3);
                for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
                {
                    *((Float3*)customFacingVectorPtr) = vector;
                    customFacingVectorPtr += stride;
                }
            }
        }
        break;
    }
    // Orient Model
    case 213:
    case 309:
    {
        PARTICLE_EMITTER_MODULE("Orient Model");
        auto modelFacingMode = node->Values[2].AsInt;
        {
            auto& attribute = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];
            byte* modelFacingModePtr = start + attribute.Offset;
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                *((int32*)modelFacingModePtr) = modelFacingMode;
                modelFacingModePtr += stride;
            }
        }
        break;
    }
    // Update Age
    case 300:
    {
        PARTICLE_EMITTER_MODULE("Update Age");
        auto& attribute = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];
        byte* agePtr = start + attribute.Offset;
        for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
        {
            *((float*)agePtr) += context.DeltaTime;
            agePtr += stride;
        }
        break;
    }
    // Gravity/Force
    case 301:
    case 304:
    {
        PARTICLE_EMITTER_MODULE("Gravity/Force");
        auto& attribute = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];
        byte* velocityPtr = start + attribute.Offset;
        auto box = node->GetBox(0);
        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                const Float3 force = (Float3)GetValue(box, 2);
                *((Float3*)velocityPtr) += force * context.DeltaTime;
                velocityPtr += stride;
            }
        }
        else
        {
            const Float3 force = (Float3)GetValue(box, 2);
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                *((Float3*)velocityPtr) += force * context.DeltaTime;
                velocityPtr += stride;
            }
        }
        break;
    }
    // Conform to Sphere
    case 305:
    {
        PARTICLE_EMITTER_MODULE("Conform to Sphere");
        auto& position = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];
        auto& velocity = context.Data->Buffer->Layout->Attributes[node->Attributes[1]];
        auto& mass = context.Data->Buffer->Layout->Attributes[node->Attributes[2]];

        byte* positionPtr = start + position.Offset;
        byte* velocityPtr = start + velocity.Offset;
        byte* massPtr = start + mass.Offset;

        auto sphereCenterBox = node->GetBox(0);
        auto sphereRadiusBox = node->GetBox(1);
        auto attractionSpeedBox = node->GetBox(2);
        auto attractionForceBox = node->GetBox(3);
        auto stickDistanceBox = node->GetBox(4);
        auto stickForceBox = node->GetBox(5);

#define INPUTS_FETCH() \
	const Float3 sphereCenter = (Float3)GetValue(sphereCenterBox, 2); \
	const float sphereRadius = (float)GetValue(sphereRadiusBox, 3); \
	const float attractionSpeed = (float)GetValue(attractionSpeedBox, 4); \
	const float attractionForce = (float)GetValue(attractionForceBox, 5); \
	const float stickDistance = (float)GetValue(stickDistanceBox, 6); \
	const float stickForce = (float)GetValue(stickForceBox, 7)
#define LOGIC() \
	Float3 dir = sphereCenter - *(Float3*)positionPtr; \
	float distToCenter = dir.Length(); \
	float distToSurface = distToCenter - sphereRadius; \
	dir /= Math::Max(0.0001f, distToCenter); \
	Float3 velocity = *(Float3*)velocityPtr; \
	float spdNormal = Float3::Dot(dir, velocity); \
	float ratio = Math::SmoothStep(0.0f, stickDistance * 2.0f, Math::Abs(distToSurface)); \
	float tgtSpeed = Math::Sign(distToSurface) * attractionSpeed * ratio; \
	float deltaSpeed = tgtSpeed - spdNormal; \
	Float3 deltaVelocity = dir * (Math::Sign(deltaSpeed) * Math::Min(Math::Abs(deltaSpeed), context.DeltaTime * Math::Lerp(stickForce, attractionForce, ratio)) / Math::Max(*(float*)massPtr, ZeroTolerance)); \
	*(Float3*)velocityPtr = velocity + deltaVelocity; \
	positionPtr += stride; \
	velocityPtr += stride; \
	massPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Kill (sphere)
    case 306:
    {
        PARTICLE_EMITTER_MODULE("Kill");
        auto& position = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + position.Offset;

        auto sphereCenterBox = node->GetBox(0);
        auto sphereRadiusBox = node->GetBox(1);
        auto sign = (bool)node->Values[4] ? -1.0f : 1.0f;

#define INPUTS_FETCH() \
	const Float3 sphereCenter = (Float3)GetValue(sphereCenterBox, 2); \
	const float sphereRadius = (float)GetValue(sphereRadiusBox, 3); \
	const float sphereRadiusSqr = sphereRadius * sphereRadius
#define LOGIC() \
	Float3 dir = *(Float3*)positionPtr - sphereCenter; \
	float lengthSqr = Float3::Dot(dir, dir); \
	if (sign * lengthSqr <= sign * sphereRadiusSqr) \
	{ \
		particlesEnd--; \
		context.Data->Buffer->CPU.Count--; \
		Platform::MemoryCopy(context.Data->Buffer->GetParticleCPU(particleIndex), context.Data->Buffer->GetParticleCPU(context.Data->Buffer->CPU.Count), context.Data->Buffer->Stride); \
		particleIndex--; \
	} \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Kill (box)
    case 307:
    {
        PARTICLE_EMITTER_MODULE("Kill");
        auto& position = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + position.Offset;

        auto boxCenterBox = node->GetBox(0);
        auto boxSizeBox = node->GetBox(1);
        auto invert = (bool)node->Values[4];

#define INPUTS_FETCH() \
	const Float3 boxCenter = (Float3)GetValue(boxCenterBox, 2); \
	const Float3 boxSize = (Float3)GetValue(boxSizeBox, 3)
#define LOGIC() \
	Float3 dir = *(Float3*)positionPtr - boxCenter; \
	Float3 absDir = Float3::Abs(dir); \
	Float3 size = boxSize * 0.5f; \
	bool collision; \
	if (invert) \
		collision = absDir.X >= size.X || absDir.Y >= size.Y || absDir.Z >= size.Z; \
	else \
		collision = absDir.X <= size.X && absDir.Y <= size.Y && absDir.Z <= size.Z; \
	if (collision) \
	{ \
		particlesEnd--; \
		context.Data->Buffer->CPU.Count--; \
		Platform::MemoryCopy(context.Data->Buffer->GetParticleCPU(particleIndex), context.Data->Buffer->GetParticleCPU(context.Data->Buffer->CPU.Count), context.Data->Buffer->Stride); \
		particleIndex--; \
	} \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Kill (custom)
    case 308:
    {
        PARTICLE_EMITTER_MODULE("Kill (custom)");
        auto killBox = node->GetBox(0);

#define INPUTS_FETCH() \
	const bool kill = (bool)TryGetValue(killBox, Value::False)
#define LOGIC() \
	if (kill) \
	{ \
		particlesEnd--; \
		context.Data->Buffer->CPU.Count--; \
		Platform::MemoryCopy(context.Data->Buffer->GetParticleCPU(particleIndex), context.Data->Buffer->GetParticleCPU(context.Data->Buffer->CPU.Count), context.Data->Buffer->Stride); \
		particleIndex--; \
	}

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Linear Drag
    case 310:
    {
        PARTICLE_EMITTER_MODULE("Linear Drag");
        auto box = node->GetBox(0);
        const bool useSpriteSize = node->Values[3].AsBool;

        auto& velocity = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];
        auto& mass = context.Data->Buffer->Layout->Attributes[node->Attributes[1]];
        byte* spriteSizePtr = useSpriteSize ? start + context.Data->Buffer->Layout->Attributes[node->Attributes[2]].Offset : nullptr;

        byte* velocityPtr = start + velocity.Offset;
        byte* massPtr = start + mass.Offset;

#define INPUTS_FETCH() \
	const float drag = (float)GetValue(box, 2)
#define LOGIC() \
	float particleDrag = drag; \
    if (useSpriteSize) \
        particleDrag *= ((Float2*)spriteSizePtr)->MulValues(); \
    *((Float3*)velocityPtr) *= Math::Max(0.0f, 1.0f - (particleDrag * context.DeltaTime) / Math::Max(*(float*)massPtr, ZeroTolerance)); \
    velocityPtr += stride; \
    massPtr += stride; \
    spriteSizePtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Turbulence
    case 311:
    {
        PARTICLE_EMITTER_MODULE("Turbulence");
        auto& position = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];
        auto& velocity = context.Data->Buffer->Layout->Attributes[node->Attributes[1]];
        auto& mass = context.Data->Buffer->Layout->Attributes[node->Attributes[2]];

        byte* positionPtr = start + position.Offset;
        byte* velocityPtr = start + velocity.Offset;
        byte* massPtr = start + mass.Offset;

        auto roughnessBox = node->GetBox(3);
        auto intensityBox = node->GetBox(4);
        auto octavesCountBox = node->GetBox(5);

        const Float3 fieldPosition = (Float3)GetValue(node->GetBox(0), 2);
        const Float3 fieldRotation = (Float3)GetValue(node->GetBox(1), 3);
        const Float3 fieldScale = (Float3)GetValue(node->GetBox(2), 4);

        // Note: no support for per-particle transformation
        Matrix fieldTransformMatrix, invFieldTransformMatrix;
        Transform fieldTransform(fieldPosition, Quaternion::Euler(fieldRotation), fieldScale);
        fieldTransform.GetWorld(fieldTransformMatrix);
        Matrix::Invert(fieldTransformMatrix, invFieldTransformMatrix);

#define INPUTS_FETCH() \
	const float roughness = (float)GetValue(roughnessBox, 5); \
	const float intensity = (float)GetValue(intensityBox, 6); \
	const int32 octavesCount = (int)GetValue(octavesCountBox, 7)
#define LOGIC() \
	Float3 vectorFieldUVW = Float3::Transform(*((Float3*)positionPtr), invFieldTransformMatrix); \
	Float3 force = Noise::CustomNoise3D(vectorFieldUVW + 0.5f, octavesCount, roughness); \
    force = Float3::Transform(force, fieldTransformMatrix) * intensity; \
    *((Float3*)velocityPtr) += force * (context.DeltaTime / Math::Max(*(float*)massPtr, ZeroTolerance)); \
    positionPtr += stride; \
    velocityPtr += stride; \
    massPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Set Attribute
    case 200:
    case 302:
    {
        PARTICLE_EMITTER_MODULE("Set Attribute");
        auto& attribute = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];
        byte* dataPtr = start + attribute.Offset;
        int32 dataSize = attribute.GetSize();
        auto box = node->GetBox(0);
        ValueType type(GetVariantType(attribute.ValueType));
        if (node->UsePerParticleDataResolve())
        {
            Value value;
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                value = GetValue(box, 4).Cast(type);
                Platform::MemoryCopy(dataPtr, &value.AsPointer, dataSize);
                dataPtr += stride;
            }
        }
        else
        {
            const Value value = GetValue(box, 4).Cast(type);
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                Platform::MemoryCopy(dataPtr, &value.AsPointer, dataSize);
                dataPtr += stride;
            }
        }
        break;
    }
    // Set Position/Lifetime/Age/..
    case 250:
    case 251:
    case 252:
    case 253:
    case 254:
    case 255:
    case 256:
    case 257:
    case 258:
    case 259:
    case 260:
    case 261:
    case 262:
    case 263:
    case 350:
    case 351:
    case 352:
    case 353:
    case 354:
    case 355:
    case 356:
    case 357:
    case 358:
    case 359:
    case 360:
    case 361:
    case 362:
    case 363:
    {
        PARTICLE_EMITTER_MODULE("Set");
        auto& attribute = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];
        byte* dataPtr = start + attribute.Offset;
        int32 dataSize = attribute.GetSize();
        auto box = node->GetBox(0);
        ValueType type(GetVariantType(attribute.ValueType));
        if (node->UsePerParticleDataResolve())
        {
            Value value;
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                value = GetValue(box, 2).Cast(type);
                Platform::MemoryCopy(dataPtr, &value.AsPointer, dataSize);
                dataPtr += stride;
            }
        }
        else
        {
            const Value value = GetValue(box, 2).Cast(type);
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                Platform::MemoryCopy(dataPtr, &value.AsPointer, dataSize);
                dataPtr += stride;
            }
        }
        break;
    }
    // Position (sphere surface)
    case 202:
    {
        PARTICLE_EMITTER_MODULE("Position");
        auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto arcBox = node->GetBox(2);

#define INPUTS_FETCH() \
	const Float3 center = (Float3)GetValue(centerBox, 2); \
	const float radius = (float)GetValue(radiusBox, 3); \
	const float arc = (float)GetValue(arcBox, 4) * DegreesToRadians
#define LOGIC() \
	float cosPhi = 2.0f * RAND - 1.0f; \
	float theta = arc * RAND; \
	Float2 sincosTheta; \
	Math::SinCos(theta, sincosTheta.X, sincosTheta.Y); \
	sincosTheta *= Math::Sqrt(1.0f - cosPhi * cosPhi); \
	*(Float3*)positionPtr = Float3(sincosTheta, cosPhi) * radius + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Position (plane)
    case 203:
    {
        PARTICLE_EMITTER_MODULE("Position");
        auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto sizeBox = node->GetBox(1);

#define INPUTS_FETCH() \
	const Float3 center = (Float3)GetValue(centerBox, 2); \
	const Float2 size = (Float2)GetValue(sizeBox, 3);
#define LOGIC() \
	*(Float3*)positionPtr = Float3((RAND - 0.5f) * size.X, 0.0f, (RAND - 0.5f) * size.Y) + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Position (circle)
    case 204:
    {
        PARTICLE_EMITTER_MODULE("Position");
        auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto arcBox = node->GetBox(2);

#define INPUTS_FETCH() \
	const Float3 center = (Float3)GetValue(centerBox, 2); \
	const float radius = (float)GetValue(radiusBox, 3); \
	const float arc = (float)GetValue(arcBox, 4) * DegreesToRadians
#define LOGIC() \
	float theta = arc * RAND; \
	Float2 sincosTheta; \
	Math::SinCos(theta, sincosTheta.X, sincosTheta.Y); \
	*(Float3*)positionPtr = Float3(sincosTheta, 0.0f) * radius + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Position (disc)
    case 205:
    {
        PARTICLE_EMITTER_MODULE("Position");
        auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto arcBox = node->GetBox(2);

#define INPUTS_FETCH() \
	const Float3 center = (Float3)GetValue(centerBox, 2); \
	const float radius = (float)GetValue(radiusBox, 3); \
	const float arc = (float)GetValue(arcBox, 4) * DegreesToRadians
#define LOGIC() \
	float theta = arc * RAND; \
	Float2 sincosTheta; \
	Math::SinCos(theta, sincosTheta.X, sincosTheta.Y); \
	*(Float3*)positionPtr = Float3(sincosTheta, 0.0f) * (radius * RAND) + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Position (box surface)
    case 206:
    {
        PARTICLE_EMITTER_MODULE("Position");
        auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto sizeBox = node->GetBox(1);

#define INPUTS_FETCH() \
	const Float3 center = (Float3)GetValue(centerBox, 2); \
	const Float3 size = (Float3)GetValue(sizeBox, 3);
#define LOGIC() \
	float areaXY = Math::Max(size.X * size.Y, ZeroTolerance); \
	float areaXZ = Math::Max(size.X * size.Z, ZeroTolerance); \
	float areaYZ = Math::Max(size.Y * size.Z, ZeroTolerance); \
	float face = RAND * (areaXY + areaXZ + areaYZ); \
	float flip = (RAND >= 0.5f) ? 0.5f : -0.5f; \
	Float3 cube(RAND2 - 0.5f, flip); \
	if (face < areaXY) \
		cube = Float3(cube.X, cube.Y, cube.Z); \
	else if(face < areaXY + areaXZ) \
		cube = Float3(cube.X, cube.Z, cube.Y); \
	else \
		cube = Float3(cube.Z, cube.X, cube.Y); \
	*(Float3*)positionPtr = cube * size + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Position (box volume)
    case 207:
    {
        PARTICLE_EMITTER_MODULE("Position");
        auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto sizeBox = node->GetBox(1);

#define INPUTS_FETCH() \
	const Float3 center = (Float3)GetValue(centerBox, 2); \
	const Float3 size = (Float3)GetValue(sizeBox, 3);
#define LOGIC() \
	*(Float3*)positionPtr = size * (RAND3 - 0.5f) + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Position (cylinder)
    case 208:
    {
        PARTICLE_EMITTER_MODULE("Position");
        auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto heightBox = node->GetBox(2);
        auto arcBox = node->GetBox(3);

#define INPUTS_FETCH() \
	const Float3 center = (Float3)GetValue(centerBox, 2); \
	const float radius = (float)GetValue(radiusBox, 3); \
	const float height = (float)GetValue(heightBox, 4); \
	const float arc = (float)GetValue(arcBox, 5) * DegreesToRadians
#define LOGIC() \
	float theta = arc * RAND; \
	Float2 sincosTheta; \
	Math::SinCos(theta, sincosTheta.X, sincosTheta.Y); \
	*(Float3*)positionPtr = Float3(sincosTheta * radius, height * RAND) + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Position (line)
    case 209:
    {
        PARTICLE_EMITTER_MODULE("Position");
        auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto startBox = node->GetBox(0);
        auto endBox = node->GetBox(1);

#define INPUTS_FETCH() \
	const Float3 start = (Float3)GetValue(startBox, 2); \
	const Float3 end = (Float3)GetValue(endBox, 3);
#define LOGIC() \
	*(Float3*)positionPtr = Math::Lerp(start, end, RAND); \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Position (torus)
    case 210:
    {
        PARTICLE_EMITTER_MODULE("Position");
        auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto thicknessBox = node->GetBox(2);
        auto arcBox = node->GetBox(3);

#define INPUTS_FETCH() \
	const Float3 center = (Float3)GetValue(centerBox, 2); \
	const float radius = Math::Max((float)GetValue(radiusBox, 3), ZeroTolerance); \
	const float thickness = (float)GetValue(thicknessBox, 4); \
	const float arc = (float)GetValue(arcBox, 5) * DegreesToRadians
#define LOGIC() \
	Float3 u = RAND3; \
	float sinTheta, cosTheta; \
	Math::SinCos(u.X * TWO_PI, sinTheta, cosTheta); \
	float r = Math::Saturate(thickness / radius); \
	Float2 s11 = r * Float2(cosTheta, sinTheta) + Float2(1, 0); \
	Float2 s12 = r * Float2(-cosTheta, sinTheta) + Float2(1, 0); \
	float w = s11.X / (s11.X + s12.X); \
	Float3 t; \
	float phi; \
	if (u.Y < w) \
	{ \
		phi = arc * u.Y / w; \
		t = Float3(s11.X, 0, s11.Y); \
	} \
	else \
	{ \
		phi = arc * (u.Y - w) / (1.0f - w); \
		t = Float3(s12.X, 0, s12.Y); \
	} \
	float s, c; \
	Math::SinCos(phi, c, s); \
	Float3 t2 = Float3(c * t.X - s * t.Y, c * t.Y + s * t.X, t.Z); \
	*(Float3*)positionPtr = center + radius * t2; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Position (sphere volume)
    case 211:
    {
        PARTICLE_EMITTER_MODULE("Position");
        auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto arcBox = node->GetBox(2);

#define INPUTS_FETCH() \
	const Float3 center = (Float3)GetValue(centerBox, 2); \
	const float radius = (float)GetValue(radiusBox, 3); \
	const float arc = (float)GetValue(arcBox, 4) * DegreesToRadians
#define LOGIC() \
	float cosPhi = 2.0f * RAND - 1.0f; \
	float theta = arc * RAND; \
	Float2 sincosTheta; \
	Math::SinCos(theta, sincosTheta.X, sincosTheta.Y); \
	sincosTheta *= Math::Sqrt(1.0f - cosPhi * cosPhi); \
	*(Float3*)positionPtr = Float3(sincosTheta, cosPhi) * (radius * RAND) + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Position (depth)
    case 212:
    {
        // Not supported
        break;
    }
    // Position (spiral)
    case 214:
    {
        PARTICLE_EMITTER_MODULE("Position");
        auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];
        auto& velocityAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[1]];

        byte* positionPtr = start + positionAttr.Offset;
        byte* velocityPtr = start + velocityAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto rotationSpeedBox = node->GetBox(1);
        auto velocityScaleBox = node->GetBox(2);

        auto& arc = *(float*)&context.Data->CustomData[node->CustomDataOffset];

#define INPUTS_FETCH() \
	const Float3 center = (Float3)GetValue(centerBox, 2); \
	const float rotationSpeed = (float)GetValue(rotationSpeedBox, 3); \
	const float velocityScale = (float)GetValue(velocityScaleBox, 4); \
	const float arcStep = rotationSpeed / (360.0f * DegreesToRadians)
#define LOGIC() \
	Float2 sincosTheta; \
	Math::SinCos(arc, sincosTheta.X, sincosTheta.Y); \
	arc += arcStep; \
	*(Float3*)velocityPtr = Float3(sincosTheta * velocityScale, 0.0f); \
	*(Float3*)positionPtr = center; \
	velocityPtr += stride; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Position (Global SDF)
    case 215:
    {
        // Not supported
        break;
    }
    // Rotate Position Shape
    case 216:
    {
        PARTICLE_EMITTER_MODULE("Rotate Position Shape");
        auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto quatBox = node->GetBox(0);

#define INPUTS_FETCH() \
    const Quaternion quat = (Quaternion)GetValue(quatBox, 2);
#define LOGIC() \
    Quaternion nq = Quaternion::Invert(quat); \
    Float3 v3 = *((Float3*)positionPtr); \
    Quaternion q = Quaternion(v3.X, v3.Y, v3.Z, 0.0f); \
    Quaternion rq = quat * (q * nq); \
    *(Float3*)positionPtr = Float3(rq.X, rq.Y, rq.Z); \
    positionPtr += stride;

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }

    // Helper macros for collision modules to share the code
#define COLLISION_BEGIN() \
	PARTICLE_EMITTER_MODULE("Collision"); \
	auto& positionAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[0]]; \
	auto& velocityAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[1]]; \
	auto& ageAttr = context.Data->Buffer->Layout->Attributes[node->Attributes[2]]; \
	byte* positionPtr = start + positionAttr.Offset; \
	byte* velocityPtr = start + velocityAttr.Offset; \
	byte* agePtr = start + ageAttr.Offset; \
	auto invert = (bool)node->Values[2]; \
	auto sign = invert ? -1.0f : 1.0f; \
	auto radiusBox = node->GetBox(0); \
	auto roughnessBox = node->GetBox(1); \
	auto elasticityBox = node->GetBox(2); \
	auto frictionBox = node->GetBox(3); \
	auto lifetimeLossBox = node->GetBox(4)
#define COLLISION_INPUTS_FETCH() \
	const float radius = (float)GetValue(radiusBox, 3); \
	const float roughness = (float)GetValue(roughnessBox, 4); \
	const float elasticity = (float)GetValue(elasticityBox, 5); \
	const float friction = (float)GetValue(frictionBox, 6); \
	const float lifetimeLoss = (float)GetValue(lifetimeLossBox, 7)
#define COLLISION_LOGIC() \
		Float3 randomNormal = Float3::Normalize(RAND3 * 2.0f - 1.0f); \
		randomNormal = (Float3::Dot(randomNormal, n) < 0.0f) ? -randomNormal : randomNormal; \
		n = Float3::Normalize(Float3::Lerp(n, randomNormal, roughness)); \
		\
		float projVelocity = Float3::Dot(n, velocity); \
		Float3 normalVelocity = projVelocity * n; \
		Float3 tangentVelocity = velocity - normalVelocity; \
		if (projVelocity < 0) \
			velocity -= ((1 + elasticity) * projVelocity) * n; \
		velocity -= friction * tangentVelocity; \
		*(Float3*)velocityPtr = velocity; \
		*(float*)agePtr += lifetimeLoss; \
	} \
	positionPtr += stride; \
	velocityPtr += stride; \
	agePtr += stride

    // Collision (plane)
    case 330:
    {
        COLLISION_BEGIN();
        auto planePositionBox = node->GetBox(5);
        auto planeNormalBox = node->GetBox(6);
#define INPUTS_FETCH() \
	COLLISION_INPUTS_FETCH(); \
	const Float3 planePosition = (Float3)GetValue(planePositionBox, 8); \
	const Float3 planeNormal = (Float3)GetValue(planeNormalBox, 9) * sign
#define LOGIC() \
	Float3 position = *(Float3*)positionPtr; \
	Float3 velocity = *(Float3*)velocityPtr; \
	Float3 nextPos = position + velocity * context.DeltaTime; \
	Float3 n = planeNormal; \
	float distToPlane = Float3::Dot(nextPos, n) - Float3::Dot(planePosition, n) - radius; \
	if (distToPlane < 0.0f) \
	{ \
		*(Float3*)positionPtr = position - n * distToPlane; \
	COLLISION_LOGIC()

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Collision (sphere)
    case 331:
    {
        COLLISION_BEGIN();
        auto spherePositionBox = node->GetBox(5);
        auto sphereRadiusBox = node->GetBox(6);
#define INPUTS_FETCH() \
	COLLISION_INPUTS_FETCH(); \
	const Float3 spherePosition = (Float3)GetValue(spherePositionBox, 8); \
	const float sphereRadius = (float)GetValue(sphereRadiusBox, 9)
#define LOGIC() \
	Float3 position = *(Float3*)positionPtr; \
	Float3 velocity = *(Float3*)velocityPtr; \
	Float3 nextPos = position + velocity * context.DeltaTime; \
	Float3 dir = nextPos - spherePosition; \
	float sqrLength = Float3::Dot(dir, dir); \
	float totalRadius = sphereRadius + sign * radius; \
	if (sign * sqrLength <= sign * totalRadius * totalRadius) \
	{ \
		float dist = Math::Sqrt(sqrLength); \
		Float3 n = sign * dir / Math::Max(dist, ZeroTolerance); \
		*(Float3*)positionPtr = position - n * (dist - totalRadius) * sign; \
	COLLISION_LOGIC()

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Collision (box)
    case 332:
    {
        COLLISION_BEGIN();
        auto boxPositionBox = node->GetBox(5);
        auto boxSizeBox = node->GetBox(6);
#define INPUTS_FETCH() \
	COLLISION_INPUTS_FETCH(); \
	const Float3 boxPosition = (Float3)GetValue(boxPositionBox, 8); \
	const Float3 boxSize = (Float3)GetValue(boxSizeBox, 9)
#define LOGIC() \
	Float3 position = *(Float3*)positionPtr; \
	Float3 velocity = *(Float3*)velocityPtr; \
	Float3 nextPos = position + velocity * context.DeltaTime; \
	Float3 dir = nextPos - boxPosition; \
	Float3 absDir = Float3::Abs(dir); \
	Float3 halfBoxSize = boxSize * 0.5f + radius * sign; \
	bool collision; \
	if (invert) \
		collision = absDir.X > halfBoxSize.X || absDir.Y > halfBoxSize.Y || absDir.Z > halfBoxSize.Z; \
	else \
		collision = absDir.X < halfBoxSize.X && absDir.Y < halfBoxSize.Y && absDir.Z < halfBoxSize.Z; \
	if (collision) \
	{ \
		Float3 distanceToEdge = (absDir - halfBoxSize); \
		Float3 absDistanceToEdge = Float3::Abs(distanceToEdge); \
		Float3 n; \
		if (absDistanceToEdge.X < absDistanceToEdge.Y && absDistanceToEdge.X < absDistanceToEdge.Z) \
			n = Float3(sign * Math::Sign(dir.X), 0.0f, 0.0f); \
		else if (absDistanceToEdge.Y < absDistanceToEdge.Z) \
			n = Float3(0.0f, sign * Math::Sign(dir.Y), 0.0f); \
		else \
			n = Float3(0.0f, 0.0f, sign * Math::Sign(dir.Z)); \
		if (invert) \
			*(Float3*)positionPtr = position - Float3::Max(distanceToEdge, Float3::Zero) * Float3(Math::Sign(dir.X), Math::Sign(dir.Y), Math::Sign(dir.Z)); \
		else \
			*(Float3*)positionPtr = position - n * distanceToEdge; \
	COLLISION_LOGIC()

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Collision (cylinder)
    case 333:
    {
        COLLISION_BEGIN();
        auto cylinderPositionBox = node->GetBox(5);
        auto cylinderHeightBox = node->GetBox(6);
        auto cylinderRadiusBox = node->GetBox(7);
#define INPUTS_FETCH() \
	COLLISION_INPUTS_FETCH(); \
	const Float3 cylinderPosition = (Float3)GetValue(cylinderPositionBox, 8); \
	const float cylinderHeight = (float)GetValue(cylinderHeightBox, 9); \
	const float cylinderRadius = (float)GetValue(cylinderRadiusBox, 10)
#define LOGIC() \
	Float3 position = *(Float3*)positionPtr; \
	Float3 velocity = *(Float3*)velocityPtr; \
	Float3 nextPos = position + velocity * context.DeltaTime; \
	Float3 dir = nextPos - cylinderPosition; \
	float halfHeight = cylinderHeight * 0.5f + radius * sign; \
	float cylinderRadiusT = cylinderRadius + radius * sign; \
	float sqrLength = Float2::Dot(Float2(dir.X, dir.Z), Float2(dir.X, dir.Z)); \
	bool collision; \
	if (invert) \
		collision = Math::Abs(dir.Y) < halfHeight && sqrLength < cylinderRadiusT * cylinderRadiusT; \
	else \
		collision = Math::Abs(dir.Y) > halfHeight || sqrLength > cylinderRadiusT * cylinderRadiusT; \
	if (collision) \
	{ \
		float dist = Math::Max(Math::Sqrt(sqrLength), ZeroTolerance); \
		float distToCap = sign * (halfHeight - Math::Abs(dir.Y)); \
		float distToSide = sign * (cylinderRadiusT - dist); \
		Float3 n = Float3(dir.X / dist, Math::Sign(dir.Y), dir.Z / dist) * sign; \
		if (invert) \
		{ \
			float distToSideClamped = Math::Max(0.0f, distToSide); \
			*(Float3*)positionPtr = position + n * Float3(distToSideClamped, Math::Max(0.0f, distToCap), distToSideClamped); \
			n *= distToSide > distToCap ? Float3(1, 0, 1) : Float3(0, 1, 0); \
		} \
		else \
		{ \
			n *= distToSide < distToCap ? Float3(1, 0, 1) : Float3(0, 1, 0); \
			*(Float3*)positionPtr = position + n * Math::Min(distToSide, distToCap); \
		} \
	COLLISION_LOGIC()

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                context.ParticleIndex = particleIndex;
                INPUTS_FETCH();
                LOGIC();
            }
        }
        else
        {
            INPUTS_FETCH();
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                LOGIC();
            }
        }
#undef INPUTS_FETCH
#undef LOGIC
        break;
    }
    // Collision (depth)
    case 334:
    {
        // Not supported
        break;
    }
    // Conform to Global SDF
    case 335:
    {
        // Not supported
        break;
    }
    // Collision (Global SDF)
    case 336:
    {
        // Not supported
        break;
    }

#undef COLLISION_BEGIN
#undef COLLISION_INPUTS_FETCH
#undef COLLISION_LOGIC
    }
}
