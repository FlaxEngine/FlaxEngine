// Copyright (c) Wojciech Figat. All rights reserved.

#include "ParticleEffect.h"
#include "Particles.h"
#include "Engine/Core/Types/CommonValue.h"
#include "Engine/Content/Deprecated.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/Engine.h"

ParticleEffect::ParticleEffect(const SpawnParams& params)
    : Actor(params)
    , _lastUpdateFrame(0)
    , _lastMinDstSqr(MAX_Real)
{
    _box = BoundingBox(_transform.Translation);
    BoundingSphere::FromBox(_box, _sphere);

    ParticleSystem.Changed.Bind<ParticleEffect, &ParticleEffect::OnParticleSystemModified>(this);
    ParticleSystem.Loaded.Bind<ParticleEffect, &ParticleEffect::OnParticleSystemLoaded>(this);
}

void ParticleEffectParameter::Init(ParticleEffect* effect, int32 emitterIndex, int32 paramIndex)
{
    _effect = effect;
    _emitterIndex = emitterIndex;
    _paramIndex = paramIndex;
}

bool ParticleEffectParameter::IsValid() const
{
    return _effect->ParticleSystem &&
            _effect->Instance.Emitters.Count() > _emitterIndex &&
            _effect->ParticleSystem->Emitters[_emitterIndex] &&
            _effect->ParticleSystem->Emitters[_emitterIndex]->Graph.Parameters.Count() > _paramIndex;
}

ParticleEmitter* ParticleEffectParameter::GetEmitter() const
{
    CHECK_RETURN(IsValid(), nullptr);
    return _effect->ParticleSystem->Emitters[_emitterIndex];
}

VariantType ParticleEffectParameter::GetParamType() const
{
    CHECK_RETURN(IsValid(), VariantType(VariantType::Bool));
    return _effect->ParticleSystem->Emitters[_emitterIndex]->Graph.Parameters[_paramIndex].Type;
}

Guid ParticleEffectParameter::GetParamIdentifier() const
{
    CHECK_RETURN(IsValid(), Guid::Empty);
    return _effect->ParticleSystem->Emitters[_emitterIndex]->Graph.Parameters[_paramIndex].Identifier;
}

const String& ParticleEffectParameter::GetTrackName() const
{
    CHECK_RETURN(IsValid(), String::Empty);
    auto system = _effect->ParticleSystem.Get();
    for (int32 i = 0; i < system->Tracks.Count(); i++)
    {
        auto& track = system->Tracks[i];
        if (track.Type == ParticleSystem::Track::Types::Emitter && track.AsEmitter.Index == _emitterIndex)
        {
            return track.Name;
        }
    }
    return String::Empty;
}

const String& ParticleEffectParameter::GetName() const
{
    CHECK_RETURN(IsValid(), String::Empty);
    return _effect->ParticleSystem->Emitters[_emitterIndex]->Graph.Parameters[_paramIndex].Name;
}

bool ParticleEffectParameter::GetIsPublic() const
{
    CHECK_RETURN(IsValid(), false);
    return _effect->ParticleSystem->Emitters[_emitterIndex]->Graph.Parameters[_paramIndex].IsPublic;
}

Variant ParticleEffectParameter::GetDefaultValue() const
{
    CHECK_RETURN(IsValid(), Variant::False);
    const ParticleSystemParameter& param = _effect->ParticleSystem->Emitters[_emitterIndex]->Graph.Parameters[_paramIndex];
    Variant paramValue = param.Value;
    _effect->ParticleSystem->EmittersParametersOverrides.TryGet(ToPair(_emitterIndex, param.Identifier), paramValue);
    return paramValue;
}

const Variant& ParticleEffectParameter::GetDefaultEmitterValue() const
{
    CHECK_RETURN(IsValid(), Variant::False);
    const ParticleSystemParameter& param = _effect->ParticleSystem->Emitters[_emitterIndex]->Graph.Parameters[_paramIndex];
    return param.Value;
}

const Variant& ParticleEffectParameter::GetValue() const
{
    CHECK_RETURN(IsValid(), Variant::False);
    const Variant& paramValue = _effect->Instance.Emitters[_emitterIndex].Parameters[_paramIndex];
    return paramValue;
}

