// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor
{
    partial class EditorPlugin
    {
        internal bool _isEditorInitialized;

        /// <summary>
        /// Gets the type of the <see cref="GamePlugin"/> that is related to this plugin. Some plugins may be used only in editor while others want to gave a runtime representation. Use this property to link the related game plugin.
        /// </summary>
        public virtual Type GamePluginType => null;

        /// <summary>
        /// Gets the editor instance. Use it to extend the editor.
        /// </summary>
        public Editor Editor { get; private set; }

        internal void Initialize_Internal()
        {
            Editor = Editor.Instance;
            if (Editor.IsInitialized)
            {
                _isEditorInitialized = true;
                InitializeEditor();
            }
            else
            {
                Editor.InitializationEnd += OnEditorInitializationEnd;
            }
        }

        internal void Deinitialize_Internal()
        {
            if (_isEditorInitialized)
            {
                _isEditorInitialized = false;
                DeinitializeEditor();
            }
        }

        private void OnEditorInitializationEnd()
        {
            Editor.InitializationEnd -= OnEditorInitializationEnd;
            if (_isEditorInitialized)
                return;
            _isEditorInitialized = true;
            InitializeEditor();
        }

        /// <summary>
        /// Initialization method called when this plugin is loaded and the Editor is after initialization. Use this method to add custom editor functionalities or override the existing ones.
        /// </summary>
        public virtual void InitializeEditor()
        {
        }

        /// <summary>
        /// Cleanup method called when this plugin is initialized and Editor is disposing (before plugin disposing). Use this method to remove any custom editor functionalities created within InitializeEditor.
        /// </summary>
        public virtual void DeinitializeEditor()
        {
        }
    }
}
