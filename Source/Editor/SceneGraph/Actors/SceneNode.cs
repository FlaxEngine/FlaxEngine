// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.SceneGraph.GUI;
using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Actor tree node for <see cref="FlaxEngine.Scene"/> objects.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class SceneNode : ActorNode
    {
        private bool _isEdited;

        /// <summary>
        /// Gets or sets a value indicating whether this scene is edited.
        /// </summary>
        public bool IsEdited
        {
            get => _isEdited;
            set
            {
                if (_isEdited != value)
                {
                    _isEdited = value;
                    _treeNode.UpdateText();
                }
            }
        }

        /// <summary>
        /// Gets the scene.
        /// </summary>
        public Scene Scene => _actor as Scene;

        /// <summary>
        /// Initializes a new instance of the <see cref="SceneNode"/> class.
        /// </summary>
        /// <param name="scene">The scene.</param>
        public SceneNode(Scene scene)
        : base(scene, new SceneTreeNode())
        {
        }

        /// <inheritdoc />
        public override bool CanCreatePrefab => false;

        /// <inheritdoc />
        public override bool CanCopyPaste => false;

        /// <inheritdoc />
        public override bool CanDuplicate => false;

        /// <inheritdoc />
        public override bool CanDelete => false;

        /// <inheritdoc />
        public override bool CanDrag => false;

        /// <inheritdoc />
        public override SceneNode ParentScene => this;

        /// <inheritdoc />
        public override void OnContextMenu(ContextMenu contextMenu)
        {
            contextMenu.AddSeparator();
            var path = Scene.Path;
            if (!string.IsNullOrEmpty(path) && File.Exists(path))
                contextMenu.AddButton("Select in Content", OnSelect).LinkTooltip("Finds and selects the scene asset int Content window.");
            contextMenu.AddButton("Save scene", OnSave).LinkTooltip("Saves this scene.").Enabled = IsEdited && !Editor.IsPlayMode;
            contextMenu.AddButton("Unload scene", OnUnload).LinkTooltip("Unloads this scene.").Enabled = Editor.Instance.StateMachine.CurrentState.CanChangeScene;

            base.OnContextMenu(contextMenu);
        }

        private void OnSelect()
        {
            Editor.Instance.Windows.ContentWin.Select(Editor.Instance.ContentDatabase.Find(Scene.Path));
        }
        
        private void OnSave()
        {
            Editor.Instance.Scene.SaveScene(this);
        }

        private void OnUnload()
        {
            Editor.Instance.Scene.CloseScene(Scene);
        }
    }
}