void ParticleEffectParameter::SetValue(const Variant& value) const
{
    CHECK(IsValid());
    _effect->Instance.Emitters[_emitterIndex].Parameters[_paramIndex] = value;
}

GraphParameter* ParticleEffectParameter::GetEmitterParameter() const
{
    CHECK_RETURN(IsValid(), nullptr);
    ParticleSystemParameter& param = _effect->ParticleSystem->Emitters[_emitterIndex]->Graph.Parameters[_paramIndex];
    return &param;
}

const Array<ParticleEffectParameter>& ParticleEffect::GetParameters()
{
    Sync();

    if (_parametersVersion != Instance.ParametersVersion)
    {
        _parametersVersion = Instance.ParametersVersion;

        int32 count = 0;
        for (auto& e : Instance.Emitters)
            count += e.Parameters.Count();
        _parameters.Clear();
        _parameters.Resize(count, false);

        int32 index = 0;
        for (int32 emitterIndex = 0; emitterIndex < Instance.Emitters.Count(); emitterIndex++)
        {
            auto& emitter = Instance.Emitters[emitterIndex];
            for (int32 paramIndex = 0; paramIndex < emitter.Parameters.Count(); paramIndex++)
            {
                _parameters[index++].Init(this, emitterIndex, paramIndex);
            }
        }

        ApplyModifiedParameters();
    }

    return _parameters;
}

uint32 ParticleEffect::GetParametersVersion() const
{
    return Instance.ParametersVersion;
}

ParticleEffectParameter* ParticleEffect::GetParameter(const StringView& emitterTrackName, const StringView& paramName)
{
    auto& parameters = GetParameters();
    if (parameters.IsEmpty())
        return nullptr;

    auto system = ParticleSystem.Get();
    for (int32 i = 0; i < system->Tracks.Count(); i++)
    {
        auto& track = system->Tracks[i];
        if (track.Type == ParticleSystem::Track::Types::Emitter && track.Name == emitterTrackName)
        {
            const int32 emitterIndex = track.AsEmitter.Index;
            for (auto& param : parameters)
            {
                if (param.GetEmitterIndex() == emitterIndex && param.GetName() == paramName)
                {
                    return (ParticleEffectParameter*)&param;
                }
            }
        }
    }

    return nullptr;
}

ParticleEffectParameter* ParticleEffect::GetParameter(const StringView& emitterTrackName, const Guid& paramId)
{
    auto& parameters = GetParameters();
    if (parameters.IsEmpty())
        return nullptr;

    auto system = ParticleSystem.Get();
    for (int32 i = 0; i < system->Tracks.Count(); i++)
    {
        auto& track = system->Tracks[i];
        if (track.Type == ParticleSystem::Track::Types::Emitter && track.Name == emitterTrackName)
        {
            const int32 emitterIndex = track.AsEmitter.Index;
            for (auto& param : parameters)
            {
                if (param.GetEmitterIndex() == emitterIndex && param.GetParamIdentifier() == paramId)
                {
                    return (ParticleEffectParameter*)&param;
                }
            }
        }
    }

    return nullptr;
}

const Variant& ParticleEffect::GetParameterValue(const StringView& emitterTrackName, const StringView& paramName)
{
    const auto param = GetParameter(emitterTrackName, paramName);
    CHECK_RETURN(param, Variant::Null);
    return param->GetValue();
}

void ParticleEffect::SetParameterValue(const StringView& emitterTrackName, const StringView& paramName, const Variant& value)
{
    auto param = GetParameter(emitterTrackName, paramName);
    CHECK(param);
    param->SetValue(value);
}

void ParticleEffect::ResetParameters()
{
    _parametersOverrides.Clear();
    auto& parameters = GetParameters();
    for (auto& p : parameters)
    {
        p.SetValue(p.GetDefaultValue());
    }
}

float ParticleEffect::GetTime() const
{
    return Instance.Time;
}

void ParticleEffect::SetTime(float time)
{
    Instance.Time = time;
}

float ParticleEffect::GetLastUpdateTime() const
{
    return Instance.LastUpdateTime;
}

void ParticleEffect::SetLastUpdateTime(float time)
{
    Instance.LastUpdateTime = time;
}

int32 ParticleEffect::GetParticlesCount() const
{
    return Instance.GetParticlesCount();
}

