// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.GUI
{
    /// <summary>
    /// Displays custom editor property name.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Label" />
    [HideInEditor]
    public class PropertyNameLabel : Label
    {
        // TODO: if name is too long to show -> use tooltip to show it

        /// <summary>
        /// Custom event delegate that can be used to extend the property name label with an additional functionality.
        /// </summary>
        /// <param name="label">The label.</param>
        /// <param name="menu">The menu.</param>
        /// <param name="linkedEditor">The linked editor. Can be null.</param>
        public delegate void SetupContextMenuDelegate(PropertyNameLabel label, ContextMenu menu, CustomEditor linkedEditor);

        private bool _mouseDown;

        /// <summary>
        /// Helper value used by the <see cref="PropertiesList"/> to draw property names in a proper area.
        /// </summary>
        internal int FirstChildControlIndex;

        /// <summary>
        /// Helper value used by the <see cref="PropertiesList"/> to draw property names in a proper area.
        /// </summary>
        internal PropertiesList FirstChildControlContainer;

        /// <summary>
        /// The linked custom editor (shows the label property).
        /// </summary>
        internal CustomEditor LinkedEditor;

        /// <summary>
        /// The highlight strip color drawn on a side (transparent if skip rendering).
        /// </summary>
        public Color HighlightStripColor;

        /// <summary>
        /// Occurs when label creates the context menu popup for th property. Can be used to add some custom logic per property editor.
        /// </summary>
        public event SetupContextMenuDelegate SetupContextMenu;

        /// <summary>
        /// Initializes a new instance of the <see cref="PropertyNameLabel"/> class.
        /// </summary>
        /// <param name="name">The name.</param>
        public PropertyNameLabel(string name)
        {
            Text = name;
            HorizontalAlignment = TextAlignment.Near;
            VerticalAlignment = TextAlignment.Center;
            Margin = new Margin(4, 0, 0, 0);
            ClipText = true;

            HighlightStripColor = Color.Transparent;
        }

        internal void LinkEditor(CustomEditor editor)
        {
            if (LinkedEditor == null)
            {
                LinkedEditor = editor;
                editor.LinkLabel(this);
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            if (HighlightStripColor.A > 0.0f)
            {
                Render2D.FillRectangle(new Rectangle(0, 0, 2, Height), HighlightStripColor);
            }
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _mouseDown = false;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Right)
            {
                _mouseDown = true;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            if (_mouseDown && button == MouseButton.Right)
            {
                _mouseDown = false;

                // Skip if is not extended
                var linkedEditor = LinkedEditor;
                if (linkedEditor == null && SetupContextMenu == null)
                    return false;

                var menu = new ContextMenu();

                if (linkedEditor != null)
                {
                    var features = linkedEditor.Presenter.Features;
                    if ((features & (FeatureFlags.UseDefault | FeatureFlags.UsePrefab)) != 0)
                    {
                        if ((features & FeatureFlags.UsePrefab) != 0)
                            menu.AddButton("Revert to Prefab", linkedEditor.RevertToReferenceValue).Enabled = linkedEditor.CanRevertReferenceValue;
                        if ((features & FeatureFlags.UseDefault) != 0)
                            menu.AddButton("Reset to default", linkedEditor.RevertToDefaultValue).Enabled = linkedEditor.CanRevertDefaultValue;
                        menu.AddSeparator();
                    }
                    menu.AddButton("Copy", linkedEditor.Copy);
                    var paste = menu.AddButton("Paste", linkedEditor.Paste);
                    paste.Enabled = linkedEditor.CanPaste;
                }

                SetupContextMenu?.Invoke(this, menu, linkedEditor);

                menu.Show(this, location);

                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            _mouseDown = false;

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            SetupContextMenu = null;
            LinkedEditor = null;
            FirstChildControlContainer = null;

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Text.ToString();
        }
    }
}
