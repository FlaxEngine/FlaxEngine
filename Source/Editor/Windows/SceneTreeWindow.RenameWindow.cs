using System.Text;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// A window used to rename multiple actors.
    /// </summary>
    public class RenameWindow : EditorWindow
    {
        /// <inheritdoc/>
        private class RenameUndoAction : IUndoAction
        {
            /// <summary>
            /// The old actors name to use on <see cref="Undo"/> action.
            /// </summary>
            public string[] OldNames;

            /// <summary>
            /// The new actors name to use on <see cref="Do"/> action.
            /// </summary>
            public string[] NewNames;

            /// <summary>
            /// All actors to rename.
            /// </summary>
            public Actor[] ActorsToRename;

            /// <summary>
            /// Create a <see cref="RenameUndoAction"/> undo action.
            /// </summary>
            /// <param name="nodes"></param>
            public RenameUndoAction(Actor[] nodes)
            {
                ActorsToRename = nodes;
                OldNames = new string[nodes.Length];

                for (int i = 0; i < nodes.Length; i++)
                    OldNames[i] = nodes[i].Name;
            }

            /// <inheritdoc/>
            public void Do()
            {
                for (int i = 0; i < ActorsToRename.Length; i++)
                    ActorsToRename[i].Name = NewNames[i];
            }

            /// <inheritdoc/>
            public void Undo()
            {
                for (int i = 0; i < ActorsToRename.Length; i++)
                    ActorsToRename[i].Name = OldNames[i];
            }

            /// <inheritdoc/>
            public string ActionString => "Renaming actors.";

            /// <inheritdoc/>
            public void Dispose() { }
        }

        /// <summary>
        /// Rename options.
        /// </summary>
        private enum RenameOptions
        {
            OnlyName,
            UsePrefix,
            UseSufix
        }

        private Label _label;
        private TextBox _textBox;
        private EnumComboBox _renameOptions;
        private Button _renameButton;

        private Actor[] _actorsToRename;

        /// <summary>
        /// Create an instance of the <see cref="RenameWindow"/> to rename actors.
        /// </summary>
        /// <param name="actorsToRename">All actors to rename</param>
        /// <param name="editor">The editor.</param>
        public RenameWindow(Actor[] actorsToRename, Editor editor) : base(editor, true, FlaxEngine.GUI.ScrollBars.None)
        {
            Title = "Rename";
            _actorsToRename = actorsToRename;

            var container = new VerticalPanel
            {
                Parent = this,
                AnchorPreset = AnchorPresets.StretchAll,
                Offset = Vector2.Zero,

            };

            _label = new Label
            {
                Text = "New Name",
                AnchorPreset = AnchorPresets.TopLeft,
                Parent = container,
                Size = new Float2(100, 25)
            };

            _textBox = new TextBox
            {
                Text = "Actor",
                AnchorPreset = AnchorPresets.TopLeft,
                Parent = container,
                Size = new Float2(200, 25)
            };

            var renameOptionLabel = new Label
            {
                Text = "Rename Option",
                AnchorPreset = AnchorPresets.TopLeft,
                Parent = container,
                Size = new Float2(100, 25)
            };

            _renameOptions = new EnumComboBox(typeof(RenameOptions))
            {
                Parent = container,
                Value = 0
            };

            _renameButton = new Button
            {
                Text = "Rename",
                AnchorPreset = AnchorPresets.TopLeft,
                Parent = container,
                Size = new Float2(200, 25),
            };

            _renameButton.Clicked += () =>
            {
                var renameUndoAction = new RenameUndoAction(_actorsToRename);
                Editor.Instance.SceneEditing.Undo.AddAction(renameUndoAction);
                renameUndoAction.NewNames = new string[_actorsToRename.Length];
                for (int i = 0; i < _actorsToRename.Length; i++)
                {
                    var actor = _actorsToRename[i];
                    if (!actor)
                        continue;
                    var newName = new StringBuilder(_textBox.Text);
                    if (_renameOptions.Value == (int)RenameOptions.UsePrefix)
                    {
                        newName = new StringBuilder();
                        newName.Append(i);
                        newName.Append(_textBox.Text);
                    }
                    else if (_renameOptions.Value == (int)RenameOptions.UseSufix)
                        newName.Append(i.ToString());
                    var newNameStr = newName.ToString();
                    actor.Name = newNameStr;
                    renameUndoAction.NewNames[i] = newNameStr;
                }
                Editor.Instance.Scene.MarkAllScenesEdited();
                Close();
            };
        }

        ~RenameWindow()
        {
            _actorsToRename = null;
            _renameButton = null;
            _label = null;
            _textBox = null;
            _renameOptions = null;
        }
    }
}
