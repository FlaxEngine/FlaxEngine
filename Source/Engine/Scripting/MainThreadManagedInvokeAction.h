// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Threading/MainThreadTask.h"
#include "Engine/Core/Log.h"
#include "ManagedCLR/MMethod.h"
#include "ManagedCLR/MUtils.h"

/// <summary>
/// Helper class for easy invoking managed code on main thread before all game systems update.
/// </summary>
/// <seealso cref="MainThreadTask" />
class FLAXENGINE_API MainThreadManagedInvokeAction : public MainThreadTask
{
public:

    struct ParamsBuilder
    {
    public:

        int32 Count;
        int32 IsRef[8];
        int32 Offsets[8];
        Array<byte> Data;

    public:

        /// <summary>
        /// Initializes a new instance of the <see cref="ParamsBuilder"/> struct.
        /// </summary>
        ParamsBuilder()
            : Count(0)
            , Data(4 * sizeof(int64)) // set initial capacity to reduce allocations
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ParamsBuilder"/> struct.
        /// </summary>
        /// <param name="capacity">The initial capacity for parameters data storage (in bytes).</param>
        ParamsBuilder(int32 capacity)
            : Count(0)
            , Data(capacity)
        {
        }

    public:

        template<typename T>
        void AddParam(const T& value)
        {
            ASSERT(Count < 8);
            IsRef[Count] = false;
            Offsets[Count++] = Data.Count();
            Data.Add((byte*)&value, sizeof(T));
        }

        template<typename T>
        void AddParam(T* value)
        {
            ASSERT(Count < 8);
            IsRef[Count] = true;
            Offsets[Count++] = Data.Count();
            Data.Add((byte*)&value, sizeof(void*));
        }

        FORCE_INLINE void AddParam(const bool& value)
        {
            int32 val = value ? 1 : 0;
            AddParam(val);
        }

        FORCE_INLINE void AddParam(const String& value)
        {
            MonoString* val = MUtils::ToString(value);
            AddParam(val);
        }

        FORCE_INLINE void AddParam(const StringView& value)
        {
            MonoString* val = MUtils::ToString(value);
            AddParam(val);
        }

        FORCE_INLINE void AddParam(const String& value, MonoDomain* domain)
        {
            MonoString* val = MUtils::ToString(value, domain);
            AddParam(val);
        }

        FORCE_INLINE void AddParam(const StringView& value, MonoDomain* domain)
        {
            MonoString* val = MUtils::ToString(value, domain);
            AddParam(val);
        }

        void GetParams(void* params[8])
        {
            for (int32 i = 0; i < Count; i++)
            {
                if (IsRef[i])
                    Platform::MemoryCopy(&params[i], &Data[Offsets[i]], sizeof(void*));
                else
                    params[i] = &Data[Offsets[i]];
            }
        }
    };

private:

    MonoObject* _instance;
    MMethod* _method;
    void* _methodThunk;
    LogType _exceptionLevel;
    ParamsBuilder _params;

public:

    MainThreadManagedInvokeAction(MonoObject* instance, MMethod* method, LogType exceptionLevel)
        : _instance(instance)
        , _method(method)
        , _methodThunk(nullptr)
        , _exceptionLevel(exceptionLevel)
        , _params(0)
    {
    }

    MainThreadManagedInvokeAction(MonoObject* instance, MMethod* method, LogType exceptionLevel, const ParamsBuilder& params)
        : _instance(instance)
        , _method(method)
        , _methodThunk(nullptr)
        , _exceptionLevel(exceptionLevel)
        , _params(params)
    {
    }

    MainThreadManagedInvokeAction(void* methodThunk, LogType exceptionLevel, const ParamsBuilder& params)
        : _instance(nullptr)
        , _method(nullptr)
        , _methodThunk(methodThunk)
        , _exceptionLevel(exceptionLevel)
        , _params(params)
    {
    }

public:

    /// <summary>
    /// Starts the new task or invokes this action now if already running on a main thread.
    /// </summary>
    /// <param name="method">The managed method.</param>
    /// <param name="instance">The managed instance object.</param>
    /// <param name="exceptionLevel">The exception logging error level.</param>
    /// <returns>The created task.</returns>
    static MainThreadManagedInvokeAction* Invoke(MMethod* method, MonoObject* instance = nullptr, LogType exceptionLevel = LogType::Error);

    /// <summary>
    /// Starts the new task or invokes this action now if already running on a main thread.
    /// </summary>
    /// <param name="method">The managed method.</param>
    /// <param name="params">The method parameters.</param>
    /// <param name="instance">The managed instance object.</param>
    /// <param name="exceptionLevel">The exception logging error level.</param>
    /// <returns>The created task.</returns>
    static MainThreadManagedInvokeAction* Invoke(MMethod* method, ParamsBuilder& params, MonoObject* instance = nullptr, LogType exceptionLevel = LogType::Error);

    /// <summary>
    /// Starts the new task or invokes this action now if already running on a main thread.
    /// </summary>
    /// <param name="methodThunk">The managed method thunk (must return void and has up to 4 parameters).</param>
    /// <param name="params">The method parameters.</param>
    /// <param name="exceptionLevel">The exception logging error level.</param>
    /// <returns>The created task.</returns>
    static MainThreadManagedInvokeAction* Invoke(void* methodThunk, ParamsBuilder& params, LogType exceptionLevel = LogType::Error);

    /// <summary>
    /// Invokes action now.
    /// </summary>
    /// <param name="method">The managed method.</param>
    /// <param name="params">The method parameters.</param>
    /// <param name="instance">The managed instance object.</param>
    /// <param name="exceptionLevel">The exception logging error level.</param>
    static void InvokeNow(MMethod* method, ParamsBuilder& params, MonoObject* instance = nullptr, LogType exceptionLevel = LogType::Error);

protected:

    // [MainThreadTask]
    bool Run() override;
};
