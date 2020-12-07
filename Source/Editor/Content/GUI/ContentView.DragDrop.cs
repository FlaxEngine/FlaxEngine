// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.Drag;
using FlaxEditor.SceneGraph;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content.GUI
{
    public partial class ContentView
    {
        private bool _validDragOver;
        private DragActors _dragActors;

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
        {
            var result = base.OnDragEnter(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            // Check if drop file(s)
            if (data is DragDataFiles)
            {
                _validDragOver = true;
                return DragDropEffect.Copy;
            }

            // Check if drop actor(s)
            if (_dragActors == null)
                _dragActors = new DragActors(ValidateDragActors);
            if (_dragActors.OnDragEnter(data))
            {
                _validDragOver = true;
                return DragDropEffect.Move;
            }

            return DragDropEffect.None;
        }

        private bool ValidateDragActors(ActorNode actor)
        {
            return actor.CanCreatePrefab && Editor.Instance.Windows.ContentWin.CurrentViewFolder.CanHaveAssets;
        }

        private void ImportActors(DragActors actors, ContentFolder location)
        {
            // Use only the first actor
            Editor.Instance.Prefabs.CreatePrefab(actors.Objects[0].Actor);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
        {
            _validDragOver = false;
            var result = base.OnDragMove(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            if (data is DragDataFiles)
            {
                _validDragOver = true;
                result = DragDropEffect.Copy;
            }
            else if (_dragActors.HasValidDrag)
            {
                _validDragOver = true;
                result = DragDropEffect.Move;
            }

            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
        {
            var result = base.OnDragDrop(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            // Check if drop file(s)
            if (data is DragDataFiles files)
            {
                // Import files
                var currentFolder = Editor.Instance.Windows.ContentWin.CurrentViewFolder;
                if (currentFolder != null)
                    Editor.Instance.ContentImporting.Import(files.Files, currentFolder);
                result = DragDropEffect.Copy;
            }
            // Check if drop actor(s)
            else if (_dragActors.HasValidDrag)
            {
                // Import actors
                var currentFolder = Editor.Instance.Windows.ContentWin.CurrentViewFolder;
                if (currentFolder != null)
                    ImportActors(_dragActors, currentFolder);

                _dragActors.OnDragDrop();
                result = DragDropEffect.Move;
            }

            // Clear cache
            _validDragOver = false;

            return result;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            _validDragOver = false;
            _dragActors?.OnDragLeave();

            base.OnDragLeave();
        }
    }
}
