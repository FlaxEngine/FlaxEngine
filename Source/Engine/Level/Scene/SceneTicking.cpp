// Copyright (c) Wojciech Figat. All rights reserved.

#include "SceneTicking.h"
#include "Scene.h"
#include "Engine/Scripting/Script.h"

SceneTicking::TickData::TickData(int32 capacity)
    : Scripts(capacity)
    , Ticks(capacity)
{
}

void SceneTicking::TickData::AddScript(Script* script)
{
    Scripts.Add(script);
#if USE_EDITOR
    if (script->_executeInEditor)
        ScriptsExecuteInEditor.Add(script);
#endif
}

void SceneTicking::TickData::RemoveScript(Script* script)
{
    Scripts.Remove(script);
#if USE_EDITOR
    if (script->_executeInEditor)
        ScriptsExecuteInEditor.Remove(script);
#endif
}

void SceneTicking::TickData::RemoveTick(void* callee)
{
    for (int32 i = 0; i < Ticks.Count(); i++)
    {
        if (Ticks.Get()[i].Callee == callee)
        {
            Ticks.RemoveAt(i);
            break;
        }
    }
}

void SceneTicking::TickData::Tick()
{
    TickScripts(Scripts);

    for (int32 i = 0; i < Ticks.Count() && _canTick; i++)
        Ticks.Get()[i].Call();
}

#if USE_EDITOR

void SceneTicking::TickData::RemoveTickExecuteInEditor(void* callee)
{
    for (int32 i = 0; i < TicksExecuteInEditor.Count(); i++)
    {
        if (TicksExecuteInEditor.Get()[i].Callee == callee)
        {
            TicksExecuteInEditor.RemoveAt(i);
            break;
        }
    }
}

void SceneTicking::TickData::TickExecuteInEditor()
{
    TickScripts(ScriptsExecuteInEditor);

    for (int32 i = 0; i < TicksExecuteInEditor.Count() && _canTick; i++)
        TicksExecuteInEditor.Get()[i].Call();
}

#endif

void SceneTicking::TickData::Clear()
{
    Scripts.Clear();
    Ticks.Clear();
#if USE_EDITOR
    ScriptsExecuteInEditor.Clear();
    TicksExecuteInEditor.Clear();
#endif
}

SceneTicking::FixedUpdateTickData::FixedUpdateTickData()
    : TickData(512)
{
}

void SceneTicking::FixedUpdateTickData::TickScripts(const Array<Script*>& scripts)
{
    for (int32 i = 0; i < scripts.Count() && _canTick; i++)
        scripts.Get()[i]->OnFixedUpdate();
}

SceneTicking::UpdateTickData::UpdateTickData()
    : TickData(1024)
{
}

void SceneTicking::UpdateTickData::TickScripts(const Array<Script*>& scripts)
{
    for (int32 i = 0; i < scripts.Count() && _canTick; i++)
        scripts.Get()[i]->OnUpdate();
}

SceneTicking::LateUpdateTickData::LateUpdateTickData()
    : TickData(0)
{
}

void SceneTicking::LateUpdateTickData::TickScripts(const Array<Script*>& scripts)
{
    for (int32 i = 0; i < scripts.Count() && _canTick; i++)
        scripts.Get()[i]->OnLateUpdate();
}

SceneTicking::LateFixedUpdateTickData::LateFixedUpdateTickData()
    : TickData(0)
{
}

void SceneTicking::LateFixedUpdateTickData::TickScripts(const Array<Script*>& scripts)
{
    for (int32 i = 0; i < scripts.Count() && _canTick; i++)
        scripts.Get()[i]->OnLateFixedUpdate();
}

void SceneTicking::AddScript(Script* obj)
{
    ASSERT_LOW_LAYER(obj && obj->GetParent() && obj->GetParent()->GetScene());
    if (obj->_tickFixedUpdate)
        FixedUpdate.AddScript(obj);
    if (obj->_tickUpdate)
        Update.AddScript(obj);
    if (obj->_tickLateUpdate)
        LateUpdate.AddScript(obj);
    if (obj->_tickLateFixedUpdate)
        LateFixedUpdate.AddScript(obj);
}

void SceneTicking::RemoveScript(Script* obj)
{
    ASSERT_LOW_LAYER(obj && obj->GetParent() && obj->GetParent()->GetScene());
    if (obj->_tickFixedUpdate)
        FixedUpdate.RemoveScript(obj);
    if (obj->_tickUpdate)
        Update.RemoveScript(obj);
    if (obj->_tickLateUpdate)
        LateUpdate.RemoveScript(obj);
    if (obj->_tickLateFixedUpdate)
        LateFixedUpdate.RemoveScript(obj);
}

void SceneTicking::Clear()
{
    FixedUpdate.Clear();
    Update.Clear();
    LateUpdate.Clear();
    LateFixedUpdate.Clear();
}

void SceneTicking::SetTicking(bool enable)
{
    FixedUpdate._canTick = enable;
    Update._canTick = enable;
    LateUpdate._canTick = enable;
    LateFixedUpdate._canTick = enable;
}
