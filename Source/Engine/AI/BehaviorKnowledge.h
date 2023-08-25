// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Collections/BitArray.h"
#include "Engine/Scripting/ScriptingObject.h"

class Behavior;
class BehaviorTree;
enum class BehaviorValueComparison;

/// <summary>
/// Behavior logic component knowledge data container. Contains blackboard values, sensors data and goals storage for Behavior Tree execution.
/// </summary>
API_CLASS() class FLAXENGINE_API BehaviorKnowledge : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorKnowledge, ScriptingObject);
    ~BehaviorKnowledge();

    /// <summary>
    /// Owning Behavior instance (constant).
    /// </summary>
    API_FIELD(ReadOnly) Behavior* Behavior = nullptr;

    /// <summary>
    /// Used Behavior Tree asset (defines blackboard and memory constraints).
    /// </summary>
    API_FIELD(ReadOnly) BehaviorTree* Tree = nullptr;

    /// <summary>
    /// Raw memory chunk with all Behavior Tree nodes state.
    /// </summary>
    void* Memory = nullptr;

    /// <summary>
    /// Array with per-node bit indicating whether node is relevant (active in graph with state created).
    /// </summary>
    BitArray<> RelevantNodes;

    /// <summary>
    /// Instance of the behaviour blackboard (structure or class).
    /// </summary>
    API_FIELD() Variant Blackboard;

    /// <summary>
    /// Initializes the knowledge for a certain tree.
    /// </summary>
    void InitMemory(BehaviorTree* tree);

    /// <summary>
    /// Releases the memory of the knowledge.
    /// </summary>
    void FreeMemory();

    /// <summary>
    /// Gets the knowledge item value via selector path.
    /// </summary>
    /// <seealso cref="BehaviorKnowledgeSelector{T}"/>
    /// <param name="path">Selector path.</param>
    /// <param name="value">Result value (valid only when returned true).</param>
    /// <returns>True if got value, otherwise false.</returns>
    API_FUNCTION() bool Get(const StringAnsiView& path, API_PARAM(Out) Variant& value);

    /// <summary>
    /// Sets the knowledge item value via selector path.
    /// </summary>
    /// <seealso cref="BehaviorKnowledgeSelector{T}"/>
    /// <param name="path">Selector path.</param>
    /// <param name="value">Value to set.</param>
    /// <returns>True if set value, otherwise false.</returns>
    API_FUNCTION() bool Set(const StringAnsiView& path, const Variant& value);

    /// <summary>
    /// Compares two values and returns the comparision result.
    /// </summary>
    /// <param name="a">The left operand.</param>
    /// <param name="b">The right operand.</param>
    /// <param name="comparison">The comparison function.</param>
    /// <returns>True if comparision passed, otherwise false.</returns>
    API_FUNCTION() static bool CompareValues(float a, float b, BehaviorValueComparison comparison);
};
