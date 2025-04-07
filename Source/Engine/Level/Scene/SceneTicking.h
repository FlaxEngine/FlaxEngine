// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Types.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Scene gameplay updating helper subsystem that boosts the level ticking by providing efficient objects cache.
/// </summary>
class FLAXENGINE_API SceneTicking
{
public:
    /// <summary>
    /// Tick function type.
    /// </summary>
    struct FLAXENGINE_API Tick
    {
        typedef void (*Signature)();
        typedef void (*SignatureObj)(void*);

        template<class T, void(T::*Method)()>
        static void MethodCaller(void* callee)
        {
            (static_cast<T*>(callee)->*Method)();
        }

        void* Callee;
        SignatureObj FunctionObj;

        template<class T, void(T::*Method)()>
        void Bind(T* callee)
        {
            Callee = callee;
            FunctionObj = &MethodCaller<T, Method>;
        }

        FORCE_INLINE void Call() const
        {
            (*FunctionObj)(Callee);
        }
    };

    /// <summary>
    /// Ticking data container.
    /// </summary>
    class FLAXENGINE_API TickData
    {
    public:
        Array<Script*> Scripts;
        Array<Tick> Ticks;
#if USE_EDITOR
        Array<Script*> ScriptsExecuteInEditor;
        Array<Tick> TicksExecuteInEditor;
#endif

        TickData(int32 capacity);

        virtual void TickScripts(const Array<Script*>& scripts) = 0;

        void AddScript(Script* script);
        void RemoveScript(Script* script);

        template<class T, void(T::*Method)()>
        void AddTick(T* callee)
        {
            SceneTicking::Tick tick;
            tick.Bind<T, Method>(callee);
            Ticks.Add(tick);
        }

        void RemoveTick(void* callee);
        void Tick();

#if USE_EDITOR
        template<class T, void(T::*Method)()>
        void AddTickExecuteInEditor(T* callee)
        {
            SceneTicking::Tick tick;
            tick.Bind<T, Method>(callee);
            TicksExecuteInEditor.Add(tick);
        }

        void RemoveTickExecuteInEditor(void* callee);
        void TickExecuteInEditor();
#endif

        void Clear();
    };

    class FLAXENGINE_API FixedUpdateTickData : public TickData
    {
    public:
        FixedUpdateTickData();
        void TickScripts(const Array<Script*>& scripts) override;
    };

    class FLAXENGINE_API UpdateTickData : public TickData
    {
    public:
        UpdateTickData();
        void TickScripts(const Array<Script*>& scripts) override;
    };

    class FLAXENGINE_API LateUpdateTickData : public TickData
    {
    public:
        LateUpdateTickData();
        void TickScripts(const Array<Script*>& scripts) override;
    };

    class FLAXENGINE_API LateFixedUpdateTickData : public TickData
    {
    public:
        LateFixedUpdateTickData();
        void TickScripts(const Array<Script*>& scripts) override;
    };

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

    /// <summary>
    /// The late fixed update tick function.
    /// </summary>
    LateFixedUpdateTickData LateFixedUpdate;
};