bool ParticleEffect::GetIsPlaying() const
{
    return _isPlaying;
}

void ParticleEffect::ResetSimulation()
{
    Instance.ClearState();
}

void ParticleEffect::UpdateSimulation(bool singleFrame)
{
    // Skip if need to
    if (!IsActiveInHierarchy()
        || ParticleSystem == nullptr
        || !ParticleSystem->IsLoaded()
        || _lastUpdateFrame == Engine::UpdateCount)
        return;

    // Request update
    _lastUpdateFrame = Engine::UpdateCount;
    _lastMinDstSqr = MAX_Real;
    if (singleFrame)
        Instance.LastUpdateTime = (UseTimeScale ? Time::Update.Time : Time::Update.UnscaledTime).GetTotalSeconds();
    Particles::UpdateEffect(this);
}

void ParticleEffect::SpawnParticles(int32 count, const StringView& emitterTrackName)
{
    auto system = ParticleSystem.Get();
    if (!system)
        return;
    if (emitterTrackName.IsEmpty())
    {
        for (auto& e : Instance.Emitters)
            e.CustomSpawnCount += count;
    }
    else
    {
        for (int32 i = 0; i < system->Tracks.Count(); i++)
        {
            auto& track = system->Tracks[i];
            if (track.Type == ParticleSystem::Track::Types::Emitter && track.Name == emitterTrackName)
            {
                const int32 emitterIndex = track.AsEmitter.Index;
                if (Instance.Emitters.IsValidIndex(emitterIndex))
                {
                    Instance.Emitters.Get()[emitterIndex].CustomSpawnCount += count;
                    break;
                }
            }
        }
    }
}

void ParticleEffect::Play()
{
    _isPlaying = true;
    _isStopped = false;
}

void ParticleEffect::Pause()
{
    _isPlaying = false;
}

void ParticleEffect::Stop()
{
    _isStopped = true;
    _isPlaying = false;
    ResetSimulation();
}

void ParticleEffect::UpdateBounds()
{
    BoundingBox bounds = BoundingBox::Empty;
    const auto particleSystem = ParticleSystem.Get();
    if (particleSystem && Instance.LastUpdateTime >= 0)
    {
        for (int32 j = 0; j < particleSystem->Tracks.Count(); j++)
        {
            const auto& track = particleSystem->Tracks[j];
            if (track.Type != ParticleSystem::Track::Types::Emitter || track.Disabled)
                continue;
            const int32 emitterIndex = track.AsEmitter.Index;
            ParticleEmitter* emitter = particleSystem->Emitters[emitterIndex].Get();
            if (!emitter || emitter->Capacity == 0 || emitter->Graph.Layout.Size == 0 || Instance.Emitters.Count() <= emitterIndex)
                continue;
            auto& data = Instance.Emitters[emitterIndex];

            BoundingBox emitterBounds;
            if (emitter->GraphExecutorCPU.ComputeBounds(emitter, this, data, emitterBounds))
            {
                ASSERT(!emitterBounds.Minimum.IsNanOrInfinity() && !emitterBounds.Maximum.IsNanOrInfinity());

                if (emitter->SimulationSpace == ParticlesSimulationSpace::Local)
                {
                    BoundingBox::Transform(emitterBounds, _transform, emitterBounds);
                }

                BoundingBox::Merge(emitterBounds, bounds, bounds);
            }
        }
    }

    // Empty bounds if there is no particle system to play or it has been never played
    if (bounds == BoundingBox::Empty)
    {
        bounds = BoundingBox(_transform.Translation);
    }

    _box = bounds;
    BoundingSphere::FromBox(bounds, _sphere);
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey);
}

void ParticleEffect::Sync()
{
    auto system = ParticleSystem.Get();
    if (!system || system->WaitForLoaded())
    {
        Instance.ClearState();
        return;
    }

    Instance.Sync(system);

    for (int32 i = 0; i < system->Tracks.Count(); i++)
    {
        auto& track = system->Tracks[i];
        if (track.Type == ParticleSystem::Track::Types::Emitter)
        {
            const int32 emitterIndex = track.AsEmitter.Index;
            const auto emitter = system->Emitters[emitterIndex].Get();
            if (emitter)
            {
                Instance.Emitters[emitterIndex].Sync(Instance, system, emitterIndex);
            }
        }
    }
}

