// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors.Editors;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Tools;

namespace FlaxEditor.CustomEditors.Dedicated;

/// <summary>
/// The missing script editor.
/// </summary>
[CustomEditor(typeof(ModelPrefab)), DefaultEditor]
public class ModelPrefabEditor : GenericEditor
{
    private Guid _prefabId;
    private Button _reimportButton;
    private string _importPath;

    /// <inheritdoc />
    public override void Initialize(LayoutElementsContainer layout)
    {
        base.Initialize(layout);

        var modelPrefab = Values[0] as ModelPrefab;
        if (modelPrefab == null)
            return;
        _prefabId = modelPrefab.PrefabID;
        while (true)
        {
            if (_prefabId == Guid.Empty)
            {
                break;
            }

            var prefab = FlaxEngine.Content.Load<Prefab>(_prefabId);
            if (prefab)
            {
                var prefabObjectId = modelPrefab.PrefabObjectID;
                var prefabObject = prefab.GetDefaultInstance(ref prefabObjectId);
                if (prefabObject.PrefabID == _prefabId)
                    break;
                _prefabId = prefabObject.PrefabID;
            }
            else
            {
                // The model was removed earlier
                _prefabId = Guid.Empty;
                break;
            }
        }

        // Creates the import path UI
        Utilities.Utils.CreateImportPathUI(layout, modelPrefab.ImportPath, false);

        var button = layout.Button("Reimport", "Reimports the source asset as prefab.");
        _reimportButton = button.Button;
        _reimportButton.Clicked += OnReimport;
    }

    private void OnReimport()
    {
        var prefab = FlaxEngine.Content.Load<Prefab>(_prefabId);
        var modelPrefab = (ModelPrefab)Values[0];
        var importPath = modelPrefab.ImportPath;
        var editor = Editor.Instance;
        if (editor.ContentImporting.GetReimportPath("Model Prefab", ref importPath))
            return;
        var folder = editor.ContentDatabase.Find(Path.GetDirectoryName(prefab.Path)) as ContentFolder;
        if (folder == null)
            return;
        var importOptions = modelPrefab.ImportOptions;
        importOptions.Type = ModelTool.ModelType.Prefab;
        _importPath = importPath;
        _reimportButton.Enabled = false;
        editor.ContentImporting.ImportFileEnd += OnImportFileEnd;
        editor.ContentImporting.Import(importPath, folder, true, importOptions);
    }

    private void OnImportFileEnd(IFileEntryAction entry, bool failed)
    {
        if (entry.SourceUrl == _importPath)
        {
            // Restore button
            _importPath = null;
            _reimportButton.Enabled = true;
            Editor.Instance.ContentImporting.ImportFileEnd -= OnImportFileEnd;
        }
    }
}
