// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SplineModel.h"
#include "Spline.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Core/Math/Matrix3x4.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Profiler/ProfilerCPU.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif

#define SPLINE_RESOLUTION 32.0f

SplineModel::SplineModel(const SpawnParams& params)
    : ModelInstanceActor(params)
{
    Model.Changed.Bind<SplineModel, &SplineModel::OnModelChanged>(this);
    Model.Loaded.Bind<SplineModel, &SplineModel::OnModelLoaded>(this);
}

SplineModel::~SplineModel()
{
    SAFE_DELETE_GPU_RESOURCE(_deformationBuffer);
    if (_deformationBufferData)
        Allocator::Free(_deformationBufferData);
}

Transform SplineModel::GetPreTransform() const
{
    return _preTransform;
}

void SplineModel::SetPreTransform(const Transform& value)
{
    if (_preTransform == value)
        return;
    _preTransform = value;
    OnSplineUpdated();
}

float SplineModel::GetQuality() const
{
    return _quality;
}

void SplineModel::SetQuality(float value)
{
    value = Math::Clamp(value, 0.0f, 100.0f);
    if (Math::NearEqual(value, _quality))
        return;
    _quality = value;
    OnSplineUpdated();
}

float SplineModel::GetBoundsScale() const
{
    return _boundsScale;
}

void SplineModel::SetBoundsScale(float value)
{
    if (Math::NearEqual(_boundsScale, value))
        return;
    _boundsScale = value;
    OnSplineUpdated();
}

int32 SplineModel::GetLODBias() const
{
    return static_cast<int32>(_lodBias);
}

void SplineModel::SetLODBias(int32 value)
{
    _lodBias = static_cast<char>(Math::Clamp(value, -100, 100));
}

int32 SplineModel::GetForcedLOD() const
{
    return static_cast<int32>(_forcedLod);
}

void SplineModel::SetForcedLOD(int32 value)
{
    _forcedLod = static_cast<char>(Math::Clamp(value, -1, 100));
}

void SplineModel::OnModelChanged()
{
    Entries.Release();

    if (Model && !Model->IsLoaded())
    {
        OnSplineUpdated();
    }
}

void SplineModel::OnModelLoaded()
{
    Entries.SetupIfInvalid(Model);

    OnSplineUpdated();
}