SceneRenderTask* ParticleEffect::GetRenderTask() const
{
    const uint64 minFrame = Engine::FrameCount - 2;

    // Custom task
    const auto customViewRenderTask = CustomViewRenderTask.Get();
    if (customViewRenderTask && customViewRenderTask->Enabled && customViewRenderTask->LastUsedFrame >= minFrame)
        return customViewRenderTask;

    // Main task
    const auto mainRenderTask = MainRenderTask::Instance;
    if (mainRenderTask && mainRenderTask->Enabled && mainRenderTask->LastUsedFrame >= minFrame)
        return mainRenderTask;

    // Editor viewport
#if USE_EDITOR
    for (auto task : RenderTask::Tasks)
    {
        if (task->LastUsedFrame >= minFrame && task->Enabled)
        {
            if (const auto sceneRenderTask = Cast<SceneRenderTask>(task))
            {
                if (sceneRenderTask->ActorsSource == ActorsSources::Scenes)
                {
                    return sceneRenderTask;
                }
            }
        }
    }
#endif
    return nullptr;
}

#if USE_EDITOR

Array<ParticleEffect::ParameterOverride>& ParticleEffect::GetParametersOverrides()
{
    CacheModifiedParameters();
    return _parametersOverrides;
}

void ParticleEffect::SetParametersOverrides(const Array<ParameterOverride>& value)
{
    ResetParameters();
    _parametersOverrides = value;
    ApplyModifiedParameters();
}

#endif

void ParticleEffect::Update()
{
    if (!_isPlaying)
    {
        // Move update timer forward while paused for correct delta time after unpause
        Instance.LastUpdateTime = (UseTimeScale ? Time::Update.Time : Time::Update.UnscaledTime).GetTotalSeconds();
        return;
    }

    // Skip if off-screen
    if (!UpdateWhenOffscreen && _lastMinDstSqr >= MAX_Real)
        return;

    if (UpdateMode == SimulationUpdateMode::FixedTimestep)
    {
        // Check if last simulation update was past enough to kick a new on
        const float time = Time::Update.Time.GetTotalSeconds();
        if (time - Instance.LastUpdateTime < FixedTimestep)
            return;
    }

    UpdateSimulation();
}

#if USE_EDITOR

#include "Editor/Editor.h"
#include "Editor/Managed/ManagedEditor.h"

void ParticleEffect::UpdateExecuteInEditor()
{
    // Auto-play in Editor
    if (!Editor::IsPlayMode && !_isStopped && IsLooping && PlayOnStart && Editor::Managed->ManagedEditorOptions.EnableParticlesPreview)
    {
        _isPlaying = true;
        Update();
    }
    else if (!Editor::IsPlayMode && _isPlaying)
    {
        _isPlaying = false;
        ResetSimulation();
    }
}

#endif

void ParticleEffect::CacheModifiedParameters()
{
    if (_parameters.IsEmpty())
        return;
    _parametersOverrides.Clear();
    auto& parameters = GetParameters();
    for (auto& param : parameters)
    {
        if (param.GetValue() != param.GetDefaultValue())
        {
            auto& e = _parametersOverrides.AddOne();
            e.Track = param.GetTrackName();
            e.Id = param.GetParamIdentifier();
            e.Value = param.GetValue();
        }
    }
}

void ParticleEffect::ApplyModifiedParameters()
{
    if (_parametersOverrides.IsEmpty())
        return;

    // Parameters getter applies the parameters overrides
    if (_parameters.IsEmpty())
    {
        GetParameters();
        return;
    }

    for (auto& e : _parametersOverrides)
    {
        const auto param = GetParameter(e.Track, e.Id);
        if (param)
        {
            param->SetValue(e.Value);
        }
        else
        {
            LOG(Warning, "Failed to apply the particle effect parameter (id={0} from track={1})", e.Id, e.Track);
        }
    }
}

void ParticleEffect::OnParticleSystemModified()
{
    Instance.ClearState();
    _parameters.Resize(0);
    _parametersVersion = 0;
}

