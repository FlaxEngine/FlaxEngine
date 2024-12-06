// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "../BinaryAsset.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Visject/VisjectGraph.h"
#if COMPILE_WITH_PROFILER
#include "Engine/Profiler/ProfilerSrcLoc.h"
#endif

#define VISUAL_SCRIPT_GRAPH_MAX_CALL_STACK 250
#define VISUAL_SCRIPT_DEBUGGING USE_EDITOR

#define VisualScriptGraphNode VisjectGraphNode<>

class VisualScripting;
class VisualScriptingBinaryModule;

/// <summary>
/// The Visual Script graph data.
/// </summary>
class VisualScriptGraph : public VisjectGraph<VisualScriptGraphNode, VisjectGraphBox, VisjectGraphParameter>
{
public:
    bool onNodeLoaded(Node* n) override;
};

/// <summary>
/// The Visual Script graph executor runtime.
/// </summary>
class VisualScriptExecutor : public VisjectExecutor
{
    friend VisualScripting;
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="VisualScriptExecutor"/> class.
    /// </summary>
    VisualScriptExecutor();

    void Invoke(const Guid& scriptId, int32 nodeId, int32 boxId, const Guid& instanceId, Variant& result) const;

private:
    void OnError(Node* node, Box* box, const StringView& message) override;
    Value eatBox(Node* caller, Box* box) override;
    Graph* GetCurrentGraph() const override;

    void ProcessGroupParameters(Box* box, Node* node, Value& value);
    void ProcessGroupTools(Box* box, Node* node, Value& value);
    void ProcessGroupFunction(Box* boxBase, Node* node, Value& value);
    void ProcessGroupFlow(Box* boxBase, Node* node, Value& value);
};

/// <summary>
/// The Visual Script asset. Contains a graph with functions and parameters for visual scripting.
/// </summary>
/// <seealso cref="BinaryAsset" />
API_CLASS(NoSpawn, Sealed) class FLAXENGINE_API VisualScript : public BinaryAsset
{
    DECLARE_BINARY_ASSET_HEADER(VisualScript, 1);
    friend VisualScripting;
    friend VisualScriptExecutor;
    friend VisualScriptingBinaryModule;
public:
    /// <summary>
    /// Visual Script flag types.
    /// </summary>
    API_ENUM(Attributes="Flags") enum class Flags
    {
        /// <summary>
        /// No flags.
        /// </summary>
        None = 0,

        /// <summary>
        /// Script is abstract and cannot be instantiated directly.
        /// </summary>
        Abstract = 1,

        /// <summary>
        /// Script is sealed and cannot be inherited by other scripts.
        /// </summary>
        Sealed = 2,
    };

    /// <summary>
    /// Visual Script metadata container.
    /// </summary>
    API_STRUCT() struct Metadata
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(Metadata);

        /// <summary>
        /// The base class typename.
        /// </summary>
        API_FIELD() String BaseTypename;

        /// <summary>
        /// The script flags.
        /// </summary>
        API_FIELD() Flags Flags;
    };

    enum class MethodFlags
    {
        None = 0,
        Static = 1,
        Virtual = 2,
        Override = 4,
    };

    struct Method
    {
        VisualScript* Script;
        VisualScriptGraphNode* Node;
        StringAnsi Name;
        MethodFlags MethodFlags;
        ScriptingTypeMethodSignature Signature;
        Array<StringAnsi, InlinedAllocation<16>> ParamNames;
#if COMPILE_WITH_PROFILER
        StringAnsi ProfilerName;
        SourceLocationData ProfilerData;
#endif
    };

    struct Field
    {
        VisualScript* Script;
        VisjectGraphParameter* Parameter;
        int32 Index;
        StringAnsi Name;
    };

    struct EventBinding
    {
        ScriptingTypeHandle Type;
        String Name;
        Array<Method*, InlinedAllocation<4>> BindedMethods;
    };

    struct Instance
    {
        Array<Variant> Params;
        Array<EventBinding> EventBindings;
    };

private:
    Dictionary<Guid, Instance> _instances;
    ScriptingTypeHandle _scriptingTypeHandle;
    ScriptingTypeHandle _scriptingTypeHandleCached;
    StringAnsiView _typename;
    char _typenameChars[33];
    Array<Method, InlinedAllocation<32>> _methods;
    Array<Field, InlinedAllocation<32>> _fields;
#if USE_EDITOR
    Array<Guid> _oldParamsLayout;
    Array<Variant> _oldParamsValues;
#endif

public:
    /// <summary>
    /// The Visual Script graph.
    /// </summary>
    VisualScriptGraph Graph;

    /// <summary>
    /// The script metadata.
    /// </summary>
    API_FIELD(ReadOnly) Metadata Meta;

