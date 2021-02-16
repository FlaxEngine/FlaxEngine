// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        /// <value>
        ///   <c>true</c> if this scene is edited; otherwise, <c>false</c>.
        /// </value>
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
        /// <value>
        /// The scene.
        /// </value>
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
    }
}
