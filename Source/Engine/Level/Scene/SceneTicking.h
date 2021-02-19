// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Types.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Scene gameplay updating helper subsystem that boosts the level ticking by providing efficient objects cache.
/// </summary>
class SceneTicking
{
    friend Scene;

public:

    /// <summary>
    /// Tick function type.
    /// </summary>
    struct Tick
    {
        typedef void (*Signature)();
        typedef void (*SignatureObj)(void*);

        template<class T, void(T::*Method)()>
        static void MethodCaller(void* callee)
        {
            (static_cast<T*>(callee)->*Method)();
        }

    public:

        void* Callee;
        SignatureObj FunctionObj;

    public:

        template<class T, void(T::*Method)()>
        void Bind(T* callee)
        {
            Callee = callee;
            FunctionObj = &MethodCaller<T, Method>;
        }

        /// <summary>
        /// Calls the binded function.
        /// </summary>
        FORCE_INLINE void Call() const
        {
            (*FunctionObj)(Callee);
        }
    };

    class TickData
    {
    public:

        Array<Script*> Scripts;
        Array<Tick> Ticks;
#if USE_EDITOR
        Array<Script*> ScriptsExecuteInEditor;
        Array<Tick> TicksExecuteInEditor;
#endif

        TickData(int32 capacity)
            : Scripts(capacity)
            , Ticks(capacity)
        {
        }

        virtual void TickScripts(const Array<Script*>& scripts) = 0;

        void Tick()
        {
            TickScripts(Scripts);

            for (int32 i = 0; i < Ticks.Count(); i++)
                Ticks[i].Call();
        }

#if USE_EDITOR
        void TickExecuteInEditor()
        {
            TickScripts(ScriptsExecuteInEditor);

            for (int32 i = 0; i < TicksExecuteInEditor.Count(); i++)
                TicksExecuteInEditor[i].Call();
        }
#endif

        void AddScript(Script* script);
        void RemoveScript(Script* script);

        template<class T, void(T::*Method)()>
        void AddTick(T* callee)
        {
            SceneTicking::Tick tick;
            tick.Bind<T, Method>(callee);
            Ticks.Add(tick);
        }

        void RemoveTick(void* callee)
        {
            for (int32 i = 0; i < Ticks.Count(); i++)
            {
                if (Ticks[i].Callee == callee)
                {
                    Ticks.RemoveAt(i);
                    break;
                }
            }
        }

#if USE_EDITOR
        template<class T, void(T::*Method)()>
        void AddTickExecuteInEditor(T* callee)
        {
            SceneTicking::Tick tick;
            tick.Bind<T, Method>(callee);
            TicksExecuteInEditor.Add(tick);
        }

        void RemoveTickExecuteInEditor(void* callee)
        {
            for (int32 i = 0; i < TicksExecuteInEditor.Count(); i++)
            {
                if (TicksExecuteInEditor[i].Callee == callee)
                {
                    TicksExecuteInEditor.RemoveAt(i);
                    break;
                }
            }
        }
#endif

        void Clear()
        {
            Scripts.Clear();
            Ticks.Clear();
#if USE_EDITOR
            ScriptsExecuteInEditor.Clear();
            TicksExecuteInEditor.Clear();
#endif
        }
    };

    class FixedUpdateTickData : public TickData
    {
    public:

        FixedUpdateTickData()
            : TickData(512)
        {
        }

        void TickScripts(const Array<Script*>& scripts) override;
    };

    class UpdateTickData : public TickData
    {
    public:

        UpdateTickData()
            : TickData(1024)
        {
        }

        void TickScripts(const Array<Script*>& scripts) override;
    };

    class LateUpdateTickData : public TickData
    {
    public:

        LateUpdateTickData()
            : TickData(64)
        {
        }

        void TickScripts(const Array<Script*>& scripts) override;
    };

private:

    Scene* Scene;

    explicit SceneTicking(::Scene* scene);

public:

    /// <summary>
    /// Adds the script to scene ticking system.
    /// </summary>
    /// <param name="obj">The object.</param>
    void AddScript(Script* obj);

    /// <summary>
    /// Removes the script from scene ticking system.
    /// </summary>
    /// <param name="obj">The object.</param>
    void RemoveScript(Script* obj);

    /// <summary>
    /// Clears this instance data.
    /// </summary>
    void Clear();

public:

    /// <summary>
    /// The fixed update tick function.
    /// </summary>
    FixedUpdateTickData FixedUpdate;

    /// <summary>
    /// The update tick function.
    /// </summary>
    UpdateTickData Update;

    /// <summary>
    /// The late update tick function.
    /// </summary>
    LateUpdateTickData LateUpdate;
};