public:
    /// <summary>
    /// Gets the typename of the Visual Script. Identifies it's scripting type.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const StringAnsiView& GetScriptTypeName() const
    {
        return _typename;
    }

    /// <summary>
    /// Gets the list of Visual Script parameters declared in this graph (excluding base types).
    /// </summary>
    API_PROPERTY() const Array<VisjectGraphParameter>& GetParameters() const
    {
        return Graph.Parameters;
    }

    /// <summary>
    /// Gets the scripting type handle of this Visual Script.
    /// </summary>
    ScriptingTypeHandle GetScriptingType();

    /// <summary>
    /// Creates a new instance of the Visual Script object.
    /// </summary>
    /// <returns>The created instance or null if failed.</returns>
    API_FUNCTION() ScriptingObject* CreateInstance();

    /// <summary>
    /// Gets the Visual Script instance data.
    /// </summary>
    /// <param name="instance">The object instance.</param>
    /// <returns>The data or invalid instance (not VS or missing).</returns>
    Instance* GetScriptInstance(ScriptingObject* instance) const;

    /// <summary>
    /// Gets the value of the Visual Script parameter of the given instance.
    /// </summary>
    /// <param name="name">The parameter name.</param>
    /// <param name="instance">The object instance.</param>
    /// <returns>The property value.</returns>
    API_FUNCTION() const Variant& GetScriptInstanceParameterValue(const StringView& name, ScriptingObject* instance) const;

    /// <summary>
    /// Sets the value of the Visual Script parameter of the given instance.
    /// </summary>
    /// <param name="name">The parameter name.</param>
    /// <param name="instance">The object instance.</param>
    /// <param name="value">The property value to set.</param>
    API_FUNCTION() void SetScriptInstanceParameterValue(const StringView& name, ScriptingObject* instance, const Variant& value) const;

    /// <summary>
    /// Sets the value of the Visual Script parameter of the given instance.
    /// </summary>
    /// <param name="name">The parameter name.</param>
    /// <param name="instance">The object instance.</param>
    /// <param name="value">The property value to set.</param>
    void SetScriptInstanceParameterValue(const StringView& name, ScriptingObject* instance, Variant&& value) const;

    /// <summary>
    /// Tries to find the method matching the given properties. Doesn't check base classes but just this script.
    /// </summary>
    /// <param name="name">The method name.</param>
    /// <param name="numParams">The method parameters count.</param>
    /// <returns>The method graph node entry for callback, null if not found.</returns>
    const Method* FindMethod(const StringAnsiView& name, int32 numParams = 0) const;

    /// <summary>
    /// Tries to find the field matching the given name. Doesn't check base classes but just this script.
    /// </summary>
    /// <param name="name">The field name.</param>
    /// <returns>The field entry for access, null if not found.</returns>
    const Field* FindField(const StringAnsiView& name) const;

    /// <summary>
    /// Tries to load surface graph from the asset.
    /// </summary>
    /// <returns>The surface data or empty if failed to load it.</returns>
    API_FUNCTION() BytesContainer LoadSurface();

#if USE_EDITOR

    /// <summary>
    /// Updates the graph surface (save new one, discard cached data, reload asset).
    /// </summary>
    /// <param name="data">Stream with graph data.</param>
    /// <param name="meta">Script metadata.</param>
    /// <returns>True if cannot save it, otherwise false.</returns>
    API_FUNCTION() bool SaveSurface(const BytesContainer& data, API_PARAM(Ref) const Metadata& meta);

    // Returns the amount of methods in the script.
    API_FUNCTION() int32 GetMethodsCount() const
    {
        return _methods.Count();
    }

    // Gets the signature data of the method.
    API_FUNCTION() void GetMethodSignature(int32 index, API_PARAM(Out) String& name, API_PARAM(Out) byte& flags, API_PARAM(Out) String& returnTypeName, API_PARAM(Out) Array<String>& paramNames, API_PARAM(Out) Array<String>& paramTypeNames, API_PARAM(Out) Array<bool>& paramOuts);

    // Invokes the method.
    API_FUNCTION() Variant InvokeMethod(int32 index, const Variant& instance, Span<Variant> parameters) const;

    // Gets the metadata of the script surface.
    API_FUNCTION() Span<byte> GetMetaData(int32 typeID);

    // Gets the metadata of the method.
    API_FUNCTION() Span<byte> GetMethodMetaData(int32 index, int32 typeID);

#endif

public:
    // [BinaryAsset]
#if USE_EDITOR
    void GetReferences(Array<Guid>& assets, Array<String>& files) const override
    {
        BinaryAsset::GetReferences(assets, files);
        Graph.GetReferences(assets);
    }
#endif

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;

private:
    void CacheScriptingType();
};