void SplineModel::OnSplineUpdated()
{
    // Skip updates when actor is disabled or something is missing
    if (!_spline || !Model || !Model->IsLoaded() || !IsActiveInHierarchy() || _spline->GetSplinePointsCount() < 2)
    {
        _box = BoundingBox(_transform.Translation, _transform.Translation);
        BoundingSphere::FromBox(_box, _sphere);
        return;
    }
    PROFILE_CPU();

    // Setup model instances over the spline segments
    const auto& keyframes = _spline->Curve.GetKeyframes();
    const int32 segments = keyframes.Count() - 1;
    const int32 chunksPerSegment = Math::Clamp(Math::CeilToInt(SPLINE_RESOLUTION * _quality), 2, 1024);
    const float chunksPerSegmentInv = 1.0f / (float)chunksPerSegment;
    const Transform splineTransform = GetTransform();
    _instances.Resize(segments, false);
    BoundingBox localModelBounds(Vector3::Maximum, Vector3::Minimum);
    {
        auto& meshes = Model->LODs[0].Meshes;
        Vector3 corners[8];
        for (int32 j = 0; j < meshes.Count(); j++)
        {
            const auto& mesh = meshes[j];
            mesh.GetCorners(corners);

            for (int32 i = 0; i < 8; i++)
            {
                // Transform mesh corner using pre-transform but use double-precision to prevent issues when rotating model
                Vector3 tmp = corners[i] * _preTransform.Scale;
                double rotation[4] = { (double)_preTransform.Orientation.X, (double)_preTransform.Orientation.Y, (double)_preTransform.Orientation.Z, (double)_preTransform.Orientation.W };
                const double length = sqrt(rotation[0] * rotation[0] + rotation[1] * rotation[1] + rotation[2] * rotation[2] + rotation[3] * rotation[3]);
                const double inv = 1.0 / length;
                rotation[0] *= inv;
                rotation[1] *= inv;
                rotation[2] *= inv;
                rotation[3] *= inv;
                double pos[3] = { (double)tmp.X, (double)tmp.Y, (double)tmp.Z };
                const double x = rotation[0] + rotation[0];
                const double y = rotation[1] + rotation[1];
                const double z = rotation[2] + rotation[2];
                const double wx = rotation[3] * x;
                const double wy = rotation[3] * y;
                const double wz = rotation[3] * z;
                const double xx = rotation[0] * x;
                const double xy = rotation[0] * y;
                const double xz = rotation[0] * z;
                const double yy = rotation[1] * y;
                const double yz = rotation[1] * z;
                const double zz = rotation[2] * z;
                tmp = Vector3(
                    (float)(pos[0] * (1.0 - yy - zz) + pos[1] * (xy - wz) + pos[2] * (xz + wy)) + _preTransform.Translation.X,
                    (float)(pos[0] * (xy + wz) + pos[1] * (1.0 - xx - zz) + pos[2] * (yz - wx)) + _preTransform.Translation.Y,
                    (float)(pos[0] * (xz - wy) + pos[1] * (yz + wx) + pos[2] * (1.0 - xx - yy)) + _preTransform.Translation.Z);

                localModelBounds.Minimum = Vector3::Min(localModelBounds.Minimum, tmp);
                localModelBounds.Maximum = Vector3::Max(localModelBounds.Maximum, tmp);
            }
        }
    }
    _meshMinZ = localModelBounds.Minimum.Z;
    _meshMaxZ = localModelBounds.Maximum.Z;
    Transform chunkLocal, chunkWorld, leftTangent, rightTangent;
    Array<Vector3> segmentPoints;
    segmentPoints.Resize(chunksPerSegment);
    for (int32 segment = 0; segment < segments; segment++)
    {
        auto& instance = _instances[segment];
        const auto& start = keyframes[segment];
        const auto& end = keyframes[segment + 1];
        const float length = end.Time - start.Time;
        AnimationUtils::GetTangent(start.Value, start.TangentOut, length, leftTangent);
        AnimationUtils::GetTangent(end.Value, end.TangentIn, length, rightTangent);

        // Find maximum scale over the segment spline and collect the segment positions for bounds
        segmentPoints.Clear();
        segmentPoints.Add(end.Value.Translation);
        float maxScale = end.Value.Scale.GetAbsolute().MaxValue();
        for (int32 chunk = 0; chunk < chunksPerSegment; chunk++)
        {
            const float alpha = (float)chunk * chunksPerSegmentInv;
            AnimationUtils::Bezier(start.Value, leftTangent, rightTangent, end.Value, alpha, chunkLocal);
            splineTransform.LocalToWorld(chunkLocal, chunkWorld);
            segmentPoints.Add(chunkWorld.Translation);
            maxScale = Math::Max(maxScale, chunkWorld.Scale.GetAbsolute().MaxValue());
        }
        BoundingSphere::FromPoints(segmentPoints.Get(), segmentPoints.Count(), instance.Sphere);
        instance.Sphere.Radius *= maxScale * _boundsScale;
    }

    // Update deformation buffer during next drawing
    _deformationDirty = true;

    // Update bounds
    _sphere = _instances.First().Sphere;
    for (int32 i = 1; i < _instances.Count(); i++)
        BoundingSphere::Merge(_sphere, _instances[i].Sphere, _sphere);
    BoundingBox::FromSphere(_sphere, _box);
}

