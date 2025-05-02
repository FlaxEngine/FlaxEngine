// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using FlaxEditor.Options;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.Modules.SourceCodeEditing
{
    /// <summary>
    /// Source code editing module.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class CodeEditingModule : EditorModule
    {
        private sealed class CachedVisualScriptPropertyTypesCollection : CachedTypesCollection
        {
            public CachedVisualScriptPropertyTypesCollection(int capacity, Func<Assembly, bool> checkAssembly)
            : base(capacity, ScriptType.Object, CheckFunc, checkAssembly)
            {
            }

            private static bool CheckFunc(ScriptType scriptType)
            {
                if (scriptType.IsStatic ||
                    scriptType.IsGenericType ||
                    !scriptType.IsPublic ||
                    scriptType.HasAttribute(typeof(HideInEditorAttribute), true) ||
                    scriptType.HasAttribute(typeof(System.Runtime.CompilerServices.CompilerGeneratedAttribute), false))
                    return false;
                var managedType = TypeUtils.GetType(scriptType);
                return !TypeUtils.IsDelegate(managedType);
            }

            /// <inheritdoc />
            protected override void Search()
            {
                // Add in-build types
                _list.Add(new ScriptType(typeof(bool)));
                _list.Add(new ScriptType(typeof(short)));
                _list.Add(new ScriptType(typeof(ushort)));
                _list.Add(new ScriptType(typeof(int)));
                _list.Add(new ScriptType(typeof(uint)));
                _list.Add(new ScriptType(typeof(long)));
                _list.Add(new ScriptType(typeof(ulong)));
                _list.Add(new ScriptType(typeof(float)));
                _list.Add(new ScriptType(typeof(double)));
                _list.Add(new ScriptType(typeof(string)));
                _list.Add(new ScriptType(typeof(Guid)));

                base.Search();
            }
        }

        private sealed class CachedAllTypesCollection : CachedTypesCollection
        {
            /// <inheritdoc />
            public CachedAllTypesCollection(int capacity, ScriptType type, Func<ScriptType, bool> checkFunc, Func<Assembly, bool> checkAssembly)
            : base(capacity, type, checkFunc, checkAssembly)
            {
            }

            /// <inheritdoc />
            protected override void Search()
            {
                // Add in-build types
                _list.Add(new ScriptType(typeof(void)));
                _list.Add(new ScriptType(typeof(bool)));
                _list.Add(new ScriptType(typeof(byte)));
                _list.Add(new ScriptType(typeof(short)));
                _list.Add(new ScriptType(typeof(ushort)));
                _list.Add(new ScriptType(typeof(int)));
                _list.Add(new ScriptType(typeof(uint)));
                _list.Add(new ScriptType(typeof(long)));
                _list.Add(new ScriptType(typeof(ulong)));
                _list.Add(new ScriptType(typeof(float)));
                _list.Add(new ScriptType(typeof(double)));
                _list.Add(new ScriptType(typeof(string)));
                _list.Add(new ScriptType(typeof(Guid)));
                _list.Add(new ScriptType(typeof(DateTime)));
                _list.Add(new ScriptType(typeof(TimeSpan)));

                base.Search();
            }
        }

        private readonly List<ISourceCodeEditor> _editors = new List<ISourceCodeEditor>(8);
        private ISourceCodeEditor _selectedEditor;
        private bool _autoGenerateScriptsProjectFiles;

        /// <summary>
        /// Gets the source code editors registered for usage in editor.
        /// </summary>
        public IReadOnlyList<ISourceCodeEditor> Editors => _editors;

        /// <summary>
        /// Occurs when source code editor gets added.
        /// </summary>
        public event Action<ISourceCodeEditor> EditorAdded;

        /// <summary>
        /// Occurs when source code editor gets removed.
        /// </summary>
        public event Action<ISourceCodeEditor> EditorRemoved;

        /// <summary>
        /// Occurs when selected source code editor gets changed.
        /// </summary>
        public event Action<ISourceCodeEditor> SelectedEditorChanged;

        /// <summary>
        /// Gets or sets the selected editor.
        /// </summary>
        public ISourceCodeEditor SelectedEditor
        {
            get => _selectedEditor;
            set
            {
                if (_selectedEditor != value)
                {
                    if (value != null && !_editors.Contains(value))
                        throw new ArgumentException("Cannot selected editor that has not been added to the editors list.");

                    var prev = _selectedEditor;
                    _selectedEditor = value;

                    Editor.Log("Select code editor " + (_selectedEditor?.Name ?? "null"));

                    prev?.OnDeselected(Editor);
                    _selectedEditor?.OnSelected(Editor);
                    SelectedEditorChanged?.Invoke(_selectedEditor);
                }
            }
        }

        /// <summary>
        /// Occurs when cached scripting types lists are cleared (eg. on global invalidation or scripting reload).
        /// </summary>
        public event Action TypesCleared;

        /// <summary>
        /// Occurs when types information gets modified (eg. after scripting reload or when script asset gets saved). Can be used to refresh any cached locally script types.
        /// </summary>
        public event Action TypesChanged;

        /// <summary> 
        /// The all types collection from all assemblies (excluding C# system libraries). Includes only primitive and basic types from std lib.
        /// </summary>
        public readonly CachedTypesCollection All = new CachedAllTypesCollection(8096, ScriptType.Null, type => true, HasAssemblyValidAnyTypes);

        /// <summary> 
        /// The all types collection from all assemblies (including C# system libraries).
        /// </summary>
        public readonly CachedTypesCollection AllWithStd = new CachedTypesCollection(8096, ScriptType.Null, type => true, assembly => true);

        /// <summary>
        /// The all valid types collection for the Visual Script property types (includes basic types like int/float, structures, object references).
        /// </summary>
        public readonly CachedTypesCollection VisualScriptPropertyTypes = new CachedVisualScriptPropertyTypesCollection(8096, HasAssemblyValidAnyTypes);

        /// <summary>
        /// The actors collection.
        /// </summary>
        public readonly CachedTypesCollection Actors = new CachedTypesCollection(1024, new ScriptType(typeof(Actor)), IsTypeValidScriptingType, HasAssemblyValidScriptingTypes);

        /// <summary>
        /// The scripts collection.
        /// </summary>
        public readonly CachedTypesCollection Scripts = new CachedTypesCollection(1024, new ScriptType(typeof(Script)), IsTypeValidScriptingType, HasAssemblyValidScriptingTypes);

        /// <summary>
        /// The control types collection (for game UI).
        /// </summary>
        public readonly CachedTypesCollection Controls = new CachedTypesCollection(64, new ScriptType(typeof(Control)), IsTypeValidScriptingTypeControls, HasAssemblyValidScriptingTypes);

        /// <summary>
        /// The Animation Graph custom nodes collection.
        /// </summary>
        public readonly CachedCustomAnimGraphNodesCollection AnimGraphNodes = new CachedCustomAnimGraphNodesCollection(32, new ScriptType(typeof(AnimationGraph.CustomNodeArchetypeFactoryAttribute)), IsTypeValidScriptingType, HasAssemblyValidScriptingTypes);

        /// <summary>
        /// The Behavior Tree custom nodes collection.
        /// </summary>
        public readonly CachedTypesCollection BehaviorTreeNodes = new CachedTypesCollection(64, new ScriptType(typeof(BehaviorTreeNode)), IsTypeValidScriptingType, HasAssemblyValidScriptingTypes);

        internal CodeEditingModule(Editor editor)
        : base(editor)
        {
        }

        internal InBuildSourceCodeEditor GetInBuildEditor(CodeEditorTypes editorType)
        {
            for (var i = 0; i < Editors.Count; i++)
            {
                if (Editors[i] is InBuildSourceCodeEditor inBuild && inBuild.Type == editorType)
                    return inBuild;
            }
            return null;
        }

        /// <summary>
        /// Adds the editor to the collection.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public void AddEditor(ISourceCodeEditor editor)
        {
            if (editor == null)
                throw new ArgumentNullException();
            if (_editors.Contains(editor))
                throw new ArgumentException("Editor already added.");

            Editor.Log("Add code editor " + editor.Name);

            _editors.Add(editor);
            editor.OnAdded(Editor);
            EditorAdded?.Invoke(editor);
        }

        /// <summary>
        /// Removes the editor from the collection.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public void RemoveEditor(ISourceCodeEditor editor)
        {
            if (editor == null)
                throw new ArgumentNullException();
            if (!_editors.Contains(editor))
                throw new ArgumentException("Editor not added.");

            if (_selectedEditor == editor)
                SelectedEditor = null;

            Editor.Log("Remove code editor " + editor.Name);

            _editors.Remove(editor);
            editor.OnRemoved(Editor);
            EditorRemoved?.Invoke(editor);
        }

        /// <summary>
        /// Opens the solution file using the selected selected code editor.
        /// </summary>
        public void OpenSolution()
        {
            if (_selectedEditor == null)
                return;

            _selectedEditor.OpenSolution();
        }

        /// <summary>
        /// Opens the file using the selected code editor.
        /// </summary>
        /// <param name="path">The file path to open.</param>
        /// <param name="line">The line number to navigate to. Use 0 to not use it.</param>
        public void OpenFile(string path, int line = 0)
        {
            if (_selectedEditor == null)
                return;

            _selectedEditor.OpenFile(path, line);
        }

        private unsafe void GetInBuildEditors()
        {
            var resultArray = stackalloc int[(int)CodeEditorTypes.MAX];
            ScriptsBuilder.GetExistingEditors(resultArray, (int)CodeEditorTypes.MAX);
            for (int i = 0; i < (int)CodeEditorTypes.MAX; i++)
            {
                if (resultArray[i] != 0)
                {
                    var editor = new InBuildSourceCodeEditor((CodeEditorTypes)i);
                    AddEditor(editor);
                }
            }
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            ScriptsBuilder.ScriptsReload += OnScriptsReload;
            ScriptsBuilder.ScriptsReloadEnd += OnScriptsReloadEnd;
            ScriptsBuilder.ScriptsLoaded += OnScriptsLoaded;
            Editor.Options.OptionsChanged += OnOptionsChanged;

            // Add default code editors (in-build)
            GetInBuildEditors();
            AddEditor(new DefaultSourceCodeEditor());

            // Editor options are loaded before this module so pick the code editor
            OnOptionsChanged(Editor.Options.Options);
        }

        /// <inheritdoc />
        public override void OnEndInit()
        {
            // Special case when failed to load scripts on editor start - clear types later so all types will be cached if needed
            if (!FlaxEngine.Scripting.HasGameModulesLoaded())
            {
                ScriptsBuilder.CompilationSuccess += OnFirstCompilationSuccess;
            }
        }

        private void OnFirstCompilationSuccess()
        {
            ScriptsBuilder.CompilationSuccess -= OnFirstCompilationSuccess;

            ClearTypes();
        }

        private void OnOptionsChanged(EditorOptions options)
        {
            // Sync options
            _autoGenerateScriptsProjectFiles = options.SourceCode.AutoGenerateScriptsProjectFiles;

            // Sync code editor
            ISourceCodeEditor editor = null;
            var codeEditor = options.SourceCode.SourceCodeEditor;
            if (codeEditor != "None")
            {
                foreach (var e in Editor.Instance.CodeEditing.Editors)
                {
                    if (string.Equals(codeEditor, e.Name, StringComparison.OrdinalIgnoreCase))
                    {
                        editor = e;
                        break;
                    }
                }
            }
            Editor.Instance.CodeEditing.SelectedEditor = editor;
        }

        /// <summary>
        /// Starts creating a new module
        /// </summary>
        internal void CreateModule(string path, string moduleName, bool editorModule, bool cpp)
        {
            if (string.IsNullOrEmpty(moduleName) || string.IsNullOrEmpty(path))
            {
                Editor.LogWarning("Failed to create module due to no name");
                return;
            }

            // Create folder
            var moduleFolderPath = Path.Combine(path, moduleName);
            Directory.CreateDirectory(moduleFolderPath);

            // Create module
            var moduleText = "using Flax.Build;\n" +
                             "using Flax.Build.NativeCpp;\n" +
                             $"\npublic class {moduleName} : Game{(editorModule ? "Editor" : "")}Module\n" +
                             "{\n    " +
                             "/// <inheritdoc />\n" +
                             "    public override void Init()\n" +
                             "    {\n" +
                             "        base.Init();\n" +
                             "\n" +
                             "        // C#-only scripting if false\n" +
                             $"        BuildNativeCode = {(cpp ? "true" : "false")};\n" +
                             "    }\n" +
                             "\n" +
                             "    /// <inheritdoc />\n" +
                             "    public override void Setup(BuildOptions options)\n" +
                             "    {" +
                             "\n" +
                             "        base.Setup(options);\n" +
                             "\n" +
                             "        options.ScriptingAPI.IgnoreMissingDocumentationWarnings = true;\n" +
                             "\n" +
                             "        // Here you can modify the build options for your game module\n" +
                             "        // To reference another module use: options.PublicDependencies.Add(\"Audio\");\n" +
                             "        // To add C++ define use: options.PublicDefinitions.Add(\"COMPILE_WITH_FLAX\");\n" +
                             "        // To learn more see scripting documentation.\n" +
                             "    }\n" +
                             "}";
            moduleText = Encoding.UTF8.GetString(Encoding.Default.GetBytes(moduleText));
            var modulePath = Path.Combine(moduleFolderPath, $"{moduleName}.Build.cs");
            File.WriteAllText(modulePath, moduleText);
            Editor.Log($"Module created at {modulePath}");

            // Get editor target and target files and add module
            var files = Directory.GetFiles(path);
            var targetModuleText = $"Modules.Add(nameof({moduleName}));\n        ";
            foreach (var file in files)
            {
                if (!file.Contains(".Build.cs", StringComparison.OrdinalIgnoreCase))
                    continue;
                var targetText = File.ReadAllText(file);

                // Skip game project if it is suppose to be an editor module
                if (editorModule && targetText.Contains("GameProjectTarget", StringComparison.Ordinal))
                    continue;

                // TODO: Handle edge case when there are no modules in a target
                var index = targetText.IndexOf("Modules.Add");
                if (index != -1)
                {
                    var newText = targetText.Insert(index, targetModuleText);
                    File.WriteAllText(file, newText);
                    Editor.Log($"Module added to Target: {file}");
                }
            }
        }

        internal void RemoveModule(string path)
        {
            if (!File.Exists(path))
                return;

            // Read text, figure out if it is an editor module or game module
            var editorModule = false;
            var moduleTextIndex = -1;
            var fileText = File.ReadAllText(path);
            if (fileText.Contains("GameModule", StringComparison.Ordinal))
            {
                moduleTextIndex = fileText.IndexOf("GameModule", StringComparison.Ordinal);
            }
            else if (fileText.Contains("ThirdPartyModule", StringComparison.Ordinal))
            {
                moduleTextIndex = fileText.IndexOf("ThirdPartyModule", StringComparison.Ordinal);
            }
            else if (fileText.Contains("DepsModule", StringComparison.Ordinal))
            {
                moduleTextIndex = fileText.IndexOf("DepsModule", StringComparison.Ordinal);
            }
            else if (fileText.Contains("GameEditorModule", StringComparison.Ordinal))
            {
                moduleTextIndex = fileText.IndexOf("GameEditorModule", StringComparison.Ordinal);
                editorModule = true;
            }
            else
            {
                // If it does not contain a module, then this could be target file and not a module file
                return;
            }

            // Get module name
            var classTextIndex = fileText.IndexOf("class ", StringComparison.Ordinal);
            var className = fileText.Substring(classTextIndex, moduleTextIndex - classTextIndex).Replace("class ", "").Replace(":", "").Trim();
            Editor.Log($"Removing Module: {className}");

            // Find target files
            // Assume Target files are in the source directory that is up 2 levels
            var sourceDirectoryInfo = Directory.GetParent(path)?.Parent;
            if (sourceDirectoryInfo != null)
            {
                var sourceFiles = Directory.GetFiles(sourceDirectoryInfo.FullName);
                // Search target files for module name and remove it
                foreach (var file in sourceFiles)
                {
                    string fileName = Path.GetFileName(file);
                    if (file.Contains(".Build.cs", StringComparison.OrdinalIgnoreCase))
                    {
                        var targetText = File.ReadAllText(file);

                        // Skip game project if it is suppose to be an editor module
                        if (editorModule && targetText.Contains("GameProjectTarget", StringComparison.Ordinal))
                            continue;

                        var newText = targetText;
                        bool removedModuleText = false;
                        if (targetText.Contains($"Modules.Add(\"{className}\")", StringComparison.Ordinal))
                        {
                            newText = newText.Replace($"Modules.Add(\"{className}\");\n", "", StringComparison.Ordinal).Replace($"Modules.Add(\"{className}\");", "", StringComparison.Ordinal);
                            removedModuleText = true;
                        }

                        if (targetText.Contains($"Modules.Add(nameof({className}))", StringComparison.Ordinal))
                        {
                            newText = newText.Replace($"Modules.Add(nameof({className}));\n", "", StringComparison.Ordinal).Replace($"Modules.Add(nameof({className}));", "", StringComparison.Ordinal);
                            removedModuleText = true;
                        }
                        if (removedModuleText)
                        {
                            File.WriteAllText(file, newText);
                            Editor.Log($"Removed Module: {className} from {file}");
                        }
                    }
                    // Remove Generated module files
                    else if (fileName.Equals($"{className}.csproj", StringComparison.Ordinal) || 
                             fileName.Equals($"{className}.Gen.cs", StringComparison.Ordinal) ||
                             fileName.Equals($"{className}.Gen.cpp", StringComparison.Ordinal) ||
                             fileName.Equals($"{className}.Gen.h", StringComparison.Ordinal))
                    {
                        File.Delete(file);
                        Editor.Log($"Deleted generated modules file for module: {className}. File path {file}");
                    }
                }
            }
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            base.OnUpdate();

            // Automatic project files generation after workspace modifications
            if (_autoGenerateScriptsProjectFiles && ScriptsBuilder.IsSourceWorkspaceDirty && !ScriptsBuilder.IsCompiling)
            {
                // Try to delay generation when a lot of files are added at once
                if (ScriptsBuilder.IsSourceDirtyFor(TimeSpan.FromMilliseconds(150)))
                    Editor.ProgressReporting.GenerateScriptsProjectFiles.RunAsync();
            }
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            // Cleanup
            SelectedEditor = null;
            _editors.Clear();

            ScriptsBuilder.ScriptsReload -= OnScriptsReload;
            ScriptsBuilder.ScriptsReloadEnd -= OnScriptsReloadEnd;
            ScriptsBuilder.ScriptsLoaded -= OnScriptsLoaded;
        }

        /// <summary>
        /// Clears all the cached types.
        /// </summary>
        public void ClearTypes()
        {
            // Invalidate cached types
            All.ClearTypes();
            VisualScriptPropertyTypes.ClearTypes();
            Actors.ClearTypes();
            Scripts.ClearTypes();
            Controls.ClearTypes();
            AnimGraphNodes.ClearTypes();
            BehaviorTreeNodes.ClearTypes();
            TypesCleared?.Invoke();
        }

        /// <summary>
        /// Calls the types change event to inform the Editor.
        /// </summary>
        public void OnTypesChanged()
        {
            TypesChanged?.Invoke();
        }

        private void OnScriptsReload()
        {
            ClearTypes();
        }

        private void OnScriptsReloadEnd()
        {
            OnTypesChanged();
        }

        private void OnScriptsLoaded()
        {
            // Clear any state with engine-only types
            ClearTypes();
        }

        private static bool IsTypeValidScriptingType(ScriptType t)
        {
            return !t.IsGenericType && !t.IsAbstract && !t.HasAttribute(typeof(HideInEditorAttribute), false);
        }

        private static bool IsTypeValidScriptingTypeControls(ScriptType t)
        {
            // Skip Flax Editor internal controls from usage in game
            if (t.Type?.Namespace?.StartsWith("FlaxEditor") ?? false)
                return false;

            return IsTypeValidScriptingType(t);
        }

        private static bool HasAssemblyValidAnyTypes(Assembly assembly)
        {
            var codeBase = Utils.GetAssemblyLocation(assembly);
            if (string.IsNullOrEmpty(codeBase))
                return true;
#if USE_NETCORE
            if (assembly.ManifestModule.FullyQualifiedName == "<In Memory Module>")
                return false;

            // Skip runtime related assemblies
            string repositoryUrl = assembly.GetCustomAttributes<AssemblyMetadataAttribute>().FirstOrDefault(x => x.Key == "RepositoryUrl")?.Value ?? "";
            if (repositoryUrl != "https://github.com/dotnet/runtime")
                return true;
#else
            // Skip assemblies from in-build Mono directory
            if (!codeBase.Contains("/Mono/lib/mono/"))
                return true;
#endif
            return false;
        }

        private static bool HasAssemblyValidScriptingTypes(Assembly a)
        {
            // Check engine assembly
            if (a.GetName().Name == Utilities.Utils.FlaxEngineAssemblyName)
                return true;

            // Skip assemblies not referencing engine assembly
            var references = a.GetReferencedAssemblies();
            return references.Any(x => x.Name == Utilities.Utils.FlaxEngineAssemblyName);
        }
    }
}
