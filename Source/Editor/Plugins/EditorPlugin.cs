// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor
{
    /// <summary>
    /// Base class for all plugins used in Editor.
    /// </summary>
    /// <remarks>
    /// Plugins should have a public and parameter-less constructor.
    /// </remarks>
    /// <seealso cref="FlaxEngine.Plugin" />
    public abstract class EditorPlugin : Plugin
    {
        /// <summary>
        /// Gets the type of the <see cref="GamePlugin"/> that is related to this plugin. Some plugins may be used only in editor while others want to gave a runtime representation. Use this property to link the related game plugin.
        /// </summary>
        public virtual Type GamePluginType => null;

        /// <summary>
        /// Gets the editor instance. Use it to extend the editor.
        /// </summary>
        public Editor Editor { get; private set; }

        /// <inheritdoc />
        public override void Initialize()
        {
            base.Initialize();

            Editor = Editor.Instance;
            if (Editor.IsInitialized)
            {
                InitializeEditor();
            }
            else
            {
                Editor.InitializationEnd += OnEditorInitializationEnd;
            }
        }

        private void OnEditorInitializationEnd()
        {
            Editor.InitializationEnd -= OnEditorInitializationEnd;
            InitializeEditor();
        }

        /// <summary>
        /// Initialization method called when this plugin is loaded and the Editor is after initialization. Use this method to add custom editor functionalities or override the existing ones.
        /// </summary>
        public virtual void InitializeEditor()
        {
        }
    }
}