/// <summary>
/// The visual scripts module for engine scripting integration.
/// </summary>
/// <seealso cref="BinaryModule" />
class FLAXENGINE_API VisualScriptingBinaryModule : public BinaryModule
{
    friend VisualScript;
private:
    StringAnsi _name;
    Array<char*> _unloadedScriptTypeNames;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="VisualScriptingBinaryModule"/> class.
    /// </summary>
    VisualScriptingBinaryModule();

public:
    /// <summary>
    /// The visual script assets loaded into the module with exposes scripting types. Order matches the Types array.
    /// </summary>
    Array<AssetReference<VisualScript>> Scripts;

private:
    static ScriptingObject* VisualScriptObjectSpawn(const ScriptingObjectSpawnParams& params);
#if USE_EDITOR
    void OnScriptsReloading();
#endif
    static void OnEvent(ScriptingObject* object, Span<Variant> parameters, ScriptingTypeHandle eventType, StringView eventName);

public:
    // [BinaryModule]
    const StringAnsi& GetName() const override;
    bool IsLoaded() const override;
    bool FindScriptingType(const StringAnsiView& typeName, int32& typeIndex) override;
    void* FindMethod(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name, int32 numParams = 0) override;
    bool InvokeMethod(void* method, const Variant& instance, Span<Variant> paramValues, Variant& result) override;
    void GetMethodSignature(void* method, ScriptingTypeMethodSignature& methodSignature) override;
    void* FindField(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name) override;
    void GetFieldSignature(void* field, ScriptingTypeFieldSignature& fieldSignature) override;
    bool GetFieldValue(void* field, const Variant& instance, Variant& result) override;
    bool SetFieldValue(void* field, const Variant& instance, Variant& value) override;
    void SerializeObject(JsonWriter& stream, ScriptingObject* object, const ScriptingObject* otherObj) override;
    void DeserializeObject(ISerializable::DeserializeStream& stream, ScriptingObject* object, ISerializeModifier* modifier) override;
    void OnObjectIdChanged(ScriptingObject* object, const Guid& oldId) override;
    void OnObjectDeleted(ScriptingObject* object) override;
    void Destroy(bool isReloading) override;
};

/// <summary>
/// The visual scripting runtime service. Allows to execute visual script functions with recursion support and thread-safety.
/// </summary>
class FLAXENGINE_API VisualScripting
{
public:
    struct NodeBoxValue
    {
        uint32 NodeId;
        uint32 BoxId;
        Variant Value;
    };

    struct ScopeContext
    {
        // Method input parameters
        Span<Variant> Parameters;
        // Invoke methods returned values cached within the scope (includes output parameters values)
        Array<NodeBoxValue, InlinedAllocation<16>> ReturnedValues;
        // Function result to return
        Variant FunctionReturn;
    };

    struct StackFrame
    {
        VisualScript* Script;
        VisualScriptGraphNode* Node;
        VisualScriptGraphNode::Box* Box;
        ScriptingObject* Instance;
        StackFrame* PreviousFrame;
        ScopeContext* Scope;
    };

    /// <summary>
    /// Gets the top frame of the current thread Visual Script execution stack frame.
    /// </summary>
    static StackFrame* GetThreadStackTop();

    /// <summary>
    /// Gets the current stack trace of the current thread Visual Script execution.
    /// </summary>
    /// <returns>The stack trace string.</returns>
    static String GetStackTrace();

    /// <summary>
    /// Gets the binary module for the Visual Scripting.
    /// </summary>
    static VisualScriptingBinaryModule* GetBinaryModule();

    /// <summary>
    /// Invokes the specified Visual Script method.
    /// </summary>
    /// <param name="method">The method to call.</param>
    /// <param name="instance">The object instance. Null for static methods.</param>
    /// <param name="parameters">The input parameters array. Unused if method is parameter-less.</param>
    /// <returns>The returned value. Undefined if method is void.</returns>
    static Variant Invoke(VisualScript::Method* method, ScriptingObject* instance, Span<Variant> parameters = Span<Variant>());

#if VISUAL_SCRIPT_DEBUGGING

    // Custom event that is called every time the Visual Script signal flows over the graph (including the data connections). Can be used to read nad visualize the Visual Script execution logic.
    static Action DebugFlow;

    /// <summary>
    /// Tries to evaluate a given script box value for the debugger (called usually during the breakpoint on the hanging thread).
    /// </summary>
    /// <param name="script">The script.</param>
    /// <param name="instance">The current object instance.</param>
    /// <param name="nodeId">The ID of the node in the script graph to evaluate it's box.</param>
    /// <param name="boxId">The ID of the box in the node to evaluate it's value.</param>
    /// <param name="result">The output value. Valid only if method returned true.</param>
    /// <returns>True if could fetch the value, otherwise false.</returns>
    static bool Evaluate(VisualScript* script, ScriptingObject* instance, uint32 nodeId, uint32 boxId, Variant& result);

#endif
};
