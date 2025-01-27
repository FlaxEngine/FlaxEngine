// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ParticleEmitterGraph.GPU.h"
#include "Engine/Graphics/Materials/MaterialInfo.h"

#define SET_ATTRIBUTE(attribute, value) _writer.Write(TEXT("\t{0} = {1};\n"), attribute.Value, value)

void ParticleEmitterGPUGenerator::ProcessModule(Node* node)
{
    auto nodeGpu = (ParticleEmitterGraphGPUNode*)node;
    switch (node->TypeID)
    {
    // Orient Sprite
    case 201:
    case 303:
    {
        auto spriteFacingMode = node->Values[2].AsInt;
        {
            auto attribute = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);
            SET_ATTRIBUTE(attribute, spriteFacingMode);
        }
        if ((ParticleSpriteFacingMode)spriteFacingMode == ParticleSpriteFacingMode::CustomFacingVector ||
            (ParticleSpriteFacingMode)spriteFacingMode == ParticleSpriteFacingMode::FixedAxis)
        {
            auto attribute = AccessParticleAttribute(node, nodeGpu->Attributes[1], AccessMode::Write);
            auto vector = GetValue(node->GetBox(0), 3).AsFloat3();
            SET_ATTRIBUTE(attribute, vector.Value);
        }
        break;
    }
    // Orient Model
    case 213:
    case 309:
    {
        auto modelFacingMode = node->Values[2].AsInt;
        {
            auto attribute = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);
            SET_ATTRIBUTE(attribute, modelFacingMode);
        }
        break;
    }
    // Update Age
    case 300:
    {
        auto attribute = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::ReadWrite);
        _writer.Write(TEXT("\t{0} += DeltaTime;\n"), attribute.Value);
        break;
    }
    // Gravity/Force
    case 301:
    case 304:
    {
        auto attribute = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::ReadWrite);
        auto force = GetValue(node->GetBox(0), 2).AsFloat3();
        _writer.Write(TEXT("\t{0} += {1} * DeltaTime;\n"), attribute.Value, force.Value);
        break;
    }
    // Conform to Sphere
    case 305:
    {
        auto position = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Read);
        auto velocity = AccessParticleAttribute(node, nodeGpu->Attributes[1], AccessMode::ReadWrite);
        auto mass = AccessParticleAttribute(node, nodeGpu->Attributes[2], AccessMode::Read);

        auto sphereCenterBox = node->GetBox(0);
        auto sphereRadiusBox = node->GetBox(1);
        auto attractionSpeedBox = node->GetBox(2);
        auto attractionForceBox = node->GetBox(3);
        auto stickDistanceBox = node->GetBox(4);
        auto stickForceBox = node->GetBox(5);

        const Value sphereCenter = GetValue(sphereCenterBox, 2).AsFloat3();
        const Value sphereRadius = GetValue(sphereRadiusBox, 3).AsFloat();
        const Value attractionSpeed = GetValue(attractionSpeedBox, 4).AsFloat();
        const Value attractionForce = GetValue(attractionForceBox, 5).AsFloat();
        const Value stickDistance = GetValue(stickDistanceBox, 6).AsFloat();
        const Value stickForce = GetValue(stickForceBox, 7).AsFloat();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Conform to Sphere\n"
                "		float3 dir = {3} - {0};\n"
                "		float distToCenter = length(dir);\n"
                "		float distToSurface = distToCenter - {4};\n"
                "		dir /= max(0.0001f, distToCenter);\n"
                "		float spdNormal = dot(dir, {1});\n"
                "		float ratio = smoothstep(0.0f, {7} * 2.0f, abs(distToSurface));\n"
                "		float tgtSpeed = sign(distToSurface) * {5} * ratio;\n"
                "		float deltaSpeed = tgtSpeed - spdNormal;\n"
                "		float3 deltaVelocity = dir * (sign(deltaSpeed) * min(abs(deltaSpeed), DeltaTime * lerp({8}, {6}, ratio)) / max({2}, PARTICLE_THRESHOLD));\n"
                "		{1} += deltaVelocity;\n"
                "	}}\n"
            ), position.Value, velocity.Value, mass.Value, sphereCenter.Value, sphereRadius.Value, attractionSpeed.Value, attractionForce.Value, stickDistance.Value, stickForce.Value);
        break;
    }
    // Kill (sphere)
    case 306:
    {
        UseKill();
        auto position = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Read);

        auto sphereCenterBox = node->GetBox(0);
        auto sphereRadiusBox = node->GetBox(1);
        auto sign = (bool)node->Values[4] ? -1.0f : 1.0f;

        const Value sphereCenter = GetValue(sphereCenterBox, 2).AsFloat3();
        const Value sphereRadius = GetValue(sphereRadiusBox, 3).AsFloat();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Kill (sphere)\n"
                "		float sphereRadiusSqr = {2} * {2};\n"
                "		float3 dir = {0} - {1};\n"
                "		float lengthSqr = dot(dir, dir);\n"
                "		kill = kill || ({3} * lengthSqr <= {3} * sphereRadiusSqr);\n"
                "	}}\n"
            ), position.Value, sphereCenter.Value, sphereRadius.Value, sign);
        break;
    }
    // Kill (box)
    case 307:
    {
        UseKill();
        auto position = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Read);

        auto boxCenterBox = node->GetBox(0);
        auto boxSizeBox = node->GetBox(1);
        auto invert = (bool)node->Values[4];

        const Value boxCenter = GetValue(boxCenterBox, 2).AsFloat3();
        const Value boxSize = GetValue(boxSizeBox, 3).AsFloat3();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Kill (box)\n"
                "		float3 dir = {0} - {1};\n"
                "		float3 absDir = abs(dir);\n"
                "		float3 size = {2} * 0.5f;\n"
                "		bool collision;\n"
                "		if ({3})\n"
                "			collision = any(absDir >= size);\n"
                "		else\n"
                "			collision = all(absDir <= size);\n"
                "		kill = kill || collision\n"
                "	}}\n"
            ), position.Value, boxCenter.Value, boxSize.Value, invert);
        break;
    }
    // Kill (custom)
    case 308:
    {
        UseKill();
        auto killBox = node->GetBox(0);

        const Value kill = tryGetValue(killBox, Value::False).AsBool();
        _writer.Write(
            TEXT(
                "	kill = kill || ({0});\n"
            ), kill.Value);
        break;
    }
    // Linear Drag
    case 310:
    {
        const bool useSpriteSize = node->Values[3].AsBool;
        auto velocity = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::ReadWrite);
        auto mass = AccessParticleAttribute(node, nodeGpu->Attributes[1], AccessMode::Read);
        auto drag = tryGetValue(node->GetBox(0), 2, Value::Zero).AsFloat();

        if (useSpriteSize)
        {
            auto spriteSize = AccessParticleAttribute(node, nodeGpu->Attributes[2], AccessMode::Read);
            _writer.Write(
                TEXT(
                    "	{{\n"
                    "		// Linear Drag\n"
                    "		float drag = {2} * {3}.x * {3}.y;\n"
                    "		{0} *= max(0.0f, 1.0f - (drag * DeltaTime) / max({1}, PARTICLE_THRESHOLD));\n"
                    "	}}\n"
                ), velocity.Value, mass.Value, drag.Value, spriteSize.Value);
        }
        else
        {
            _writer.Write(
                TEXT(
                    "	{{\n"
                    "		// Linear Drag\n"
                    "		float drag = {2};\n"
                    "		{0} *= max(0.0f, 1.0f - (drag * DeltaTime) / max({1}, PARTICLE_THRESHOLD));\n"
                    "	}}\n"
                ), velocity.Value, mass.Value, drag.Value);
        }
        break;
    }
    // Turbulence
    case 311:
    {
        auto position = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Read);
        auto velocity = AccessParticleAttribute(node, nodeGpu->Attributes[1], AccessMode::ReadWrite);
        auto mass = AccessParticleAttribute(node, nodeGpu->Attributes[2], AccessMode::Read);
        auto fieldPosition = tryGetValue(node->GetBox(0), 2, Value::Zero).AsFloat3();
        auto fieldRotation = tryGetValue(node->GetBox(1), 3, Value::Zero).AsFloat3();
        auto fieldScale = tryGetValue(node->GetBox(2), 4, Value::One).AsFloat3();
        auto roughness = tryGetValue(node->GetBox(3), 5, Value::Zero).AsFloat();
        auto intensity = tryGetValue(node->GetBox(4), 6, Value::Zero).AsFloat();
        auto octavesCount = tryGetValue(node->GetBox(5), 7, Value::Zero).AsInt();
        _includes.Add(TEXT("./Flax/Noise.hlsl"));
        // TODO: build fieldTransformMatrix and invFieldTransformMatrix on CPU and pass in constant buffer - no need to support field transform per particle
        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Turbulence\n"
                "		float3x3 rotationMatrix = EulerMatrix(radians({4}));\n"
                "		float4x4 scaleMatrix = float4x4(float4({5}.x, 0.0f, 0.0f, 0.0f), float4(0.0f, {5}.y, 0.0f, 0.0f), float4(0.0f, 0.0f, {5}.z, 0.0f), float4(0.0f, 0.0f, 0.0f, 1.0f));\n"
                "		float4x4 fieldTransformMatrix = float4x4(float4(rotationMatrix[0], {3}.x), float4(rotationMatrix[1], {3}.y), float4(rotationMatrix[2], {3}.z), float4(0.0f, 0.0f, 0.0f, 1.0f));\n"
                "		fieldTransformMatrix = mul(fieldTransformMatrix, scaleMatrix);\n"
                "		float4x4 invFieldTransformMatrix = Inverse(fieldTransformMatrix);\n"
                "		float3 vectorFieldUVW = mul(invFieldTransformMatrix, float4({0}, 1.0f)).xyz;\n"
                "		float3 force = CustomNoise3D(vectorFieldUVW + 0.5f, {8}, {6});\n"
                "		force = mul(fieldTransformMatrix, float4(force, 0.0f)).xyz * {7};\n"
                "		{1} += force * (DeltaTime / max({2}, PARTICLE_THRESHOLD));\n"
                "	}}\n"
            ), position.Value, velocity.Value, mass.Value, fieldPosition.Value, fieldRotation.Value, fieldScale.Value, roughness.Value, intensity.Value, octavesCount.Value);
        break;
    }
    // Set Attribute
    case 200:
    case 302:
    {
        auto attribute = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);
        auto box = node->GetBox(0);
        Value value = GetValue(box, 4).Cast(attribute.Type);
        SET_ATTRIBUTE(attribute, value.Value);
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
        auto attribute = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);
        auto box = node->GetBox(0);
        Value value = GetValue(box, 2).Cast(attribute.Type);
        SET_ATTRIBUTE(attribute, value.Value);
        break;
    }
    // Position (sphere surface)
    case 202:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto arcBox = node->GetBox(2);

        const Value center = GetValue(centerBox, 2).AsFloat3();
        const Value radius = GetValue(radiusBox, 3).AsFloat();
        const Value arc = GetValue(arcBox, 4).AsFloat();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (sphere surface)\n"
                "		float cosPhi = 2.0f * RAND - 1.0f;\n"
                "		float theta = radians({4}) * RAND;\n"
                "		float2 sincosTheta;\n"
                "		sincos(theta, sincosTheta.x, sincosTheta.y);\n"
                "		sincosTheta *= sqrt(1.0f - cosPhi * cosPhi);\n"
                "		{0} = float3(sincosTheta, cosPhi) * {3} + {2};\n"
                "	}}\n"
            ), positionAttr.Value, 0, center.Value, radius.Value, arc.Value);
        break;
    }
    // Position (plane)
    case 203:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);

        auto centerBox = node->GetBox(0);
        auto sizeBox = node->GetBox(1);

        const Value center = GetValue(centerBox, 2).AsFloat3();
        const Value size = GetValue(sizeBox, 3).AsFloat2();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (plane)\n"
                "		{0} = float3((RAND - 0.5f) * {2}.x, 0.0f, (RAND - 0.5f) * {2}.y) + {1};\n"
                "	}}\n"
            ), positionAttr.Value, center.Value, size.Value);
        break;
    }
    // Position (circle)
    case 204:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto arcBox = node->GetBox(2);

        const Value center = GetValue(centerBox, 2).AsFloat3();
        const Value radius = GetValue(radiusBox, 3).AsFloat();
        const Value arc = GetValue(arcBox, 4).AsFloat();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (circle)\n"
                "		float theta = radians({4}) * RAND;\n"
                "		float2 sincosTheta;\n"
                "		sincos(theta, sincosTheta.x, sincosTheta.y);\n"
                "		{0} = float3(sincosTheta, 0.0f) * {3} + {2};\n"
                "	}}\n"
            ), positionAttr.Value, 0, center.Value, radius.Value, arc.Value);
        break;
    }
    // Position (disc)
    case 205:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto arcBox = node->GetBox(2);

        const Value center = GetValue(centerBox, 2).AsFloat3();
        const Value radius = GetValue(radiusBox, 3).AsFloat();
        const Value arc = GetValue(arcBox, 4).AsFloat();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (disc)\n"
                "		float theta = radians({4}) * RAND;\n"
                "		float2 sincosTheta;\n"
                "		sincos(theta, sincosTheta.x, sincosTheta.y);\n"
                "		{0} = float3(sincosTheta, 0.0f) * ({3} * RAND) + {2};\n"
                "	}}\n"
            ), positionAttr.Value, 0, center.Value, radius.Value, arc.Value);
        break;
    }
    // Position (box surface)
    case 206:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);

        auto centerBox = node->GetBox(0);
        auto sizeBox = node->GetBox(1);

        const Value center = GetValue(centerBox, 2).AsFloat3();
        const Value size = GetValue(sizeBox, 3).AsFloat3();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (box surface)\n"
                "		float areaXY = max({2}.x * {2}.y, PARTICLE_THRESHOLD);\n"
                "		float areaXZ = max({2}.x * {2}.z, PARTICLE_THRESHOLD);\n"
                "		float areaYZ = max({2}.y * {2}.z, PARTICLE_THRESHOLD);\n"
                "		float face = RAND * (areaXY + areaXZ + areaYZ);\n"
                "		float flip = (RAND >= 0.5f) ? 0.5f : -0.5f;\n"
                "		float3 cube = float3(RAND - 0.5f, RAND - 0.5f, flip);\n"
                "		if (face < areaXY)\n"
                "			cube = float3(cube.x, cube.y, cube.z);\n"
                "		else if (face < areaXY + areaXZ)\n"
                "			cube = float3(cube.x, cube.z, cube.y);\n"
                "		else\n"
                "			cube = float3(cube.z, cube.x, cube.y);\n"
                "		{0} = cube * {2} + {1};\n"
                "	}}\n"
            ), positionAttr.Value, center.Value, size.Value);
        break;
    }
    // Position (box volume)
    case 207:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);

        auto centerBox = node->GetBox(0);
        auto sizeBox = node->GetBox(1);

        const Value center = GetValue(centerBox, 2).AsFloat3();
        const Value size = GetValue(sizeBox, 3).AsFloat3();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (box volume)\n"
                "		{0} = {2} * (RAND3 - 0.5f) + {1};\n"
                "	}}\n"
            ), positionAttr.Value, center.Value, size.Value);
        break;
    }
    // Position (cylinder)
    case 208:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto heightBox = node->GetBox(2);
        auto arcBox = node->GetBox(3);

        const Value center = GetValue(centerBox, 2).AsFloat3();
        const Value radius = GetValue(radiusBox, 3).AsFloat();
        const Value height = GetValue(heightBox, 4).AsFloat();
        const Value arc = GetValue(arcBox, 5).AsFloat();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (cylinder)\n"
                "		float theta = radians({4}) * RAND;\n"
                "		float2 sincosTheta;\n"
                "		sincos(theta, sincosTheta.x, sincosTheta.y);\n"
                "		{0} = float3(sincosTheta * {2}, {3} * RAND) + {1};\n"
                "	}}\n"
            ), positionAttr.Value, center.Value, radius.Value, height.Value, arc.Value);
        break;
    }
    // Position (line)
    case 209:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);

        auto startBox = node->GetBox(0);
        auto endBox = node->GetBox(1);

        const Value start = GetValue(startBox, 2).AsFloat3();
        const Value end = GetValue(endBox, 3).AsFloat3();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (line)\n"
                "		{0} = lerp({1}, {2}, RAND);\n"
                "	}}\n"
            ), positionAttr.Value, start.Value, end.Value);
        break;
    }
    // Position (torus)
    case 210:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto thicknessBox = node->GetBox(2);
        auto arcBox = node->GetBox(3);

        const Value center = GetValue(centerBox, 2).AsFloat3();
        const Value radius = GetValue(radiusBox, 3).AsFloat();
        const Value thickness = GetValue(thicknessBox, 4).AsFloat();
        const Value arc = GetValue(arcBox, 5).AsFloat();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (torus)\n"
                "		float3 u = RAND3;\n"
                "		float sinTheta, cosTheta;\n"
                "		sincos(u.x * PI * 2.0f, sinTheta, cosTheta);\n"
                "		float r = saturate((float){4} / max({3}, PARTICLE_THRESHOLD));\n"
                "		float2 s11 = r * float2( cosTheta, sinTheta) + float2(1, 0);\n"
                "		float2 s12 = r * float2(-cosTheta, sinTheta) + float2(1, 0);\n"
                "		float w = s11.x / (s11.x + s12.x);\n"
                "		float3 t;\n"
                "		float phi;\n"
                "		if (u.y < w)\n"
                "		{{\n"
                "			phi = radians({5}) * u.y / w;\n"
                "			t = float3(s11.x, 0, s11.y);\n"
                "		}}\n"
                "		else\n"
                "		{{\n"
                "			phi = radians({5}) * (u.y - w) / (1.0f - w);\n"
                "			t = float3(s12.x, 0, s12.y);\n"
                "		}}\n"
                "		float s, c;\n"
                "		sincos(phi, c, s);\n"
                "		float3 t2 = float3(c * t.x - s * t.y, c * t.y + s * t.x, t.z);\n"
                "		{0} = {2} + {3} * t2;\n"
                "	}}\n"
            ), positionAttr.Value, 0, center.Value, radius.Value, thickness.Value, arc.Value);
        break;
    }
    // Position (sphere volume)
    case 211:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);

        auto centerBox = node->GetBox(0);
        auto radiusBox = node->GetBox(1);
        auto arcBox = node->GetBox(2);

        const Value center = GetValue(centerBox, 2).AsFloat3();
        const Value radius = GetValue(radiusBox, 3).AsFloat();
        const Value arc = GetValue(arcBox, 4).AsFloat();

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (sphere volume)\n"
                "		float cosPhi = 2.0f * RAND - 1.0f;\n"
                "		float theta = radians({4}) * RAND;\n"
                "		float2 sincosTheta;\n"
                "		sincos(theta, sincosTheta.x, sincosTheta.y);\n"
                "		sincosTheta *= sqrt(1.0f - cosPhi * cosPhi);\n"
                "		{0} = float3(sincosTheta, cosPhi) * ({3} * RAND) + {2};\n"
                "	}}\n"
            ), positionAttr.Value, 0, center.Value, radius.Value, arc.Value);
        break;
    }
    // Position (depth)
    case 212:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);
        auto lifetimeAttr = AccessParticleAttribute(node, nodeGpu->Attributes[1], AccessMode::Write);

        auto uvBox = node->GetBox(0);
        auto depthCullRangeBox = node->GetBox(1);
        auto depthOffsetBox = node->GetBox(2);

        const Value uv = GetValue(uvBox).AsFloat2();
        const Value depthCullRange = GetValue(depthCullRangeBox, 2).AsFloat2();
        const Value depthOffset = GetValue(depthOffsetBox, 3).AsFloat();

        const auto sceneDepthTexture = findOrAddSceneTexture(MaterialSceneTextures::SceneDepth);
        Value depth = writeLocal(VariantType::Float, String::Format(TEXT("{0}.Load(uint3({1} * ScreenSize.xy, 0)).r"), sceneDepthTexture.ShaderName, uv.Value), node);
        Value linearDepth;
        linearizeSceneDepth(node, depth, linearDepth);

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (depth)\n"
                "		float linearDepth = ({4} * ViewFar) - {3};\n"
                "		float2 uv = {1} * float2(2.0, -2.0) + float2(-1.0, 1.0);\n"
                "		float3 viewPos = float3(uv * ViewInfo.xy * linearDepth, linearDepth);\n"
                "		{0} = mul(float4(viewPos, 1), InvViewMatrix).xyz;\n"
                "		{0} = mul(float4({0}, 1), InvWorldMatrix).xyz;\n" // TODO: don't transform by InvWorldMatrix if particle system uses World Space simulation
                "		if ({4} < {2}.x || {4} > {2}.y)\n"
                "		{{ {5} = 0; {0} = 10000000; }}\n"
                "	}}\n"
            ), positionAttr.Value, uv.Value, depthCullRange.Value, depthOffset.Value, linearDepth.Value, lifetimeAttr.Value);
        break;
    }
    // Position (spiral)
    case 214:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);
        auto velocityAttr = AccessParticleAttribute(node, nodeGpu->Attributes[1], AccessMode::Write);

        auto centerBox = node->GetBox(0);
        auto rotationSpeedBox = node->GetBox(1);
        auto velocityScaleBox = node->GetBox(2);

        const Value center = GetValue(centerBox, 2).AsFloat3();
        const Value rotationSpeed = GetValue(rotationSpeedBox, 3).AsFloat();
        const Value velocityScale = GetValue(velocityScaleBox, 4).AsFloat();

        // Use state information inside the particle buffer
        const int32 size = ((ParticleEmitterGraphGPU*)_graphStack.Peek())->Capacity * ((ParticleEmitterGraphGPU*)_graphStack.Peek())->Layout.Size;
        uint32 customDataOffset = size + sizeof(uint32) + _customDataSize;
        _customDataSize += sizeof(float);

        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (spiral)\n"
                "		float arcDelta = (float)({3}) / (PI * 2.0f);\n"
                "		int arcDeltaAsInteger = (int)(arcDelta * 3600);\n"
                "		int arcAsInteger;\n"
                "		DstParticlesData.InterlockedAdd({5}, arcDeltaAsInteger, arcAsInteger);\n"
                "		float arc = (float)arcAsInteger / 3600.0f;\n"
                "		float2 sincosTheta;\n"
                "		sincos(arc, sincosTheta.x, sincosTheta.y);\n"
                "		{1} = float3(sincosTheta * {4}, 0.0f);\n"
                "		{0} = {2};\n"
                "	}}\n"
            ), positionAttr.Value, velocityAttr.Value, center.Value, rotationSpeed.Value, velocityScale.Value, customDataOffset);
        break;
    }
    // Position (Global SDF)
    case 215:
    {
        auto position = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::ReadWrite);
        auto param = findOrAddGlobalSDF();
        String wsPos = position.Value;
        if (IsLocalSimulationSpace())
            wsPos = String::Format(TEXT("mul(float4({0}, 1), WorldMatrix).xyz"), wsPos);
        _includes.Add(TEXT("./Flax/GlobalSignDistanceField.hlsl"));
        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Position (Global SDF)\n"
                "		float3 wsPos = {2};\n"
                "		float dist;\n"
                "		float3 dir = -normalize(SampleGlobalSDFGradient({1}, {1}_Tex, {1}_Mip, wsPos, dist));\n"
                "		{0} += dist < GLOBAL_SDF_WORLD_SIZE ? dir * dist : float3(0, 0, 0);\n"
                "	}}\n"
            ), position.Value, param.ShaderName, wsPos);
        break;
    }
    // Rotate position shape
    case 216:
    {
        auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Write);
        const Value quaternion = GetValue(node->GetBox(0), 2).Cast(VariantType::Quaternion);
        _writer.Write(
            TEXT(
                "   {{\n"
                "       // Rotate position shape\n"
                "       {0} = QuatRotateVector({1}, {0});\n"
                "   }}\n"
                ), positionAttr.Value, quaternion.Value);
        break;
    }

    // Helper macros for collision modules to share the code
