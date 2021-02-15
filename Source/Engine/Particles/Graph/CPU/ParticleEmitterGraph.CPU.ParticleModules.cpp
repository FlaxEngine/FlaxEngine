// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ParticleEmitterGraph.CPU.h"
#include "Engine/Core/Random.h"

#define RAND Random::Rand()
#define RAND2 Vector2(RAND, RAND)
#define RAND3 Vector3(RAND, RAND, RAND)
#define RAND4 Vector4(RAND, RAND, RAND, RAND)

namespace
{
    FORCE_INLINE Vector4 Mod289(Vector4 x)
    {
        return x - Vector4::Floor(x * (1.0f / 289.0f)) * 289.0f;
    }

    FORCE_INLINE Vector4 Perm(Vector4 x)
    {
        return Mod289((x * 34.0f + 1.0f) * x);
    }

    float Noise(Vector3 p)
    {
        Vector3 a = Vector3::Floor(p);
        Vector3 d = p - a;
        d = d * d * (3.0f - 2.0f * d);

        Vector4 b(a.X, a.X + 1.0f, a.Y, a.Y + 1.0f);
        Vector4 k1 = Perm(Vector4(b.X, b.Y, b.X, b.Y));
        Vector4 k2 = Perm(Vector4(k1.X + b.Z, k1.Y + b.Z, k1.X + b.W, k1.Y + b.W));

        Vector4 c = k2 + Vector4(a.Z);
        Vector4 k3 = Perm(c);
        Vector4 k4 = Perm(c + 1.0f);

        Vector4 o1 = Vector4::Frac(k3 * (1.0f / 41.0f));
        Vector4 o2 = Vector4::Frac(k4 * (1.0f / 41.0f));

        Vector4 o3 = o2 * d.Z + o1 * (1.0f - d.Z);
        Vector2 o4 = Vector2(o3.Y, o3.W) * d.X + Vector2(o3.X, o3.Z) * (1.0f - d.X);

        return o4.Y * d.Y + o4.X * (1.0f - d.Y);
    }

    Vector3 Noise3D(Vector3 p)
    {
        const float o = Noise(p);
        const float a = Noise(p + Vector3(0.0001f, 0.0f, 0.0f));
        const float b = Noise(p + Vector3(0.0f, 0.0001f, 0.0f));
        const float c = Noise(p + Vector3(0.0f, 0.0f, 0.0001f));

        const Vector3 grad(o - a, o - b, o - c);
        const Vector3 other = Vector3::Abs(Vector3(grad.Z, grad.X, grad.Y));
        return Vector3::Normalize(Vector3::Cross(grad, other));
    }

    Vector3 Noise3D(Vector3 position, int32 octaves, float roughness)
    {
        float weight = 0.0f;
        Vector3 noise = Vector3::Zero;
        float scale = 1.0f;
        for (int32 i = 0; i < octaves; i++)
        {
            const float curWeight = Math::Pow(1.0f - ((float)i / octaves), Math::Lerp(2.0f, 0.2f, roughness));

            noise += Noise3D(position * scale) * curWeight;
            weight += curWeight;

            scale *= 1.72531f;
        }
        return noise / Math::Max(weight, ZeroTolerance);
    }

