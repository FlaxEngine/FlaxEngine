using System.Text;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEditor.GUI;

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
            UseSuffix
        }

        private string _newActorsName;
        private RenameOptions _renameOption;
        private Actor[] _actorsToRename;

        private static RenameWindow _currentOpenedWindow;

        private RenameWindow(Actor[] actorsToRename, Editor editor) : base(editor, true, FlaxEngine.GUI.ScrollBars.None)
        {
            Title = "Rename";
            Size = new Float2(300, 110);

            _newActorsName = "Actor ";
            _renameOption = RenameOptions.UseSuffix;
            _actorsToRename = actorsToRename;

            var container = new VerticalPanel
            {
                Parent = this,
                AnchorPreset = AnchorPresets.StretchAll,
                Offset = Vector2.Zero,
                Pivot = Float2.Zero,
                AutoSize = false,
                Bounds = Rectangle.Empty
            };

            var nameContainer = new HorizontalPanel
            {
                Parent = container,
                AnchorPreset = AnchorPresets.TopLeft,
                Bounds = new Rectangle(0, 0, 300, 22),
                Offset = Vector2.Zero,
                AutoSize = false,
                Spacing = 2,
                CullChildren = false,
                ClipChildren = false,
            };

            var optionsContainer = new HorizontalPanel
            {
                Parent = container,
                AnchorPreset = AnchorPresets.TopLeft,
                Bounds = new Rectangle(0, 22, 300, 22),
                Offset = Vector2.Zero,
                AutoSize = false,
                Spacing = 2,
                CullChildren = false,
                ClipChildren = false,
            };

            var renameLabel = new Label
            {
                Text = "New Name",
                AnchorPreset = AnchorPresets.Custom,
                AnchorMin = Float2.Zero,
                AnchorMax = new Float2(0.5f, 0),
                Parent = nameContainer,
                HorizontalAlignment = TextAlignment.Near,
                Size = new Float2(150, 22),
                Offsets = Margin.Zero,
            };

            var newNameTextBox = new TextBox
            {
                Text = _newActorsName,
                AnchorPreset = AnchorPresets.Custom,
                AnchorMin = new Float2(0.5f, 0),
                AnchorMax = new Float2(1, 0),
                Parent = nameContainer,
                Size = new Float2(150, 22),
                Offsets = Margin.Zero,
            };

            var optionNameLabel = new Label
            {
                Text = "Rename Option",
                HorizontalAlignment = TextAlignment.Near,
                AnchorPreset = AnchorPresets.Custom,
                AnchorMin = Float2.Zero,
                AnchorMax = new Float2(0.5f, 0),
                Parent = optionsContainer,
                Size = new Float2(150, 22),
                Offsets = Margin.Zero,
            };

            var renameOptions = new EnumComboBox(typeof(RenameOptions))
            {
                Parent = optionsContainer,
                Value = (int)_renameOption,
                AnchorPreset = AnchorPresets.Custom,
                AnchorMin = new Float2(0.5f, 0f),
                AnchorMax = new Float2(1, 0),
                Size = new Float2(150, 22),
                Offsets = Margin.Zero,
            };

            var renameButton = new Button
            {
                Text = "Rename",
                AnchorPreset = AnchorPresets.TopLeft,
                Parent = container,
            };

            newNameTextBox.TextBoxEditEnd += textBox =>
            {
                _newActorsName = textBox.Text;
            };

            renameOptions.EnumValueChanged += combo =>
            {
                _renameOption = (RenameOptions)combo.Value;
            };

            newNameTextBox.Focus();
            newNameTextBox.KeyDown += k =>
            {
                if (k == KeyboardKeys.Return)
                {
                    _newActorsName = newNameTextBox.Text;
                    RenameActors();
                }
            };

            renameButton.Clicked += RenameActors;
        }

        private void RenameActors()
        {
            var renameUndoAction = new RenameUndoAction(_actorsToRename);
            Editor.Instance.SceneEditing.Undo.AddAction(renameUndoAction);
            renameUndoAction.NewNames = new string[_actorsToRename.Length];
            for (int i = 0; i < _actorsToRename.Length; i++)
            {
                var actor = _actorsToRename[i];
                if (!actor)
                    continue;
                var newName = new StringBuilder(_newActorsName);
                if (_renameOption == RenameOptions.UsePrefix)
                {
                    newName = new StringBuilder();
                    newName.Append(i);
                    newName.Append(_newActorsName);
                }
                else if (_renameOption == RenameOptions.UseSuffix)
                    newName.Append(i.ToString());

                var newNameStr = newName.ToString();
                actor.Name = newNameStr;
                renameUndoAction.NewNames[i] = newNameStr;
            }
            Editor.Instance.Scene.MarkAllScenesEdited();
            Close();
        }

        /// <summary>
        /// Create an instance of the <see cref="RenameWindow"/> to rename actors and show the window.
        /// </summary>
        /// <param name="actorsToRename">All actors to rename</param>
        /// <param name="editor">The editor.</param>
        public static void Show(Actor[] actorsToRename, Editor editor)
        {
            // Can only one window opened.
            if (_currentOpenedWindow != null)
                _currentOpenedWindow.Close(ClosingReason.CloseEvent);

            _currentOpenedWindow = new RenameWindow(actorsToRename, editor);
            _currentOpenedWindow.ShowFloating(new Float2(300, 110));
            _currentOpenedWindow.RootWindow.Window.Closed += () =>
            {
                _currentOpenedWindow = null;
            };
        }
    }
}
