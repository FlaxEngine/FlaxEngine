// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Audio.h"
#include "AudioBackend.h"
#include "AudioSettings.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Level/Level.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/EngineService.h"
#if AUDIO_API_NONE
#include "None/AudioBackendNone.h"
#endif
#if AUDIO_API_PS4
#include "Platforms/PS4/Engine/Audio/AudioBackendPS4.h"
#endif
#if AUDIO_API_OPENAL
#include "OpenAL/AudioBackendOAL.h"
#endif
#if AUDIO_API_XAUDIO2
#include "XAudio2/AudioBackendXAudio2.h"
#endif

const Char* ToString(AudioFormat value)
{
    switch (value)
    {
    case AudioFormat::Raw:
        return TEXT("Raw");
    case AudioFormat::Vorbis:
        return TEXT("Vorbis");
    default:
        return TEXT("");
    }
}

Array<AudioListener*> Audio::Listeners;
Array<AudioSource*> Audio::Sources;
Array<AudioDevice> Audio::Devices;
Action Audio::DevicesChanged;
Action Audio::ActiveDeviceChanged;
AudioBackend* AudioBackend::Instance = nullptr;

namespace
{
    float MasterVolume = 1.0f;
    float Volume = 1.0f;
    int32 ActiveDeviceIndex = -1;
    bool MuteOnFocusLoss = true;
}

class AudioService : public EngineService
{
public:

    AudioService()
        : EngineService(TEXT("Audio"), -50)
    {
    }

    bool Init() override;
    void Update() override;
    void Dispose() override;
};

AudioService AudioServiceInstance;

namespace
{
    void OnEnginePause()
    {
        AudioBackend::SetVolume(0.0f);
    }

    void OnEngineUnpause()
    {
        AudioBackend::SetVolume(Volume);
    }
}

void AudioSettings::Apply()
{
    ::MuteOnFocusLoss = MuteOnFocusLoss;
}

AudioDevice* Audio::GetActiveDevice()
{
    return &Devices[ActiveDeviceIndex];
}

int32 Audio::GetActiveDeviceIndex()
{
    return ActiveDeviceIndex;
}

void Audio::SetActiveDeviceIndex(int32 index)
{
    index = Math::Clamp(index, -1, Devices.Count() - 1);
    if (ActiveDeviceIndex == index)
        return;

    ActiveDeviceIndex = index;

    AudioBackend::OnActiveDeviceChanged();

    ActiveDeviceChanged();
}

float Audio::GetMasterVolume()
{
    return MasterVolume;
}

void Audio::SetMasterVolume(float value)
{
    MasterVolume = Math::Saturate(value);
}

float Audio::GetVolume()
{
    return Volume;
}

void Audio::SetDopplerFactor(float value)
{
    value = Math::Max(0.0f, value);
    AudioBackend::SetDopplerFactor(value);
}

void Audio::OnAddListener(AudioListener* listener)
{
    ASSERT(!Listeners.Contains(listener));

    if (Listeners.Count() >= AUDIO_MAX_LISTENERS)
    {
        LOG(Error, "Unsupported amount of the audio listeners!");
        return;
    }

    Listeners.Add(listener);
    AudioBackend::Listener::OnAdd(listener);
}

void Audio::OnRemoveListener(AudioListener* listener)
{
    if (!Listeners.Remove(listener))
    {
        AudioBackend::Listener::OnRemove(listener);
    }
}

void Audio::OnAddSource(AudioSource* source)
{
    ASSERT(!Sources.Contains(source));

    Sources.Add(source);
    AudioBackend::Source::OnAdd(source);
}

void Audio::OnRemoveSource(AudioSource* source)
{
    if (!Sources.Remove(source))
    {
        AudioBackend::Source::OnRemove(source);
    }
}

bool AudioService::Init()
{
    const auto settings = AudioSettings::Get();
    const bool mute = CommandLine::Options.Mute.IsTrue() || settings->DisableAudio;

    // Pick a backend to use
    AudioBackend* backend = nullptr;
#if AUDIO_API_NONE
    if (mute)
    {
        backend = New<AudioBackendNone>();
    }
#endif
#if AUDIO_API_PS4
    if (!backend)
    {
        backend = New<AudioBackendPS4>();
    }
#endif
#if AUDIO_API_OPENAL
    if (!backend)
    {
        backend = New<AudioBackendOAL>();
    }
#endif
#if AUDIO_API_XAUDIO2
	if (!backend)
	{
		backend = New<AudioBackendXAudio2>();
	}
#endif
#if AUDIO_API_NONE
    if (!backend)
    {
        backend = New<AudioBackendNone>();
    }
#else
    if (mute)
    {
        LOG(Warning, "Cannot use mute audio. Null Audio Backend not available on this platform.");
    }
#endif
    if (backend == nullptr)
    {
        LOG(Error, "Failed to create audio backend.");
        return true;
    }
    AudioBackend::Instance = backend;

    LOG(Info, "Audio system initialization... (backend: {0})", AudioBackend::Name());

    if (AudioBackend::Init())
    {
        LOG(Warning, "Failed to initialize audio backend.");
    }

    Engine::Pause.Bind(&OnEnginePause);
    Engine::Unpause.Bind(&OnEngineUnpause);

    return false;
}

void AudioService::Update()
{
    PROFILE_CPU();

    // Update the master volume
    float masterVolume = MasterVolume;
    if (MuteOnFocusLoss && !Engine::HasFocus)
    {
        // Mute audio if app has no user focus
        masterVolume = 0.0f;
    }
    if (Volume != masterVolume)
    {
        Volume = masterVolume;

        AudioBackend::SetVolume(masterVolume);
    }

    AudioBackend::Update();
}

void AudioService::Dispose()
{
    ASSERT(Audio::Sources.IsEmpty() && Audio::Listeners.IsEmpty());

    // Cleanup
    Audio::Devices.Resize(0);
    if (AudioBackend::Instance)
    {
        AudioBackend::Dispose();
        Delete(AudioBackend::Instance);
        AudioBackend::Instance = nullptr;
    }
    ActiveDeviceIndex = -1;
}
