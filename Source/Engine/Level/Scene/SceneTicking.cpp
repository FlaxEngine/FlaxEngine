// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "SceneTicking.h"
#include "Scene.h"
#include "Engine/Scripting/Script.h"

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

void SceneTicking::FixedUpdateTickData::TickScripts(const Array<Script*>& scripts)
{
    for (auto* script : scripts)
    {
        script->OnFixedUpdate();
    }
}

void SceneTicking::UpdateTickData::TickScripts(const Array<Script*>& scripts)
{
    for (auto* script : scripts)
    {
        script->OnUpdate();
    }
}

void SceneTicking::LateUpdateTickData::TickScripts(const Array<Script*>& scripts)
{
    for (auto* script : scripts)
    {
        script->OnLateUpdate();
    }
}

SceneTicking::SceneTicking(::Scene* scene)
    : Scene(scene)
{
}

void SceneTicking::AddScript(Script* obj)
{
    ASSERT(obj && obj->GetParent() && obj->GetParent()->GetScene() == Scene);

    if (obj->_tickFixedUpdate)
        FixedUpdate.AddScript(obj);
    if (obj->_tickUpdate)
        Update.AddScript(obj);
    if (obj->_tickLateUpdate)
        LateUpdate.AddScript(obj);
}

void SceneTicking::RemoveScript(Script* obj)
{
    ASSERT(obj && obj->GetParent() && obj->GetParent()->GetScene() == Scene);

    if (obj->_tickFixedUpdate)
        FixedUpdate.RemoveScript(obj);
    if (obj->_tickUpdate)
        Update.RemoveScript(obj);
    if (obj->_tickLateUpdate)
        LateUpdate.RemoveScript(obj);
}

void SceneTicking::Clear()
{
    FixedUpdate.Clear();
    Update.Clear();
    LateUpdate.Clear();
}