#define COLLISION_BEGIN() \
		auto positionAttr = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::ReadWrite); \
		auto velocityAttr = AccessParticleAttribute(node, nodeGpu->Attributes[1], AccessMode::ReadWrite); \
		auto ageAttr = AccessParticleAttribute(node, nodeGpu->Attributes[2], AccessMode::ReadWrite); \
		auto invert = (bool)node->Values[2]; \
		auto sign = invert ? -1.0f : 1.0f; \
		auto radiusBox = node->GetBox(0); \
		auto roughnessBox = node->GetBox(1); \
		auto elasticityBox = node->GetBox(2); \
		auto frictionBox = node->GetBox(3); \
		auto lifetimeLossBox = node->GetBox(4); \
		const Value radius = GetValue(radiusBox, 3).AsFloat(); \
		const Value roughness = GetValue(roughnessBox, 4).AsFloat(); \
		const Value elasticity = GetValue(elasticityBox, 5).AsFloat(); \
		const Value friction = GetValue(frictionBox, 6).AsFloat(); \
		const Value lifetimeLoss = GetValue(lifetimeLossBox, 7).AsFloat()
#define COLLISION_LOGIC() \
			"			float3 randomNormal = normalize(RAND3 * 2.0f - 1.0f);\n" \
			"			randomNormal = (dot(randomNormal, n) < 0.0f) ? -randomNormal : randomNormal;\n" \
			"			n = normalize(lerp(n, randomNormal, {6}));\n" \
			"			float projVelocity = dot(n, {1});\n" \
			"			float3 normalVelocity = projVelocity * n;\n" \
			"			float3 tangentVelocity = {1} - normalVelocity;\n" \
			"			if (projVelocity < 0)\n" \
			"				{1} -= ((1 + {7}) * projVelocity) * n;\n" \
			"			{1} -= {8} * tangentVelocity;\n" \
			"			{2} += {9};\n" \
			"		}}\n"

    // Collision (plane)
    case 330:
    {
        COLLISION_BEGIN();
        const Value planePosition = GetValue(node->GetBox(5), 8).AsFloat3();
        const Value planeNormal = GetValue(node->GetBox(6), 9).AsFloat3();
        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Collision (plane)\n"
                "		float3 nextPos = {0} + {1} * DeltaTime;\n"
                "		float3 n = {11} * {4};\n"
                "		float distToPlane = dot(nextPos, n) - dot({10}, n) - {5};\n"
                "		if (distToPlane < 0.0f)\n"
                "		{{\n"
                "			{0} -= n * distToPlane;\n"
                COLLISION_LOGIC()
                "	}}\n"
            ),
            // 0-4
            positionAttr.Value, velocityAttr.Value, ageAttr.Value, invert, sign,
            // 5-9
            radius.Value, roughness.Value, elasticity.Value, friction.Value, lifetimeLoss.Value,
            // 10-11
            planePosition.Value, planeNormal.Value
        );
        break;
    }
    // Collision (sphere)
    case 331:
    {
        COLLISION_BEGIN();
        const Value spherePosition = GetValue(node->GetBox(5), 8).AsFloat3();
        const Value sphereRadius = GetValue(node->GetBox(6), 9).AsFloat();
        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Collision (sphere)\n"
                "		float3 nextPos = {0} + {1} * DeltaTime;\n"
                "		float3 dir = nextPos - {10};\n"
                "		float sqrLength = dot(dir, dir);\n"
                "		float totalRadius = {11} + {4} * {5};\n"
                "		if ({4} * sqrLength <= {4} * totalRadius * totalRadius)\n"
                "		{{\n"
                "			float dist = sqrt(sqrLength);\n"
                "			float3 n = {4} * dir / max(dist, PARTICLE_THRESHOLD);\n"
                "			{0} -= n * (dist - totalRadius) * {4};\n"
                COLLISION_LOGIC()
                "	}}\n"
            ),
            // 0-4
            positionAttr.Value, velocityAttr.Value, ageAttr.Value, invert, sign,
            // 5-9
            radius.Value, roughness.Value, elasticity.Value, friction.Value, lifetimeLoss.Value,
            // 10-11
            spherePosition.Value, sphereRadius.Value
        );
        break;
    }
    // Collision (box)
    case 332:
    {
        COLLISION_BEGIN();
        const Value boxPosition = GetValue(node->GetBox(5), 8).AsFloat3();
        const Value boxSize = GetValue(node->GetBox(6), 9).AsFloat3();
        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Collision (box)\n"
                "		float3 nextPos = {0} + {1} * DeltaTime;\n"
                "		float3 dir = nextPos - {10};\n"
                "		float3 absDir = abs(dir);\n"
                "		float3 halfBoxSize = {11} * 0.5f + {5} * {4};\n"
                "		bool collision;\n"
                "		if ({3})\n"
                "			collision = any(absDir > halfBoxSize);\n"
                "		else\n"
                "			collision = all(absDir < halfBoxSize);\n"
                "		if (collision)\n"
                "		{{\n"
                "			float3 distanceToEdge = (absDir - halfBoxSize);\n"
                "			float3 absDistanceToEdge = abs(distanceToEdge);\n"
                "			float3 n;\n"
                "			if (absDistanceToEdge.x < absDistanceToEdge.y && absDistanceToEdge.x < absDistanceToEdge.z)\n"
                "				n = float3({4} * sign(dir.x), 0.0f, 0.0f);\n"
                "			else if (absDistanceToEdge.y < absDistanceToEdge.z)\n"
                "				n = float3(0.0f, {4} * sign(dir.y), 0.0f);\n"
                "			else\n"
                "				n = float3(0.0f, 0.0f, {4} * sign(dir.z));\n"
                "			if ({3})\n"
                "				{0} -= max(distanceToEdge, 0.0f) * sign(dir);\n"
                "			else\n"
                "				{0} -= n * distanceToEdge;\n"
                COLLISION_LOGIC()
                "	}}\n"
            ),
            // 0-4
            positionAttr.Value, velocityAttr.Value, ageAttr.Value, invert, sign,
            // 5-9
            radius.Value, roughness.Value, elasticity.Value, friction.Value, lifetimeLoss.Value,
            // 10-11
            boxPosition.Value, boxSize.Value
        );
        break;
    }
    // Collision (cylinder)
    case 333:
    {
        COLLISION_BEGIN();
        const Value cylinderPosition = GetValue(node->GetBox(5), 8).AsFloat3();
        const Value cylinderHeight = GetValue(node->GetBox(6), 9).AsFloat();
        const Value cylinderRadius = GetValue(node->GetBox(7), 10).AsFloat();
        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Collision (cylinder)\n"
                "		float3 nextPos = {0} + {1} * DeltaTime;\n"
                "		float3 dir = nextPos - {10};\n"
                "		float halfHeight = {11} * 0.5f + {5} * {4};\n"
                "		float cylinderRadiusT = {12} + {5} * {4};\n"
                "		float sqrLength = dot(dir.xz, dir.xz);\n"
                "		bool collision;\n"
                "		if ({3})\n"
                "			collision = abs(dir.y) < halfHeight && sqrLength < cylinderRadiusT * cylinderRadiusT;\n"
                "		else\n"
                "			collision = abs(dir.y) > halfHeight || sqrLength > cylinderRadiusT * cylinderRadiusT;\n"
                "		if (collision)\n"
                "		{{\n"
                "			float dist = max(sqrt(sqrLength), PARTICLE_THRESHOLD);\n"
                "			float distToCap = {4} * (halfHeight - abs(dir.y));\n"
                "			float distToSide = {4} * (cylinderRadiusT - dist);\n"
                "			float3 n = float3(dir.x / dist, sign(dir.y), dir.z / dist) * {4};\n"
                "			if ({3})\n"
                "			{{\n"
                "				float distToSideClamped = max(0.0f, distToSide);\n"
                "				{0} += n * float3(distToSideClamped, max(0.0f, distToCap), distToSideClamped);\n"
                "				n *= distToSide > distToCap ? float3(1, 0, 1) : float3(0, 1, 0);\n"
                "			}}\n"
                "			else\n"
                "			{{\n"
                "				n *= distToSide < distToCap ? float3(1, 0, 1) : float3(0, 1, 0);\n"
                "				{0} += n * min(distToSide, distToCap);\n"
                "			}}\n"
                COLLISION_LOGIC()
                "	}}\n"
            ),
            // 0-4
            positionAttr.Value, velocityAttr.Value, ageAttr.Value, invert, sign,
            // 5-9
            radius.Value, roughness.Value, elasticity.Value, friction.Value, lifetimeLoss.Value,
            // 10-12
            cylinderPosition.Value, cylinderHeight.Value, cylinderRadius.Value
        );
        break;
    }
    // Collision (depth)
    case 334:
    {
        COLLISION_BEGIN();
        const Value surfaceThickness = GetValue(node->GetBox(5), 8).AsFloat();
        const auto sceneDepthTexture = findOrAddSceneTexture(MaterialSceneTextures::SceneDepth);
        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Collision (depth)\n"
                "		float3 nextPos = {0} + {1} * DeltaTime;\n"
                "		nextPos = mul(float4(nextPos, 1), WorldMatrix).xyz;\n" // TODO: don't transform by WorldMatrix if particle system uses World Space simulation

                "		float3 viewPos = mul(float4(nextPos, 1), ViewMatrix);\n"
                "		float4 projPos = mul(float4(nextPos, 1), ViewProjectionMatrix);\n"
                "		projPos.xyz /= projPos.w;\n"
                "		if (all(abs(projPos.xy) < 1.0f))\n"
                "		{{\n"
                "			float2 uv = projPos.xy * float2(0.5f, -0.5f) + 0.5f;\n"
                "			uint2 pixel = uv * ScreenSize.xy;\n"

                "			float depth = {11}.Load(uint3(pixel, 0)).r;\n"
                "			float linearDepth = ViewInfo.w / (depth - ViewInfo.z) * ViewFar;\n"

                "			if (viewPos.z > linearDepth - {5} && viewPos.z < linearDepth + {5} + {10})\n"
                "			{{\n"

                "				float depth10 = {11}.Load(uint3(pixel + uint2(1, 0), 0)).r;\n"
                "				float depth01 = {11}.Load(uint3(pixel + uint2(0, 1), 0)).r;\n"

                "				float3 p = ReprojectPosition(uv, depth);\n"
                "				float3 p10 = ReprojectPosition(uv + float2(1, 0) * ScreenSize.zw, depth10);\n"
                "				float3 p01 = ReprojectPosition(uv + float2(0, 1) * ScreenSize.zw, depth01);\n"

                "				float3 n = normalize(cross(p10 - p, p01 - p));\n"

                "				viewPos.z = linearDepth;\n"
                "				\n"

                "				{0} = mul(float4(viewPos, 1), InvViewMatrix).xyz;\n" // TODO: don't transform by WorldMatrix if particle system uses World Space simulation
                "				{0} = mul(float4({0}, 1), InvWorldMatrix).xyz;\n" // TODO: don't transform by WorldMatrix if particle system uses World Space simulation
                COLLISION_LOGIC()

                "		}}\n"
                "	}}\n"
            ),
            // 0-4
            positionAttr.Value, velocityAttr.Value, ageAttr.Value, invert, sign,
            // 5-9
            radius.Value, roughness.Value, elasticity.Value, friction.Value, lifetimeLoss.Value,
            // 10-11
            surfaceThickness.Value, sceneDepthTexture.ShaderName
        );
        break;
    }
    // Conform to Global SDF
    case 335:
    {
        auto position = AccessParticleAttribute(node, nodeGpu->Attributes[0], AccessMode::Read);
        auto velocity = AccessParticleAttribute(node, nodeGpu->Attributes[1], AccessMode::ReadWrite);
        auto mass = AccessParticleAttribute(node, nodeGpu->Attributes[2], AccessMode::Read);

        const Value attractionSpeed = GetValue(node->GetBox(0), 2).AsFloat();
        const Value attractionForce = GetValue(node->GetBox(1), 3).AsFloat();
        const Value stickDistance = GetValue(node->GetBox(2), 4).AsFloat();
        const Value stickForce = GetValue(node->GetBox(3), 5).AsFloat();

        auto param = findOrAddGlobalSDF();
        _includes.Add(TEXT("./Flax/GlobalSignDistanceField.hlsl"));
        _writer.Write(
            TEXT(
                "	{{\n"
                "		// Conform to Global SDF\n"
                "		float dist;\n"
                "		float3 dir = normalize(SampleGlobalSDFGradient({3}, {3}_Tex, {3}_Mip, {0}, dist));\n"
                "		if (dist > 0) dir *= -1;\n"
                "		float distToSurface = abs(dist);\n"
                "		float spdNormal = dot(dir, {1});\n"
                "		float ratio = smoothstep(0.0f, {6} * 2.0f, distToSurface);\n"
                "		float tgtSpeed = {4} * ratio;\n"
                "		float deltaSpeed = tgtSpeed - spdNormal;\n"
                "		float3 deltaVelocity = dir * (sign(deltaSpeed) * min(abs(deltaSpeed), DeltaTime * lerp({7}, {5}, ratio)) / max({2}, PARTICLE_THRESHOLD));\n"
                "		{1} += dist < GLOBAL_SDF_WORLD_SIZE ? deltaVelocity : 0.0f;\n"
                "	}}\n"
            ), position.Value, velocity.Value, mass.Value, param.ShaderName, attractionSpeed.Value, attractionForce.Value, stickDistance.Value, stickForce.Value);
        break;
    }
    // Collision (Global SDF)
    case 336:
    {
        COLLISION_BEGIN();
        auto param = findOrAddGlobalSDF();
        _includes.Add(TEXT("./Flax/GlobalSignDistanceField.hlsl"));
        const Char* format = IsLocalSimulationSpace()
                                 ? TEXT(
                                     "	{{\n"
                                     "		// Collision (Global SDF)\n"
                                     "		float3 nextPos = {0} + {1} * DeltaTime;\n"
                                     "		nextPos = mul(float4(nextPos, 1), WorldMatrix).xyz;\n"
                                     "		float dist = SampleGlobalSDF({10}, {10}_Tex, {10}_Mip, nextPos);\n"
                                     "		if (dist < {5})\n"
                                     "		{{\n"
                                     "			{0} = mul(float4({0}, 1), WorldMatrix).xyz;\n"
                                     "			float3 n = normalize(SampleGlobalSDFGradient({10}, {10}_Tex, {10}_Mip, {0}, dist));\n"
                                     "			{0} += n * -dist;\n"
                                     "			{0} = mul(float4({0}, 1), InvWorldMatrix).xyz;\n"
                                     COLLISION_LOGIC()
                                     "	}}\n"
                                 )
                                 : TEXT(
                                     "	{{\n"
                                     "		// Collision (Global SDF)\n"
                                     "		float3 nextPos = {0} + {1} * DeltaTime;\n"
                                     "		float dist = SampleGlobalSDF({10}, {10}_Tex, {10}_Mip, nextPos);\n"
                                     "		if (dist < {5})\n"
                                     "		{{\n"
                                     "			float3 n = normalize(SampleGlobalSDFGradient({10}, {10}_Tex, {10}_Mip, {0}, dist));\n"
                                     "			{0} += n * -dist;\n"
                                     COLLISION_LOGIC()
                                     "	}}\n"
                                 );
        _writer.Write(format,
                      // 0-4
                      positionAttr.Value, velocityAttr.Value, ageAttr.Value, invert, sign,
                      // 5-9
                      radius.Value, roughness.Value, elasticity.Value, friction.Value, lifetimeLoss.Value,
                      // 10
                      param.ShaderName
        );
        break;
    }

#undef COLLISION_BEGIN
#undef COLLISION_LOGIC
    }
}
