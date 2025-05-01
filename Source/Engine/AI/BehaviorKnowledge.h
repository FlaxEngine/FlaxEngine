// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Collections/Array.h"
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
    /// List of all active goals of the behaviour (structure or class).
    /// </summary>
    API_FIELD() Array<Variant> Goals;

public:
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
    API_FUNCTION() bool Get(const StringAnsiView& path, API_PARAM(Out) Variant& value) const;

    /// <summary>
    /// Sets the knowledge item value via selector path.
    /// </summary>
    /// <seealso cref="BehaviorKnowledgeSelector{T}"/>
    /// <param name="path">Selector path.</param>
    /// <param name="value">Value to set.</param>
    /// <returns>True if set value, otherwise false.</returns>
    API_FUNCTION() bool Set(const StringAnsiView& path, const Variant& value);

public:
    /// <summary>
    /// Checks if knowledge has a given goal (exact type match without base class check).
    /// </summary>
    /// <param name="type">The goal type.</param>
    /// <returns>True if knowledge has a given goal, otherwise false.</returns>
    API_FUNCTION() bool HasGoal(ScriptingTypeHandle type) const;

    /// <summary>
    /// Checks if knowledge has a given goal (exact type match without base class check).
    /// </summary>
    /// <returns>True if knowledge has a given goal, otherwise false.</returns>
    template<typename T>
    FORCE_INLINE bool HasGoal()
    {
        return HasGoal(T::TypeInitializer);
    }

    /// <summary>
    /// Gets the goal from the knowledge.
    /// </summary>
    /// <param name="type">The goal type.</param>
    /// <returns>The goal value or null if not found.</returns>
    API_FUNCTION() const Variant& GetGoal(ScriptingTypeHandle type) const;

    /// <summary>
    /// Adds the goal to the knowledge. If goal of that type already exists then it's value is updated.
    /// </summary>
    /// <param name="goal">The goal value to add/set.</param>
    API_FUNCTION() void AddGoal(Variant&& goal);

    /// <summary>
    /// Removes the goal from the knowledge. Does nothing if goal of the given type doesn't exist in the knowledge.
    /// </summary>
    /// <param name="type">The goal type.</param>
    API_FUNCTION() void RemoveGoal(ScriptingTypeHandle type);

    /// <summary>
    /// Removes the goal from the knowledge. Does nothing if goal of the given type doesn't exist in the knowledge.
    /// </summary>
    template<typename T>
    FORCE_INLINE void RemoveGoal()
    {
        RemoveGoal(T::TypeInitializer);
    }

public:
    /// <summary>
    /// Compares two values and returns the comparision result.
    /// </summary>
    /// <param name="a">The left operand.</param>
    /// <param name="b">The right operand.</param>
    /// <param name="comparison">The comparison function.</param>
    /// <returns>True if comparision passed, otherwise false.</returns>
    API_FUNCTION() static bool CompareValues(float a, float b, BehaviorValueComparison comparison);
};