void ParticleEffect::OnParticleSystemLoaded()
{
    ApplyModifiedParameters();
#if USE_EDITOR
    // When one of the emitters gets edited, cached parameters need to be applied
    auto& emitters = ParticleSystem.Get()->Emitters;
    for (auto& emitter : emitters)
    {
        emitter.Loaded.BindUnique<ParticleEffect, &ParticleEffect::OnParticleEmitterLoaded>(this);
    }
#endif
}

void ParticleEffect::OnParticleEmitterLoaded()
{
    ApplyModifiedParameters();
}

bool ParticleEffect::HasContentLoaded() const
{
    if (ParticleSystem == nullptr)
        return true;
    if (!ParticleSystem->IsLoaded())
        return false;
    for (int32 i = 0; i < ParticleSystem->Emitters.Count(); i++)
    {
        const auto emitter = ParticleSystem->Emitters[i].Get();
        if (emitter && !emitter->IsLoaded())
            return false;
    }
    return true;
}

void ParticleEffect::Draw(RenderContext& renderContext)
{
    if (renderContext.View.Pass == DrawPass::GlobalSDF || renderContext.View.Pass == DrawPass::GlobalSurfaceAtlas)
        return;
    _lastMinDstSqr = Math::Min(_lastMinDstSqr, Vector3::DistanceSquared(GetPosition(), renderContext.View.Position));
    Particles::DrawParticles(renderContext, this);
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void ParticleEffect::OnDebugDrawSelected()
{
    DEBUG_DRAW_WIRE_BOX(_box, Color::Violet * 0.7f, 0, true);

    Actor::OnDebugDrawSelected();
}

void ParticleEffect::OnDebugDraw()
{
    if (ShowDebugDraw)
    {
        Particles::DebugDraw(this);
    }

    Actor::OnDebugDraw();
}

#endif

void ParticleEffect::OnLayerChanged()
{
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Layer);
}

void ParticleEffect::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(ParticleEffect);

    const auto& params = GetParameters();
    const auto* otherParams = other ? &((ParticleEffect*)other)->GetParameters() : nullptr;
    {
        stream.JKEY("Overrides");
        stream.StartArray();
        for (int32 i = 0; i < params.Count(); i++)
        {
            auto& param = params[i];
            if (otherParams)
            {
                if (otherParams->IsEmpty())
                {
                    // Serialize only parameters values overriding the default value (from system)
                    if (param.GetValue() == param.GetDefaultValue())
                        continue;
                }
                else
                {
                    // Serialize only parameters values overriding the other object value (from diff)
                    const auto& otherParam = (*otherParams)[i];
                    if (param.GetValue() == otherParam.GetValue())
                        continue;
                }
            }

            stream.StartObject();

            stream.JKEY("Track");
            stream.String(param.GetTrackName());

            stream.JKEY("Id");
            stream.Guid(param.GetParamIdentifier());

            stream.JKEY("Value");
            Serialization::Serialize(stream, param.GetValue(), nullptr);

            stream.EndObject();
        }
        stream.EndArray();
    }

    SERIALIZE(ParticleSystem);
    SERIALIZE(UpdateMode);
    SERIALIZE(FixedTimestep);
    SERIALIZE(SimulationSpeed);
    SERIALIZE(UseTimeScale);
    SERIALIZE(IsLooping);
    SERIALIZE(PlayOnStart);
    SERIALIZE(UpdateWhenOffscreen);
    SERIALIZE(DrawModes);
    SERIALIZE(SortOrder);
}

