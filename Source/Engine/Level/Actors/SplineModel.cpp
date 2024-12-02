// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "SplineModel.h"
#include "Spline.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Matrix3x4.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Graphics/GPUBufferDescription.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Renderer/RenderList.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif

#define SPLINE_RESOLUTION 32.0f

SplineModel::SplineModel(const SpawnParams& params)
    : ModelInstanceActor(params)
{
    _drawCategory = SceneRendering::SceneDrawAsync;
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
        _box = BoundingBox(_transform.Translation);
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
            mesh.GetBox().GetCorners(corners);

            for (int32 i = 0; i < 8; i++)
            {
                // Transform mesh corner using pre-transform but use double-precision to prevent issues when rotating model
                Vector3 tmp = corners[i] * _preTransform.Scale;
                double rotation[4] = { (double)_preTransform.Orientation.X, (double)_preTransform.Orientation.Y, (double)_preTransform.Orientation.Z, (double)_preTransform.Orientation.W };
                const double length = sqrt(rotation[0] * rotation[0] + rotation[1] * rotation[1] + rotation[2] * rotation[2] + rotation[3] * rotation[3]);
                rotation[0] /= length;
                rotation[1] /= length;
                rotation[2] /= length;
                rotation[3] /= length;
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
    _meshMinZ = (float)localModelBounds.Minimum.Z;
    _meshMaxZ = (float)localModelBounds.Maximum.Z;
    Transform chunkLocal, chunkWorld, leftTangent, rightTangent;
    Array<Vector3> segmentPoints;
    segmentPoints.Resize(chunksPerSegment);
    for (int32 segment = 0; segment < segments; segment++)
    {
        auto& instance = _instances[segment];
        const auto& start = keyframes[segment];
        const auto& end = keyframes[segment + 1];
        const float tangentScale = (end.Time - start.Time) / 3.0f;
        AnimationUtils::GetTangent(start.Value, start.TangentOut, tangentScale, leftTangent);
        AnimationUtils::GetTangent(end.Value, end.TangentIn, tangentScale, rightTangent);

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
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey);
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
    const uint32 size = count * sizeof(Float4);
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
        const float tangentScale = (end.Time - start.Time) / 3.0f;
        AnimationUtils::GetTangent(start.Value, start.TangentOut, tangentScale, leftTangent);
        AnimationUtils::GetTangent(end.Value, end.TangentIn, tangentScale, rightTangent);
        for (int32 chunk = 0; chunk < chunksPerSegment; chunk++)
        {
            const float alpha = (chunk == chunksPerSegment - 1) ? 1.0f : ((float)chunk * chunksPerSegmentInv);

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
        const float tangentScale = (end.Time - start.Time) / 3.0f;
        const float alpha = 1.0f - ZeroTolerance; // Offset to prevent zero derivative at the end of the curve
        AnimationUtils::GetTangent(start.Value, start.TangentOut, tangentScale, leftTangent);
        AnimationUtils::GetTangent(end.Value, end.TangentIn, tangentScale, rightTangent);
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
    GPUDevice::Instance->GetMainContext()->UpdateBuffer(_deformationBuffer, _deformationBufferData, size);

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

const Span<MaterialSlot> SplineModel::GetMaterialSlots() const
{
    const auto model = Model.Get();
    if (model && !model->WaitForLoaded())
        return ToSpan(model->MaterialSlots);
    return Span<MaterialSlot>();
}

MaterialBase* SplineModel::GetMaterial(int32 entryIndex)
{
    if (Model)
        Model->WaitForLoaded();
    else
        return nullptr;
    CHECK_RETURN(entryIndex >= 0 && entryIndex < Entries.Count(), nullptr);
    MaterialBase* material = Entries[entryIndex].Material.Get();
    if (!material)
    {
        material = Model->MaterialSlots[entryIndex].Material.Get();
        if (!material)
            material = GPUDevice::Instance->GetDefaultDeformableMaterial();
    }
    return material;
}

void SplineModel::UpdateBounds()
{
    OnSplineUpdated();
}

bool SplineModel::HasContentLoaded() const
{
    return (Model == nullptr || Model->IsLoaded()) && Entries.HasContentLoaded();
}

void SplineModel::Draw(RenderContext& renderContext)
{
    const DrawPass actorDrawModes = DrawModes & renderContext.View.Pass;
    if (!_spline || !Model || !Model->IsLoaded() || !Model->CanBeRendered() || actorDrawModes == DrawPass::None)
        return;
    auto model = Model.Get();
    if (renderContext.View.Pass == DrawPass::GlobalSDF)
        return; // TODO: Spline Model rendering to Global SDF
    if (renderContext.View.Pass == DrawPass::GlobalSurfaceAtlas)
        return; // TODO: Spline Model rendering to Global Surface Atlas
    if (!Entries.IsValidFor(model))
        Entries.Setup(model);

    // Build mesh deformation buffer for the whole spline
    if (_deformationDirty)
    {
        RenderContext::GPULocker.Lock();
        UpdateDeformationBuffer();
        RenderContext::GPULocker.Unlock();
    }

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
    renderContext.View.GetWorldMatrix(splineTransform, drawCall.World);
    drawCall.ObjectPosition = drawCall.World.GetTranslation() + drawCall.Deformable.LocalMatrix.GetTranslation();
    drawCall.ObjectRadius = (float)_sphere.Radius; // TODO: use radius for the spline chunk rather than whole spline
    const float worldDeterminantSign = drawCall.World.RotDeterminant() * drawCall.Deformable.LocalMatrix.RotDeterminant();
    for (int32 segment = 0; segment < _instances.Count(); segment++)
    {
        auto& instance = _instances[segment];
        BoundingSphere instanceSphere(instance.Sphere.Center - renderContext.View.Origin, instance.Sphere.Radius);
        if (!(renderContext.View.IsCullingDisabled || renderContext.View.CullingFrustum.Intersects(instanceSphere)))
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
            lodIndex = RenderTools::ComputeModelLOD(model, instanceSphere.Center, (float)instanceSphere.Radius, renderContext);
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

            // Check if skip rendering
            const auto shadowsMode = entry.ShadowsMode & slot.ShadowsMode;
            const auto drawModes = actorDrawModes & renderContext.View.GetShadowsDrawPassMask(shadowsMode) & material->GetDrawModes();
            if (drawModes == DrawPass::None)
                continue;

            // Submit draw call
            mesh->GetDrawCallGeometry(drawCall);
            drawCall.Material = material;
            drawCall.WorldDeterminantSign = Math::FloatSelect(worldDeterminantSign * instance.RotDeterminant, 1, -1);
            renderContext.List->AddDrawCall(renderContext, drawModes, _staticFlags, drawCall, entry.ReceiveDecals);
        }
    }
}

bool SplineModel::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
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

    // [Deprecated on 07.02.2022, expires on 07.02.2024]
    if (modifier->EngineBuild <= 6330)
        DrawModes |= DrawPass::GlobalSDF;
    // [Deprecated on 27.04.2022, expires on 27.04.2024]
    if (modifier->EngineBuild <= 6331)
        DrawModes |= DrawPass::GlobalSurfaceAtlas;
}

void SplineModel::OnActiveInTreeChanged()
{
    // Base
    ModelInstanceActor::OnActiveInTreeChanged();

    OnSplineUpdated();
}