void SplineModel::UpdateDeformationBuffer()
{
    PROFILE_CPU();

    // Deformation buffer contains precomputed matrices for each chunk of the spline segment (packed with transposed float3x4 matrix)
    _deformationDirty = false;
    if (!_deformationBuffer)
        _deformationBuffer = GPUDevice::Instance->CreateBuffer(GetName());
    const auto& keyframes = _spline->Curve.GetKeyframes();
    const int32 segments = keyframes.Count() - 1;
    const int32 chunksPerSegment = Math::Clamp(Math::CeilToInt(SPLINE_RESOLUTION * _quality), 2, 1024);
    const int32 count = (chunksPerSegment * segments + 1) * 3;
    const uint32 size = count * sizeof(Vector4);
    if (_deformationBuffer->GetSize() != size)
    {
        if (_deformationBufferData)
        {
            Allocator::Free(_deformationBufferData);
            _deformationBufferData = nullptr;
        }
        if (_deformationBuffer->Init(GPUBufferDescription::Typed(count, PixelFormat::R32G32B32A32_Float, false, IsTransformStatic() ? GPUResourceUsage::Default : GPUResourceUsage::Dynamic)))
        {
            LOG(Error, "Failed to initialize the spline model {0} deformation buffer.", ToString());
            return;
        }
    }
    if (!_deformationBufferData)
        _deformationBufferData = Allocator::Allocate(size);
    _chunksPerSegment = (float)chunksPerSegment;

    // Update pre-calculated matrices for spline chunks
    auto ptr = (Matrix3x4*)_deformationBufferData;
    const float chunksPerSegmentInv = 1.0f / (float)chunksPerSegment;
    Matrix m;
    Transform transform, leftTangent, rightTangent;
    for (int32 segment = 0; segment < segments; segment++)
    {
        auto& instance = _instances[segment];
        const auto& start = keyframes[segment];
        const auto& end = keyframes[segment + 1];
        const float length = end.Time - start.Time;
        AnimationUtils::GetTangent(start.Value, start.TangentOut, length, leftTangent);
        AnimationUtils::GetTangent(end.Value, end.TangentIn, length, rightTangent);
        for (int32 chunk = 0; chunk < chunksPerSegment; chunk++)
        {
            const float alpha = (float)chunk * chunksPerSegmentInv;

            // Evaluate transformation at the curve
            AnimationUtils::Bezier(start.Value, leftTangent, rightTangent, end.Value, alpha, transform);

            // Apply spline direction (from position 1st derivative)
            Vector3 direction;
            AnimationUtils::BezierFirstDerivative(start.Value.Translation, leftTangent.Translation, rightTangent.Translation, end.Value.Translation, alpha, direction);
            direction.Normalize();
            Quaternion orientation;
            if (direction.IsZero())
                orientation = Quaternion::Identity;
            else if (Vector3::Dot(direction, Vector3::Up) >= 0.999f)
                Quaternion::RotationAxis(Vector3::Left, PI_HALF, orientation);
            else
                Quaternion::LookRotation(direction, Vector3::Cross(Vector3::Cross(direction, Vector3::Up), direction), orientation);
            transform.Orientation = orientation * transform.Orientation;

            // Write transform into deformation buffer
            transform.GetWorld(m);
            ptr->SetMatrixTranspose(m);
            ptr++;
        }
        instance.RotDeterminant = m.RotDeterminant();
    }

    // Add last transformation to prevent issues when sampling spline deformation buffer with alpha=1
    {
        const auto& start = keyframes[segments - 1];
        const auto& end = keyframes[segments];
        const float length = end.Time - start.Time;
        const float alpha = 1.0f - ZeroTolerance; // Offset to prevent zero derivative at the end of the curve
        AnimationUtils::GetTangent(start.Value, start.TangentOut, length, leftTangent);
        AnimationUtils::GetTangent(end.Value, end.TangentIn, length, rightTangent);
        AnimationUtils::Bezier(start.Value, leftTangent, rightTangent, end.Value, alpha, transform);
        Vector3 direction;
        AnimationUtils::BezierFirstDerivative(start.Value.Translation, leftTangent.Translation, rightTangent.Translation, end.Value.Translation, alpha, direction);
        direction.Normalize();
        Quaternion orientation;
        if (direction.IsZero())
            orientation = Quaternion::Identity;
        else if (Vector3::Dot(direction, Vector3::Up) >= 0.999f)
            Quaternion::RotationAxis(Vector3::Left, PI_HALF, orientation);
        else
            Quaternion::LookRotation(direction, Vector3::Cross(Vector3::Cross(direction, Vector3::Up), direction), orientation);
        transform.Orientation = orientation * transform.Orientation;
        transform.GetWorld(m);
        ptr->SetMatrixTranspose(m);
    }

    // Flush data with GPU
    auto context = GPUDevice::Instance->GetMainContext();
    context->UpdateBuffer(_deformationBuffer, _deformationBufferData, size);

    // Static splines are rarely updated so release scratch memory
    if (IsTransformStatic())
    {
        Allocator::Free(_deformationBufferData);
        _deformationBufferData = nullptr;
    }
}

void SplineModel::OnParentChanged()
{
    if (_spline)
    {
        _spline->SplineUpdated.Unbind<SplineModel, &SplineModel::OnSplineUpdated>(this);
    }

    // Base
    Actor::OnParentChanged();

    _spline = Cast<Spline>(_parent);
    if (_spline)
    {
        _spline->SplineUpdated.Bind<SplineModel, &SplineModel::OnSplineUpdated>(this);
    }

    OnSplineUpdated();
}

bool SplineModel::HasContentLoaded() const
{
    return (Model == nullptr || Model->IsLoaded()) && Entries.HasContentLoaded();
}