    VariantType::Types GetVariantType(ParticleAttribute::ValueTypes type)
    {
        switch (type)
        {
        case ParticleAttribute::ValueTypes::Vector2:
            return VariantType::Vector2;
        case ParticleAttribute::ValueTypes::Vector3:
            return VariantType::Vector3;
        case ParticleAttribute::ValueTypes::Vector4:
            return VariantType::Vector4;
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
    auto& data = _data->SpawnModulesData[index];

    // Accumulate the previous frame fraction
    float spawnCount = data.SpawnCounter;

    // Calculate particles to spawn during this frame
    switch (node->TypeID)
    {
        // Constant Spawn Rate
    case 100:
    {
        const float rate = Math::Max((float)TryGetValue(node->GetBox(0), node->Values[2]), 0.0f);
        spawnCount += rate * _deltaTime;
        break;
    }
        // Single Burst
    case 101:
    {
        const bool isFirstUpdate = (_data->Time - _deltaTime) <= 0.0f;
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
        if (nextSpawnTime - _data->Time <= 0.0f)
        {
            const float count = Math::Max((float)TryGetValue(node->GetBox(0), node->Values[2]), 0.0f);
            const float delay = Math::Max((float)TryGetValue(node->GetBox(1), node->Values[3]), 0.0f);
            nextSpawnTime = _data->Time + delay;
            spawnCount += count;
        }
        break;
    }
        // Periodic Burst (range)
    case 103:
    {
        float& nextSpawnTime = data.NextSpawnTime;
        if (nextSpawnTime - _data->Time <= 0.0f)
        {
            const Vector2 countMinMax = (Vector2)TryGetValue(node->GetBox(0), node->Values[2]);
            const Vector2 delayMinMax = (Vector2)TryGetValue(node->GetBox(1), node->Values[3]);
            const float count = Math::Max(countMinMax.X + RAND * (countMinMax.Y - countMinMax.X), 0.0f);
            const float delay = Math::Max(delayMinMax.X + RAND * (delayMinMax.Y - delayMinMax.X), 0.0f);
            nextSpawnTime = _data->Time + delay;
            spawnCount += count;
        }
        break;
    }
    }

    // Calculate actual spawn amount
    spawnCount = Math::Max(spawnCount, 0.0f);
    const int32 result = Math::FloorToInt(spawnCount);
    spawnCount -= result;
    data.SpawnCounter = spawnCount;

    return result;
}

void ParticleEmitterGraphCPUExecutor::ProcessModule(ParticleEmitterGraphCPUNode* node, int32 particlesStart, int32 particlesEnd)
{
    auto stride = _data->Buffer->Stride;
    auto start = _data->Buffer->GetParticleCPU(particlesStart);

    switch (node->TypeID)
    {
        // Orient Sprite
    case 201:
    case 303:
    {
        auto spriteFacingMode = node->Values[2].AsInt;
        {
            auto& attribute = _data->Buffer->Layout->Attributes[node->Attributes[0]];
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
            auto& attribute = _data->Buffer->Layout->Attributes[node->Attributes[1]];
            byte* customFacingVectorPtr = start + attribute.Offset;
            auto box = node->GetBox(0);
            if (node->UsePerParticleDataResolve())
            {
                for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
                {
                    _particleIndex = particleIndex;
                    const Vector3 vector = (Vector3)GetValue(box, 3);
                    *((Vector3*)customFacingVectorPtr) = vector;
                    customFacingVectorPtr += stride;
                }
            }
            else
            {
                const Vector3 vector = (Vector3)GetValue(box, 3);
                for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
                {
                    *((Vector3*)customFacingVectorPtr) = vector;
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
        auto modelFacingMode = node->Values[2].AsInt;
        {
            auto& attribute = _data->Buffer->Layout->Attributes[node->Attributes[0]];
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
        auto& attribute = _data->Buffer->Layout->Attributes[node->Attributes[0]];
        byte* agePtr = start + attribute.Offset;
        for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
        {
            *((float*)agePtr) += _deltaTime;
            agePtr += stride;
        }
        break;
    }
        // Gravity/Force
    case 301:
    case 304:
    {
        auto& attribute = _data->Buffer->Layout->Attributes[node->Attributes[0]];
        byte* velocityPtr = start + attribute.Offset;
        auto box = node->GetBox(0);
        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
                const Vector3 force = (Vector3)GetValue(box, 2);
                *((Vector3*)velocityPtr) += force * _deltaTime;
                velocityPtr += stride;
            }
        }
        else
        {
            const Vector3 force = (Vector3)GetValue(box, 2);
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                *((Vector3*)velocityPtr) += force * _deltaTime;
                velocityPtr += stride;
            }
        }
        break;
    }
        // Conform to Sphere
    case 305:
    {
        auto& position = _data->Buffer->Layout->Attributes[node->Attributes[0]];
        auto& velocity = _data->Buffer->Layout->Attributes[node->Attributes[1]];
        auto& mass = _data->Buffer->Layout->Attributes[node->Attributes[2]];

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
	const Vector3 sphereCenter = (Vector3)GetValue(sphereCenterBox, 2); \
	const float sphereRadius = (float)GetValue(sphereRadiusBox, 3); \
	const float attractionSpeed = (float)GetValue(attractionSpeedBox, 4); \
	const float attractionForce = (float)GetValue(attractionForceBox, 5); \
	const float stickDistance = (float)GetValue(stickDistanceBox, 6); \
	const float stickForce = (float)GetValue(stickForceBox, 7)
#define LOGIC() \
	Vector3 dir = sphereCenter - *(Vector3*)positionPtr; \
	float distToCenter = dir.Length(); \
	float distToSurface = distToCenter - sphereRadius; \
	dir /= Math::Max(0.0001f, distToCenter); \
	Vector3 velocity = *(Vector3*)velocityPtr; \
	float spdNormal = Vector3::Dot(dir, velocity); \
	float ratio = Math::SmoothStep(0.0f, stickDistance * 2.0f, Math::Abs(distToSurface)); \
	float tgtSpeed = Math::Sign(distToSurface) * attractionSpeed * ratio; \
	float deltaSpeed = tgtSpeed - spdNormal; \
	Vector3 deltaVelocity = dir * (Math::Sign(deltaSpeed) * Math::Min(Math::Abs(deltaSpeed), _deltaTime * Math::Lerp(stickForce, attractionForce, ratio)) / Math::Max(*(float*)massPtr, ZeroTolerance)); \
	*(Vector3*)velocityPtr = velocity + deltaVelocity; \
	positionPtr += stride; \
	velocityPtr += stride; \
	massPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& position = _data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + position.Offset;

        auto sphereCenterBox = node->GetBox(0);
        auto sphereRadiusBox = node->GetBox(1);
        auto sign = (bool)node->Values[4] ? -1.0f : 1.0f;

#define INPUTS_FETCH() \
	const Vector3 sphereCenter = (Vector3)GetValue(sphereCenterBox, 2); \
	const float sphereRadius = (float)GetValue(sphereRadiusBox, 3); \
	const float sphereRadiusSqr = sphereRadius * sphereRadius
#define LOGIC() \
	Vector3 dir = *(Vector3*)positionPtr - sphereCenter; \
	float lengthSqr = Vector3::Dot(dir, dir); \
	if (sign * lengthSqr <= sign * sphereRadiusSqr) \
	{ \
		particlesEnd--; \
		_data->Buffer->CPU.Count--; \
		Platform::MemoryCopy(_data->Buffer->GetParticleCPU(particleIndex), _data->Buffer->GetParticleCPU(_data->Buffer->CPU.Count), _data->Buffer->Stride); \
		particleIndex--; \
	} \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& position = _data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + position.Offset;

        auto boxCenterBox = node->GetBox(0);
        auto boxSizeBox = node->GetBox(1);
        auto invert = (bool)node->Values[4];

#define INPUTS_FETCH() \
	const Vector3 boxCenter = (Vector3)GetValue(boxCenterBox, 2); \
	const Vector3 boxSize = (Vector3)GetValue(boxSizeBox, 3)
#define LOGIC() \
	Vector3 dir = *(Vector3*)positionPtr - boxCenter; \
	Vector3 absDir = Vector3::Abs(dir); \
	Vector3 size = boxSize * 0.5f; \
	bool collision; \
	if (invert) \
		collision = absDir.X >= size.X || absDir.Y >= size.Y || absDir.Z >= size.Z; \
	else \
		collision = absDir.X <= size.X && absDir.Y <= size.Y && absDir.Z <= size.Z; \
	if (collision) \
	{ \
		particlesEnd--; \
		_data->Buffer->CPU.Count--; \
		Platform::MemoryCopy(_data->Buffer->GetParticleCPU(particleIndex), _data->Buffer->GetParticleCPU(_data->Buffer->CPU.Count), _data->Buffer->Stride); \
		particleIndex--; \
	} \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto killBox = node->GetBox(0);

#define INPUTS_FETCH() \
	const bool kill = (bool)TryGetValue(killBox, Value::False)
#define LOGIC() \
	if (kill) \
	{ \
		particlesEnd--; \
		_data->Buffer->CPU.Count--; \
		Platform::MemoryCopy(_data->Buffer->GetParticleCPU(particleIndex), _data->Buffer->GetParticleCPU(_data->Buffer->CPU.Count), _data->Buffer->Stride); \
		particleIndex--; \
	}

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto box = node->GetBox(0);
        const bool useSpriteSize = node->Values[3].AsBool;

        auto& velocity = _data->Buffer->Layout->Attributes[node->Attributes[0]];
        auto& mass = _data->Buffer->Layout->Attributes[node->Attributes[1]];
        byte* spriteSizePtr = useSpriteSize ? start + _data->Buffer->Layout->Attributes[node->Attributes[2]].Offset : 0;

        byte* velocityPtr = start + velocity.Offset;
        byte* massPtr = start + mass.Offset;

#define INPUTS_FETCH() \
	const float drag = (float)GetValue(box, 2)
#define LOGIC() \
	float particleDrag = drag; \
    if (useSpriteSize) \
        particleDrag *= ((Vector2*)spriteSizePtr)->MulValues(); \
    *((Vector3*)velocityPtr) *= Math::Max(0.0f, 1.0f - (particleDrag * _deltaTime) / Math::Max(*(float*)massPtr, ZeroTolerance)); \
    velocityPtr += stride; \
    massPtr += stride; \
    spriteSizePtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& position = _data->Buffer->Layout->Attributes[node->Attributes[0]];
        auto& velocity = _data->Buffer->Layout->Attributes[node->Attributes[1]];
        auto& mass = _data->Buffer->Layout->Attributes[node->Attributes[2]];

        byte* positionPtr = start + position.Offset;
        byte* velocityPtr = start + velocity.Offset;
        byte* massPtr = start + mass.Offset;

        auto roughnessBox = node->GetBox(3);
        auto intensityBox = node->GetBox(4);
        auto octavesCountBox = node->GetBox(5);

        const Vector3 fieldPosition = (Vector3)GetValue(node->GetBox(0), 2);
        const Vector3 fieldRotation = (Vector3)GetValue(node->GetBox(1), 3);
        const Vector3 fieldScale = (Vector3)GetValue(node->GetBox(2), 4);

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
	Vector3 vectorFieldUVW = Vector3::Transform(*((Vector3*)positionPtr), invFieldTransformMatrix); \
	Vector3 force = Noise3D(vectorFieldUVW + 0.5f, octavesCount, roughness); \
    force = Vector3::Transform(force, fieldTransformMatrix) * intensity; \
    *((Vector3*)velocityPtr) += force * (_deltaTime / Math::Max(*(float*)massPtr, ZeroTolerance)); \
    positionPtr += stride; \
    velocityPtr += stride; \
    massPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& attribute = _data->Buffer->Layout->Attributes[node->Attributes[0]];
        byte* dataPtr = start + attribute.Offset;
        int32 dataSize = attribute.GetSize();
        auto box = node->GetBox(0);
        ValueType type(GetVariantType(attribute.ValueType));
        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
                const Value value = GetValue(box, 4).Cast(type);
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
    {
        auto& attribute = _data->Buffer->Layout->Attributes[node->Attributes[0]];
        byte* dataPtr = start + attribute.Offset;
        int32 dataSize = attribute.GetSize();
        auto box = node->GetBox(0);
        ValueType type(GetVariantType(attribute.ValueType));
        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
                const Value value = GetValue(box, 2).Cast(type);
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
        auto& positionAttr = _data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto arcBox = node->GetBox(2);

#define INPUTS_FETCH() \
	const Vector3 center = (Vector3)GetValue(centerBox, 2); \
	const float radius = (float)GetValue(radiusBox, 3); \
	const float arc = (float)GetValue(arcBox, 4) * DegreesToRadians
#define LOGIC() \
	float cosPhi = 2.0f * RAND - 1.0f; \
	float theta = arc * RAND; \
	Vector2 sincosTheta; \
	Math::SinCos(theta, sincosTheta.X, sincosTheta.Y); \
	sincosTheta *= Math::Sqrt(1.0f - cosPhi * cosPhi); \
	*(Vector3*)positionPtr = Vector3(sincosTheta, cosPhi) * radius + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& positionAttr = _data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto sizeBox = node->GetBox(1);

#define INPUTS_FETCH() \
	const Vector3 center = (Vector3)GetValue(centerBox, 2); \
	const Vector2 size = (Vector2)GetValue(sizeBox, 3);
#define LOGIC() \
	*(Vector3*)positionPtr = Vector3((RAND - 0.5f) * size.X, 0.0f, (RAND - 0.5f) * size.Y) + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& positionAttr = _data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto arcBox = node->GetBox(2);

#define INPUTS_FETCH() \
	const Vector3 center = (Vector3)GetValue(centerBox, 2); \
	const float radius = (float)GetValue(radiusBox, 3); \
	const float arc = (float)GetValue(arcBox, 4) * DegreesToRadians
#define LOGIC() \
	float theta = arc * RAND; \
	Vector2 sincosTheta; \
	Math::SinCos(theta, sincosTheta.X, sincosTheta.Y); \
	*(Vector3*)positionPtr = Vector3(sincosTheta, 0.0f) * radius + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& positionAttr = _data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto arcBox = node->GetBox(2);

#define INPUTS_FETCH() \
	const Vector3 center = (Vector3)GetValue(centerBox, 2); \
	const float radius = (float)GetValue(radiusBox, 3); \
	const float arc = (float)GetValue(arcBox, 4) * DegreesToRadians
#define LOGIC() \
	float theta = arc * RAND; \
	Vector2 sincosTheta; \
	Math::SinCos(theta, sincosTheta.X, sincosTheta.Y); \
	*(Vector3*)positionPtr = Vector3(sincosTheta, 0.0f) * (radius * RAND) + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& positionAttr = _data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto sizeBox = node->GetBox(1);

#define INPUTS_FETCH() \
	const Vector3 center = (Vector3)GetValue(centerBox, 2); \
	const Vector3 size = (Vector3)GetValue(sizeBox, 3);
#define LOGIC() \
	float areaXY = Math::Max(size.X * size.Y, ZeroTolerance); \
	float areaXZ = Math::Max(size.X * size.Z, ZeroTolerance); \
	float areaYZ = Math::Max(size.Y * size.Z, ZeroTolerance); \
	float face = RAND * (areaXY + areaXZ + areaYZ); \
	float flip = (RAND >= 0.5f) ? 0.5f : -0.5f; \
	Vector3 cube(RAND2 - 0.5f, flip); \
	if (face < areaXY) \
		cube = Vector3(cube.X, cube.Y, cube.Z); \
	else if(face < areaXY + areaXZ) \
		cube = Vector3(cube.X, cube.Z, cube.Y); \
	else \
		cube = Vector3(cube.Z, cube.X, cube.Y); \
	*(Vector3*)positionPtr = cube * size + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& positionAttr = _data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto sizeBox = node->GetBox(1);

#define INPUTS_FETCH() \
	const Vector3 center = (Vector3)GetValue(centerBox, 2); \
	const Vector3 size = (Vector3)GetValue(sizeBox, 3);
#define LOGIC() \
	*(Vector3*)positionPtr = size * (RAND3 - 0.5f) + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& positionAttr = _data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto heightBox = node->GetBox(2);
        auto arcBox = node->GetBox(3);

#define INPUTS_FETCH() \
	const Vector3 center = (Vector3)GetValue(centerBox, 2); \
	const float radius = (float)GetValue(radiusBox, 3); \
	const float height = (float)GetValue(heightBox, 4); \
	const float arc = (float)GetValue(arcBox, 5) * DegreesToRadians
#define LOGIC() \
	float theta = arc * RAND; \
	Vector2 sincosTheta; \
	Math::SinCos(theta, sincosTheta.X, sincosTheta.Y); \
	*(Vector3*)positionPtr = Vector3(sincosTheta * radius, height * RAND) + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& positionAttr = _data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto startBox = node->GetBox(0);
        auto endBox = node->GetBox(1);

#define INPUTS_FETCH() \
	const Vector3 start = (Vector3)GetValue(startBox, 2); \
	const Vector3 end = (Vector3)GetValue(endBox, 3);
#define LOGIC() \
	*(Vector3*)positionPtr = Math::Lerp(start, end, RAND); \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& positionAttr = _data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto thicknessBox = node->GetBox(2);
        auto arcBox = node->GetBox(3);

#define INPUTS_FETCH() \
	const Vector3 center = (Vector3)GetValue(centerBox, 2); \
	const float radius = Math::Max((float)GetValue(radiusBox, 3), ZeroTolerance); \
	const float thickness = (float)GetValue(thicknessBox, 4); \
	const float arc = (float)GetValue(arcBox, 5) * DegreesToRadians
#define LOGIC() \
	Vector3 u = RAND3; \
	float sinTheta, cosTheta; \
	Math::SinCos(u.X * TWO_PI, sinTheta, cosTheta); \
	float r = Math::Saturate(thickness / radius); \
	Vector2 s11 = r * Vector2(cosTheta, sinTheta) + Vector2(1, 0); \
	Vector2 s12 = r * Vector2(-cosTheta, sinTheta) + Vector2(1, 0); \
	float w = s11.X / (s11.X + s12.X); \
	Vector3 t; \
	float phi; \
	if (u.Y < w) \
	{ \
		phi = arc * u.Y / w; \
		t = Vector3(s11.X, 0, s11.Y); \
	} \
	else \
	{ \
		phi = arc * (u.Y - w) / (1.0f - w); \
		t = Vector3(s12.X, 0, s12.Y); \
	} \
	float s, c; \
	Math::SinCos(phi, c, s); \
	Vector3 t2 = Vector3(c * t.X - s * t.Y, c * t.Y + s * t.X, t.Z); \
	*(Vector3*)positionPtr = center + radius * t2; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& positionAttr = _data->Buffer->Layout->Attributes[node->Attributes[0]];

        byte* positionPtr = start + positionAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto arcBox = node->GetBox(2);

#define INPUTS_FETCH() \
	const Vector3 center = (Vector3)GetValue(centerBox, 2); \
	const float radius = (float)GetValue(radiusBox, 3); \
	const float arc = (float)GetValue(arcBox, 4) * DegreesToRadians
#define LOGIC() \
	float cosPhi = 2.0f * RAND - 1.0f; \
	float theta = arc * RAND; \
	Vector2 sincosTheta; \
	Math::SinCos(theta, sincosTheta.X, sincosTheta.Y); \
	sincosTheta *= Math::Sqrt(1.0f - cosPhi * cosPhi); \
	*(Vector3*)positionPtr = Vector3(sincosTheta, cosPhi) * (radius * RAND) + center; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
        auto& positionAttr = _data->Buffer->Layout->Attributes[node->Attributes[0]];
        auto& velocityAttr = _data->Buffer->Layout->Attributes[node->Attributes[1]];

        byte* positionPtr = start + positionAttr.Offset;
        byte* velocityPtr = start + velocityAttr.Offset;

        auto centerBox = node->GetBox(0);
        auto rotationSpeedBox = node->GetBox(1);
        auto velocityScaleBox = node->GetBox(2);

        auto& arc = node->SpiralModuleProgress;

#define INPUTS_FETCH() \
	const Vector3 center = (Vector3)GetValue(centerBox, 2); \
	const float rotationSpeed = (float)GetValue(rotationSpeedBox, 3); \
	const float velocityScale = (float)GetValue(velocityScaleBox, 4); \
	const float arcStep = rotationSpeed / (360.0f * DegreesToRadians)
#define LOGIC() \
	Vector2 sincosTheta; \
	Math::SinCos(arc, sincosTheta.X, sincosTheta.Y); \
	arc += arcStep; \
	*(Vector3*)velocityPtr = Vector3(sincosTheta * velocityScale, 0.0f); \
	*(Vector3*)positionPtr = center; \
	velocityPtr += stride; \
	positionPtr += stride

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
	auto& positionAttr = _data->Buffer->Layout->Attributes[node->Attributes[0]]; \
	auto& velocityAttr = _data->Buffer->Layout->Attributes[node->Attributes[1]]; \
	auto& ageAttr = _data->Buffer->Layout->Attributes[node->Attributes[2]]; \
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
		Vector3 randomNormal = Vector3::Normalize(RAND3 * 2.0f - 1.0f); \
		randomNormal = (Vector3::Dot(randomNormal, n) < 0.0f) ? -randomNormal : randomNormal; \
		n = Vector3::Normalize(Vector3::Lerp(n, randomNormal, roughness)); \
		\
		float projVelocity = Vector3::Dot(n, velocity); \
		Vector3 normalVelocity = projVelocity * n; \
		Vector3 tangentVelocity = velocity - normalVelocity; \
		if (projVelocity < 0) \
			velocity -= ((1 + elasticity) * projVelocity) * n; \
		velocity -= friction * tangentVelocity; \
		*(Vector3*)velocityPtr = velocity; \
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
	const Vector3 planePosition = (Vector3)GetValue(planePositionBox, 8); \
	const Vector3 planeNormal = (Vector3)GetValue(planeNormalBox, 9) * sign
#define LOGIC() \
	Vector3 position = *(Vector3*)positionPtr; \
	Vector3 velocity = *(Vector3*)velocityPtr; \
	Vector3 nextPos = position + velocity * _deltaTime; \
	Vector3 n = planeNormal; \
	float distToPlane = Vector3::Dot(nextPos, n) - Vector3::Dot(planePosition, n) - radius; \
	if (distToPlane < 0.0f) \
	{ \
		*(Vector3*)positionPtr = position - n * distToPlane; \
	COLLISION_LOGIC()

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
	const Vector3 spherePosition = (Vector3)GetValue(spherePositionBox, 8); \
	const float sphereRadius = (float)GetValue(sphereRadiusBox, 9)
#define LOGIC() \
	Vector3 position = *(Vector3*)positionPtr; \
	Vector3 velocity = *(Vector3*)velocityPtr; \
	Vector3 nextPos = position + velocity * _deltaTime; \
	Vector3 dir = nextPos - spherePosition; \
	float sqrLength = Vector3::Dot(dir, dir); \
	float totalRadius = sphereRadius + sign * radius; \
	if (sign * sqrLength <= sign * totalRadius * totalRadius) \
	{ \
		float dist = Math::Sqrt(sqrLength); \
		Vector3 n = sign * dir / Math::Max(dist, ZeroTolerance); \
		*(Vector3*)positionPtr = position - n * (dist - totalRadius) * sign; \
	COLLISION_LOGIC()

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
	const Vector3 boxPosition = (Vector3)GetValue(boxPositionBox, 8); \
	const Vector3 boxSize = (Vector3)GetValue(boxSizeBox, 9)
#define LOGIC() \
	Vector3 position = *(Vector3*)positionPtr; \
	Vector3 velocity = *(Vector3*)velocityPtr; \
	Vector3 nextPos = position + velocity * _deltaTime; \
	Vector3 dir = nextPos - boxPosition; \
	Vector3 absDir = Vector3::Abs(dir); \
	Vector3 halfBoxSize = boxSize * 0.5f + radius * sign; \
	bool collision; \
	if (invert) \
		collision = absDir.X > halfBoxSize.X || absDir.Y > halfBoxSize.Y || absDir.Z > halfBoxSize.Z; \
	else \
		collision = absDir.X < halfBoxSize.X && absDir.Y < halfBoxSize.Y && absDir.Z < halfBoxSize.Z; \
	if (collision) \
	{ \
		Vector3 distanceToEdge = (absDir - halfBoxSize); \
		Vector3 absDistanceToEdge = Vector3::Abs(distanceToEdge); \
		Vector3 n; \
		if (absDistanceToEdge.X < absDistanceToEdge.Y && absDistanceToEdge.X < absDistanceToEdge.Z) \
			n = Vector3(sign * Math::Sign(dir.X), 0.0f, 0.0f); \
		else if (absDistanceToEdge.Y < absDistanceToEdge.Z) \
			n = Vector3(0.0f, sign * Math::Sign(dir.Y), 0.0f); \
		else \
			n = Vector3(0.0f, 0.0f, sign * Math::Sign(dir.Z)); \
		if (invert) \
			*(Vector3*)positionPtr = position - Vector3::Max(distanceToEdge, Vector3::Zero) * Vector3(Math::Sign(dir.X), Math::Sign(dir.Y), Math::Sign(dir.Z)); \
		else \
			*(Vector3*)positionPtr = position - n * distanceToEdge; \
	COLLISION_LOGIC()

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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
	const Vector3 cylinderPosition = (Vector3)GetValue(cylinderPositionBox, 8); \
	const float cylinderHeight = (float)GetValue(cylinderHeightBox, 9); \
	const float cylinderRadius = (float)GetValue(cylinderRadiusBox, 10)
#define LOGIC() \
	Vector3 position = *(Vector3*)positionPtr; \
	Vector3 velocity = *(Vector3*)velocityPtr; \
	Vector3 nextPos = position + velocity * _deltaTime; \
	Vector3 dir = nextPos - cylinderPosition; \
	float halfHeight = cylinderHeight * 0.5f + radius * sign; \
	float cylinderRadiusT = cylinderRadius + radius * sign; \
	float sqrLength = Vector2::Dot(Vector2(dir.X, dir.Z), Vector2(dir.X, dir.Z)); \
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
		Vector3 n = Vector3(dir.X / dist, Math::Sign(dir.Y), dir.Z / dist) * sign; \
		if (invert) \
		{ \
			float distToSideClamped = Math::Max(0.0f, distToSide); \
			*(Vector3*)positionPtr = position + n * Vector3(distToSideClamped, Math::Max(0.0f, distToCap), distToSideClamped); \
			n *= distToSide > distToCap ? Vector3(1, 0, 1) : Vector3(0, 1, 0); \
		} \
		else \
		{ \
			n *= distToSide < distToCap ? Vector3(1, 0, 1) : Vector3(0, 1, 0); \
			*(Vector3*)positionPtr = position + n * Math::Min(distToSide, distToCap); \
		} \
	COLLISION_LOGIC()

        if (node->UsePerParticleDataResolve())
        {
            for (int32 particleIndex = particlesStart; particleIndex < particlesEnd; particleIndex++)
            {
                _particleIndex = particleIndex;
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

#undef COLLISION_BEGIN
#undef COLLISION_INPUTS_FETCH
#undef COLLISION_LOGIC
    }
}
