// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Modules;

namespace FlaxEditor
{
    /// <summary>
    /// Interface for undo action that can modify scene data (actors, scripts, etc.)
    /// </summary>
    public interface ISceneEditAction
    {
        /// <summary>
        /// Marks the scenes edited.
        /// </summary>
        /// <param name="sceneModule">The scene module.</param>
        void MarkSceneEdited(SceneModule sceneModule);
    }
}