void SplineModel::Draw(RenderContext& renderContext)
{
    const DrawPass actorDrawModes = (DrawPass)(DrawModes & renderContext.View.Pass);
    if (!_spline || !Model || !Model->IsLoaded() || !Model->CanBeRendered() || actorDrawModes == DrawPass::None)
        return;
    auto model = Model.Get();
    if (!Entries.IsValidFor(model))
        Entries.Setup(model);

    // Build mesh deformation buffer for the whole spline
    if (_deformationDirty)
        UpdateDeformationBuffer();

    // Draw all segments
    DrawCall drawCall;
    drawCall.InstanceCount = 1;
    drawCall.Deformable.SplineDeformation = _deformationBuffer;
    drawCall.Deformable.ChunksPerSegment = _chunksPerSegment;
    drawCall.Deformable.MeshMinZ = _meshMinZ;
    drawCall.Deformable.MeshMaxZ = _meshMaxZ;
    drawCall.Deformable.GeometrySize = _box.GetSize();
    drawCall.PerInstanceRandom = GetPerInstanceRandom();
    _preTransform.GetWorld(drawCall.Deformable.LocalMatrix);
    const Transform splineTransform = GetTransform();
    splineTransform.GetWorld(drawCall.World);
    drawCall.ObjectPosition = drawCall.World.GetTranslation() + drawCall.Deformable.LocalMatrix.GetTranslation();
    const float worldDeterminantSign = drawCall.World.RotDeterminant() * drawCall.Deformable.LocalMatrix.RotDeterminant();
    for (int32 segment = 0; segment < _instances.Count(); segment++)
    {
        auto& instance = _instances[segment];
        if (!renderContext.View.CullingFrustum.Intersects(instance.Sphere))
            continue;
        drawCall.Deformable.Segment = (float)segment;

        // Select a proper LOD index (model may be culled)
        int32 lodIndex;
        if (_forcedLod != -1)
        {
            lodIndex = _forcedLod;
        }
        else
        {
            lodIndex = RenderTools::ComputeModelLOD(model, instance.Sphere.Center, instance.Sphere.Radius, renderContext);
            if (lodIndex == -1)
                continue;
        }
        lodIndex += _lodBias + renderContext.View.ModelLODBias;
        lodIndex = model->ClampLODIndex(lodIndex);

        // Draw
        const auto& lod = model->LODs[lodIndex];
        for (int32 i = 0; i < lod.Meshes.Count(); i++)
        {
            const auto mesh = &lod.Meshes[i];

            // Cache data
            const auto& entry = Entries[mesh->GetMaterialSlotIndex()];
            if (!entry.Visible || !mesh->IsInitialized())
                continue;
            const MaterialSlot& slot = model->MaterialSlots[mesh->GetMaterialSlotIndex()];

            // Check if skip rendering
            const auto shadowsMode = static_cast<ShadowsCastingMode>(entry.ShadowsMode & slot.ShadowsMode);
            const auto drawModes = static_cast<DrawPass>(actorDrawModes & renderContext.View.GetShadowsDrawPassMask(shadowsMode));
            if (drawModes == DrawPass::None)
                continue;

            // Select material
            MaterialBase* material = nullptr;
            if (entry.Material && entry.Material->IsLoaded())
                material = entry.Material;
            else if (slot.Material && slot.Material->IsLoaded())
                material = slot.Material;
            if (!material || !material->IsDeformable())
                material = GPUDevice::Instance->GetDefaultDeformableMaterial();
            if (!material || !material->IsDeformable())
                continue;

            // Submit draw call
            mesh->GetDrawCallGeometry(drawCall);
            drawCall.Material = material;
            drawCall.WorldDeterminantSign = Math::FloatSelect(worldDeterminantSign * instance.RotDeterminant, 1, -1);
            renderContext.List->AddDrawCall(drawModes, _staticFlags, drawCall, entry.ReceiveDecals);
        }
    }
}

void SplineModel::DrawGeneric(RenderContext& renderContext)
{
    Draw(renderContext);
}

bool SplineModel::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    return false;
}

void SplineModel::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    ModelInstanceActor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(SplineModel);

    SERIALIZE_MEMBER(Quality, _quality);
    SERIALIZE_MEMBER(BoundsScale, _boundsScale);
    SERIALIZE_MEMBER(LODBias, _lodBias);
    SERIALIZE_MEMBER(ForcedLOD, _forcedLod);
    SERIALIZE_MEMBER(PreTransform, _preTransform)
    SERIALIZE(Model);
    SERIALIZE(DrawModes);

    stream.JKEY("Buffer");
    stream.Object(&Entries, other ? &other->Entries : nullptr);
}

void SplineModel::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    ModelInstanceActor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Quality, _quality);
    DESERIALIZE_MEMBER(BoundsScale, _boundsScale);
    DESERIALIZE_MEMBER(LODBias, _lodBias);
    DESERIALIZE_MEMBER(ForcedLOD, _forcedLod);
    DESERIALIZE_MEMBER(PreTransform, _preTransform);
    DESERIALIZE(Model);
    DESERIALIZE(DrawModes);

    Entries.DeserializeIfExists(stream, "Buffer", modifier);
}

void SplineModel::OnTransformChanged()
{
    // Base
    ModelInstanceActor::OnTransformChanged();

    OnSplineUpdated();
}