void ParticleEffect::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    const auto overridesMember = stream.FindMember("Overrides");
    if (overridesMember != stream.MemberEnd())
    {
        // [Deprecated on 25.11.2018, expires on 25.11.2022]
        if (modifier->EngineBuild < 6197)
        {
            PRAGMA_DISABLE_DEPRECATION_WARNINGS
            MARK_CONTENT_DEPRECATED();
            const auto& overrides = overridesMember->value;
            ASSERT(overrides.IsArray());
            _parametersOverrides.EnsureCapacity(_parametersOverrides.Count() + overrides.Size());
            for (rapidjson::SizeType i = 0; i < overrides.Size(); i++)
            {
                const auto& o = (DeserializeStream&)overrides[i];
                const String trackName = JsonTools::GetString(o, "Track");
                const Guid id = JsonTools::GetGuid(o, "Id");
                ParameterOverride* e = nullptr;
                for (auto& q : _parametersOverrides)
                {
                    if (q.Id == id && q.Track == trackName)
                    {
                        e = &q;
                        break;
                    }
                }
                if (e)
                {
                    // Update overriden parameter value
                    CommonValue value;
                    auto mValue = SERIALIZE_FIND_MEMBER(o, "Value");
                    if (mValue != o.MemberEnd())
                        e->Value = Variant(JsonTools::GetCommonValue(mValue->value));
                }
                else
                {
                    // Add parameter override
                    auto& p = _parametersOverrides.AddOne();
                    p.Track = trackName;
                    p.Id = id;
                    CommonValue value;
                    auto mValue = SERIALIZE_FIND_MEMBER(o, "Value");
                    if (mValue != o.MemberEnd())
                        p.Value = Variant(JsonTools::GetCommonValue(mValue->value));
                }
            }
            PRAGMA_ENABLE_DEPRECATION_WARNINGS
        }
        else
        {
            const auto& overrides = overridesMember->value;
            ASSERT(overrides.IsArray());
            _parametersOverrides.EnsureCapacity(_parametersOverrides.Count() + overrides.Size());
            for (rapidjson::SizeType i = 0; i < overrides.Size(); i++)
            {
                auto& o = (DeserializeStream&)overrides[i];
                const String trackName = JsonTools::GetString(o, "Track");
                const Guid id = JsonTools::GetGuid(o, "Id");
                ParameterOverride* e = nullptr;
                for (auto& q : _parametersOverrides)
                {
                    if (q.Id == id && q.Track == trackName)
                    {
                        e = &q;
                        break;
                    }
                }
                if (e)
                {
                    // Update overriden parameter value
                    const auto mValue = SERIALIZE_FIND_MEMBER(o, "Value");
                    if (mValue != stream.MemberEnd())
                        Serialization::Deserialize(mValue->value, e->Value, modifier);
                }
                else
                {
                    // Add parameter override
                    auto& p = _parametersOverrides.AddOne();
                    p.Track = trackName;
                    p.Id = id;
                    const auto mValue = SERIALIZE_FIND_MEMBER(o, "Value");
                    if (mValue != stream.MemberEnd())
                        Serialization::Deserialize(mValue->value, p.Value, modifier);
                }
            }
        }
    }

    DESERIALIZE(ParticleSystem);
    DESERIALIZE(UpdateMode);
    DESERIALIZE(FixedTimestep);
    DESERIALIZE(SimulationSpeed);
    DESERIALIZE(UseTimeScale);
    DESERIALIZE(IsLooping);
    DESERIALIZE(PlayOnStart);
    DESERIALIZE(UpdateWhenOffscreen);
    DESERIALIZE(DrawModes);
    DESERIALIZE(SortOrder);

    if (_parameters.HasItems())
    {
        ApplyModifiedParameters();
    }
}

void ParticleEffect::EndPlay()
{
    CacheModifiedParameters();
    Particles::OnEffectDestroy(this);
    Instance.ClearState();
    _parameters.Clear();
    _parametersVersion = 0;

    // Base
    Actor::EndPlay();
}

void ParticleEffect::OnEnable()
{
    GetScene()->Ticking.Update.AddTick<ParticleEffect, &ParticleEffect::Update>(this);
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
    GetScene()->Ticking.Update.AddTickExecuteInEditor<ParticleEffect, &ParticleEffect::UpdateExecuteInEditor>(this);
#endif

    if (PlayOnStart)
        Play();

    // Base
    Actor::OnEnable();
}

void ParticleEffect::OnDisable()
{
#if USE_EDITOR
    GetScene()->Ticking.Update.RemoveTickExecuteInEditor(this);
    GetSceneRendering()->RemoveViewportIcon(this);
#endif
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);
    GetScene()->Ticking.Update.RemoveTick(this);

    // Base
    Actor::OnDisable();
}

void ParticleEffect::OnActiveInTreeChanged()
{
    // Base
    Actor::OnActiveInTreeChanged();

    if (!IsActiveInHierarchy())
    {
        // Invalidate the simulation
        CacheModifiedParameters();
        Instance.ClearState();
    }
    else
    {
        ApplyModifiedParameters();
    }
}

void ParticleEffect::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    UpdateBounds();
}
