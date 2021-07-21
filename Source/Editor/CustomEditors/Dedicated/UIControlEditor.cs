// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using System.Reflection;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Dedicated custom editor for <see cref="AnchorPresets"/> enum.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.CustomEditor" />
    [CustomEditor(typeof(AnchorPresets)), DefaultEditor]
    public sealed class AnchorPresetsEditor : CustomEditor
    {
        class AnchorButton : Button
        {
            private AnchorPresets _presets = (AnchorPresets)((int)AnchorPresets.StretchAll + 1);

            public AnchorPresets Presets
            {
                get => _presets;
                set
                {
                    if (_presets != value)
                    {
                        _presets = value;
                        TooltipText = CustomEditorsUtil.GetPropertyNameUI(_presets.ToString());
                    }
                }
            }

            public bool IsSelected;
            public bool SupportsShiftModulation;

            /// <inheritdoc />
            public override void Draw()
            {
                // Cache data
                var rect = new Rectangle(Vector2.Zero, Size);
                if (rect.Width >= rect.Height)
                {
                    rect.X = (rect.Width - rect.Height) * 0.5f;
                    rect.Width = rect.Height;
                }
                else
                {
                    rect.Y = (rect.Height - rect.Width) * 0.5f;
                    rect.Height = rect.Width;
                }
                var enabled = EnabledInHierarchy;
                var style = FlaxEngine.GUI.Style.Current;
                var backgroundColor = BackgroundColor;
                var borderColor = BorderColor;
                if (!enabled)
                {
                    backgroundColor *= 0.7f;
                    borderColor *= 0.7f;
                }
                else if (_isPressed)
                {
                    backgroundColor = BackgroundColorSelected;
                    borderColor = BorderColorSelected;
                }
                else if (IsMouseOver)
                {
                    backgroundColor = BackgroundColorHighlighted;
                    borderColor = BorderColorHighlighted;
                }

                if (SupportsShiftModulation && Input.GetKey(KeyboardKeys.Shift))
                {
                    backgroundColor = BackgroundColorSelected;
                }

                // Calculate fill area
                float fillSize = rect.Width / 3;
                Rectangle fillArea;
                switch (_presets)
                {
                case AnchorPresets.Custom:
                    fillArea = Rectangle.Empty;
                    break;
                case AnchorPresets.TopLeft:
                    fillArea = new Rectangle(0, 0, fillSize, fillSize);
                    break;
                case AnchorPresets.TopCenter:
                    fillArea = new Rectangle(fillSize, 0, fillSize, fillSize);
                    break;
                case AnchorPresets.TopRight:
                    fillArea = new Rectangle(fillSize * 2, 0, fillSize, fillSize);
                    break;
                case AnchorPresets.MiddleLeft:
                    fillArea = new Rectangle(0, fillSize, fillSize, fillSize);
                    break;
                case AnchorPresets.MiddleCenter:
                    fillArea = new Rectangle(fillSize, fillSize, fillSize, fillSize);
                    break;
                case AnchorPresets.MiddleRight:
                    fillArea = new Rectangle(fillSize * 2, fillSize, fillSize, fillSize);
                    break;
                case AnchorPresets.BottomLeft:
                    fillArea = new Rectangle(0, fillSize * 2, fillSize, fillSize);
                    break;
                case AnchorPresets.BottomCenter:
                    fillArea = new Rectangle(fillSize, fillSize * 2, fillSize, fillSize);
                    break;
                case AnchorPresets.BottomRight:
                    fillArea = new Rectangle(fillSize * 2, fillSize * 2, fillSize, fillSize);
                    break;
                case AnchorPresets.VerticalStretchLeft:
                    fillArea = new Rectangle(0, 0, fillSize, fillSize * 3);
                    break;
                case AnchorPresets.VerticalStretchRight:
                    fillArea = new Rectangle(fillSize * 2, 0, fillSize, fillSize * 3);
                    break;
                case AnchorPresets.VerticalStretchCenter:
                    fillArea = new Rectangle(fillSize, 0, fillSize, fillSize * 3);
                    break;
                case AnchorPresets.HorizontalStretchTop:
                    fillArea = new Rectangle(0, 0, fillSize * 3, fillSize);
                    break;
                case AnchorPresets.HorizontalStretchMiddle:
                    fillArea = new Rectangle(0, fillSize, fillSize * 3, fillSize);
                    break;
                case AnchorPresets.HorizontalStretchBottom:
                    fillArea = new Rectangle(0, fillSize * 2, fillSize * 3, fillSize);
                    break;
                case AnchorPresets.StretchAll:
                    fillArea = new Rectangle(0, 0, fillSize * 3, fillSize * 3);
                    break;
                default: throw new ArgumentOutOfRangeException();
                }

                // Draw background
                //Render2D.FillRectangle(rect, backgroundColor);
                Render2D.DrawRectangle(rect, borderColor, 1.1f);

                // Draw fill
                Render2D.FillRectangle(fillArea.MakeOffsetted(rect.Location), backgroundColor);

                // Draw frame
                if (IsMouseOver)
                {
                    Render2D.DrawRectangle(rect, style.ProgressNormal.AlphaMultiplied(0.8f), 1.1f);
                }
                if (IsSelected)
                {
                    Render2D.DrawRectangle(rect, style.BackgroundSelected.AlphaMultiplied(0.8f), 1.1f);
                }

                // Draw pivot point
                if (SupportsShiftModulation && Input.GetKey(KeyboardKeys.Control))
                {
                    Vector2 pivotPoint;
                    switch (_presets)
                    {
                    case AnchorPresets.Custom:
                        pivotPoint = Vector2.Minimum;
                        break;
                    case AnchorPresets.TopLeft:
                        pivotPoint = new Vector2(0, 0);
                        break;
                    case AnchorPresets.TopCenter:
                    case AnchorPresets.HorizontalStretchTop:
                        pivotPoint = new Vector2(rect.Width / 2, 0);
                        break;
                    case AnchorPresets.TopRight:
                        pivotPoint = new Vector2(rect.Width, 0);
                        break;
                    case AnchorPresets.MiddleLeft:
                    case AnchorPresets.VerticalStretchLeft:
                        pivotPoint = new Vector2(0, rect.Height / 2);
                        break;
                    case AnchorPresets.MiddleCenter:
                    case AnchorPresets.VerticalStretchCenter:
                    case AnchorPresets.HorizontalStretchMiddle:
                    case AnchorPresets.StretchAll:
                        pivotPoint = new Vector2(rect.Width / 2, rect.Height / 2);
                        break;
                    case AnchorPresets.MiddleRight:
                    case AnchorPresets.VerticalStretchRight:
                        pivotPoint = new Vector2(rect.Width, rect.Height / 2);
                        break;
                    case AnchorPresets.BottomLeft:
                        pivotPoint = new Vector2(0, rect.Height);
                        break;
                    case AnchorPresets.BottomCenter:
                    case AnchorPresets.HorizontalStretchBottom:
                        pivotPoint = new Vector2(rect.Width / 2, rect.Height);
                        break;
                    case AnchorPresets.BottomRight:
                        pivotPoint = new Vector2(rect.Width, rect.Height);
                        break;
                    default: throw new ArgumentOutOfRangeException();
                    }
                    var pivotPointSize = new Vector2(3.0f);
                    Render2D.DrawRectangle(new Rectangle(pivotPoint - pivotPointSize * 0.5f, pivotPointSize), style.ProgressNormal, 1.1f);
                }
            }
        }

        /// <summary>
        /// Context menu for anchors presets editing.
        /// </summary>
        /// <seealso cref="FlaxEditor.GUI.ContextMenu.ContextMenuBase" />
        public sealed class AnchorPresetsEditorPopup : ContextMenuBase
        {
            const float ButtonsMargin = 10.0f;
            const float ButtonsMarginStretch = 8.0f;
            const float ButtonsSize = 32.0f;
            const float TitleHeight = 23.0f;
            const float InfoHeight = 23.0f;
            const float DialogWidth = ButtonsSize * 4 + ButtonsMargin * 5 + ButtonsMarginStretch;
            const float DialogHeight = TitleHeight + InfoHeight + ButtonsSize * 4 + ButtonsMargin * 5 + ButtonsMarginStretch;

            private readonly bool _supportsShiftModulation;

            /// <summary>
            /// Initializes a new instance of the <see cref="AnchorPresetsEditorPopup"/> class.
            /// </summary>
            /// <param name="presets">The initial value.</param>
            /// <param name="supportsShiftModulation">If the popup should react to shift</param>
            public AnchorPresetsEditorPopup(AnchorPresets presets, bool supportsShiftModulation = true)
            {
                _supportsShiftModulation = supportsShiftModulation;

                var style = FlaxEngine.GUI.Style.Current;
                Tag = presets;
                Size = new Vector2(DialogWidth, DialogHeight);

                // Title
                var title = new Label(2, 2, DialogWidth - 4, TitleHeight)
                {
                    Font = new FontReference(style.FontLarge),
                    Text = "Anchor Presets",
                    Parent = this
                };

                // Info
                var info = new Label(0, title.Bottom, DialogWidth, InfoHeight)
                {
                    Font = new FontReference(style.FontSmall),
                    Text = "Shift: also set bounds\nControl: also set pivot",
                    Parent = this
                };

                // Buttons
                var buttonsX = ButtonsMargin;
                var buttonsY = info.Bottom + ButtonsMargin;
                var buttonsSpacingX = ButtonsSize + ButtonsMargin;
                var buttonsSpacingY = ButtonsSize + ButtonsMargin;
                //
                AddButton(buttonsX + buttonsSpacingX * 0, buttonsY + buttonsSpacingY * 0, AnchorPresets.TopLeft);
                AddButton(buttonsX + buttonsSpacingX * 1, buttonsY + buttonsSpacingY * 0, AnchorPresets.TopCenter);
                AddButton(buttonsX + buttonsSpacingX * 2, buttonsY + buttonsSpacingY * 0, AnchorPresets.TopRight);
                AddButton(buttonsX + buttonsSpacingX * 3 + ButtonsMarginStretch, buttonsY + buttonsSpacingY * 0, AnchorPresets.HorizontalStretchTop);
                //
                AddButton(buttonsX + buttonsSpacingX * 0, buttonsY + buttonsSpacingY * 1, AnchorPresets.MiddleLeft);
                AddButton(buttonsX + buttonsSpacingX * 1, buttonsY + buttonsSpacingY * 1, AnchorPresets.MiddleCenter);
                AddButton(buttonsX + buttonsSpacingX * 2, buttonsY + buttonsSpacingY * 1, AnchorPresets.MiddleRight);
                AddButton(buttonsX + buttonsSpacingX * 3 + ButtonsMarginStretch, buttonsY + buttonsSpacingY * 1, AnchorPresets.HorizontalStretchMiddle);
                //
                AddButton(buttonsX + buttonsSpacingX * 0, buttonsY + buttonsSpacingY * 2, AnchorPresets.BottomLeft);
                AddButton(buttonsX + buttonsSpacingX * 1, buttonsY + buttonsSpacingY * 2, AnchorPresets.BottomCenter);
                AddButton(buttonsX + buttonsSpacingX * 2, buttonsY + buttonsSpacingY * 2, AnchorPresets.BottomRight);
                AddButton(buttonsX + buttonsSpacingX * 3 + ButtonsMarginStretch, buttonsY + buttonsSpacingY * 2, AnchorPresets.HorizontalStretchBottom);
                //
                AddButton(buttonsX + buttonsSpacingX * 0, buttonsY + buttonsSpacingY * 3 + ButtonsMarginStretch, AnchorPresets.VerticalStretchLeft);
                AddButton(buttonsX + buttonsSpacingX * 1, buttonsY + buttonsSpacingY * 3 + ButtonsMarginStretch, AnchorPresets.VerticalStretchCenter);
                AddButton(buttonsX + buttonsSpacingX * 2, buttonsY + buttonsSpacingY * 3 + ButtonsMarginStretch, AnchorPresets.VerticalStretchRight);
                AddButton(buttonsX + buttonsSpacingX * 3 + ButtonsMarginStretch, buttonsY + buttonsSpacingY * 3 + ButtonsMarginStretch, AnchorPresets.StretchAll);
            }

            private void AddButton(float x, float y, AnchorPresets presets)
            {
                var button = new AnchorButton
                {
                    Bounds = new Rectangle(x, y, ButtonsSize, ButtonsSize),
                    Parent = this,
                    Presets = presets,
                    IsSelected = presets == (AnchorPresets)Tag,
                    SupportsShiftModulation = _supportsShiftModulation,
                    Tag = presets,
                };
                button.ButtonClicked += OnButtonClicked;
            }

            private void OnButtonClicked(Button button)
            {
                Tag = button.Tag;
                Hide();
            }

            /// <inheritdoc />
            protected override void OnShow()
            {
                Focus();

                base.OnShow();
            }

            /// <inheritdoc />
            public override void Hide()
            {
                if (!Visible)
                    return;

                Focus(null);

                base.Hide();
            }

            /// <inheritdoc />
            public override bool OnKeyDown(KeyboardKeys key)
            {
                if (key == KeyboardKeys.Escape)
                {
                    Hide();
                    return true;
                }

                return base.OnKeyDown(key);
            }
        }

        private AnchorButton _button;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _button = layout.Custom<AnchorButton>().CustomControl;
            _button.Presets = (AnchorPresets)Values[0];
            _button.Clicked += OnButtonClicked;
        }

        private void OnButtonClicked()
        {
            var location = _button.Center + new Vector2(3.0f);
            var editor = new AnchorPresetsEditorPopup(_button.Presets, true);
            editor.VisibleChanged += OnEditorVisibleChanged;
            editor.Show(_button.Parent, location);
        }

        private void OnEditorVisibleChanged(Control control)
        {
            if (control.Visible)
                return;
            SetValue(control.Tag);
        }

        /// <inheritdoc/>
        protected override void SynchronizeValue(object value)
        {
            // Custom anchors editing for Control to handle bounds preservation via key modifiers
            if (ParentEditor != null)
            {
                var centerToPosition = Input.GetKey(KeyboardKeys.Shift);
                var setPivot = Input.GetKey(KeyboardKeys.Control);
                var editedAny = false;
                foreach (var parentValue in ParentEditor.Values)
                {
                    if (parentValue is Control parentControl)
                    {
                        parentControl.SetAnchorPreset((AnchorPresets)value, !centerToPosition, setPivot);
                        editedAny = true;
                    }
                }
                if (editedAny)
                    return;
            }

            base.SynchronizeValue(value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            _button.Presets = (AnchorPresets)Values[0];
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _button = null;

            base.Deinitialize();
        }
    }

    /// <summary>
    /// Dedicated custom editor for <see cref="UIControl.Control"/> object.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.Editors.GenericEditor" />
    public class UIControlControlEditor : GenericEditor
    {
        private Type _cachedType;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _cachedType = null;
            if (HasDifferentTypes)
            {
                // TODO: support stable editing multiple different control types (via generic way or for transform-only)
                return;
            }

            // Set control type button
            var space = layout.Space(20);
            float setTypeButtonWidth = 60.0f;
            var setTypeButton = new Button
            {
                TooltipText = "Sets the control to the given type",
                AnchorPreset = AnchorPresets.MiddleCenter,
                Text = "Set Type",
                Parent = space.Spacer,
                Bounds = new Rectangle((space.Spacer.Width - setTypeButtonWidth) / 2, 1, setTypeButtonWidth, 18),
            };
            setTypeButton.ButtonClicked += OnSetTypeButtonClicked;

            // Don't show editor if any control is invalid
            if (Values.HasNull)
            {
                var label = layout.Label("Select control type to create", TextAlignment.Center);
                label.Label.Enabled = false;
                return;
            }

            // Add control type helper label
            {
                var type = Values[0].GetType();
                _cachedType = type;
                var label = layout.AddPropertyItem("Type", "The type of the created control.");
                label.Label(type.FullName);
            }

            // Show control properties
            base.Initialize(layout);

            for (int i = 0; i < layout.Children.Count; i++)
            {
                if (layout.Children[i] is GroupElement group && group.Panel.HeaderText == "Transform")
                {
                    VerticalPanelElement mainHor = VerticalPanelWithoutMargin(group);
                    CreateTransformElements(mainHor, ValuesTypes);
                    group.ContainerControl.ChangeChildIndex(mainHor.Control, 0);
                    break;
                }
            }
        }

        private void CreateTransformElements(LayoutElementsContainer main, ScriptType[] valueTypes)
        {
            main.Space(10);
            HorizontalPanelElement sidePanel = main.HorizontalPanel();
            sidePanel.Panel.ClipChildren = false;

            ScriptMemberInfo anchorInfo = valueTypes[0].GetProperty("AnchorPreset");
            ItemInfo anchorItem = new ItemInfo(anchorInfo);
            sidePanel.Object(anchorItem.GetValues(Values));

            VerticalPanelElement group = VerticalPanelWithoutMargin(sidePanel);

            group.Panel.AnchorPreset = AnchorPresets.HorizontalStretchTop;
            group.Panel.Offsets = new Margin(100, 10, 0, 0);

            var horUp = UniformGridTwoByOne(group);
            horUp.CustomControl.Height = TextBoxBase.DefaultHeight;
            var horDown = UniformGridTwoByOne(group);
            horDown.CustomControl.Height = TextBoxBase.DefaultHeight;

            GetAnchorEquality(out _cachedXEq, out _cachedYEq, valueTypes);

            BuildLocationSizeOffsets(horUp, horDown, _cachedXEq, _cachedYEq, valueTypes);

            main.Space(10);
            BuildAnchorsDropper(main, valueTypes);
        }

        private void BuildAnchorsDropper(LayoutElementsContainer main, ScriptType[] valueTypes)
        {
            ScriptMemberInfo minInfo = valueTypes[0].GetProperty("AnchorMin");
            ScriptMemberInfo maxInfo = valueTypes[0].GetProperty("AnchorMax");
            ItemInfo minItem = new ItemInfo(minInfo);
            ItemInfo maxItem = new ItemInfo(maxInfo);

            GroupElement ng = main.Group("Anchors", true);
            ng.Panel.Close(false);
            ng.Property("Min", minItem.GetValues(Values));
            ng.Property("Max", maxItem.GetValues(Values));
        }

        private void GetAnchorEquality(out bool xEq, out bool yEq, ScriptType[] valueTypes)
        {
            ScriptMemberInfo minInfo = valueTypes[0].GetProperty("AnchorMin");
            ScriptMemberInfo maxInfo = valueTypes[0].GetProperty("AnchorMax");
            ItemInfo minItem = new ItemInfo(minInfo);
            ItemInfo maxItem = new ItemInfo(maxInfo);
            ValueContainer minVal = minItem.GetValues(Values);
            ValueContainer maxVal = maxItem.GetValues(Values);

            ItemInfo xItem = new ItemInfo(minInfo.ValueType.GetField("X"));
            ItemInfo yItem = new ItemInfo(minInfo.ValueType.GetField("Y"));

            xEq = xItem.GetValues(minVal).ToList().Any(xItem.GetValues(maxVal).ToList().Contains);
            yEq = yItem.GetValues(minVal).ToList().Any(yItem.GetValues(maxVal).ToList().Contains);
        }

        private void BuildLocationSizeOffsets(LayoutElementsContainer horUp, LayoutElementsContainer horDown, bool xEq, bool yEq, ScriptType[] valueTypes)
        {
            ScriptMemberInfo xInfo = valueTypes[0].GetProperty("LocalX");
            ItemInfo xItem = new ItemInfo(xInfo);
            ScriptMemberInfo yInfo = valueTypes[0].GetProperty("LocalY");
            ItemInfo yItem = new ItemInfo(yInfo);
            ScriptMemberInfo widthInfo = valueTypes[0].GetProperty("Width");
            ItemInfo widthItem = new ItemInfo(widthInfo);
            ScriptMemberInfo heightInfo = valueTypes[0].GetProperty("Height");
            ItemInfo heightItem = new ItemInfo(heightInfo);

            ScriptMemberInfo leftInfo = valueTypes[0].GetProperty("Proxy_Offset_Left", BindingFlags.Default | BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
            ItemInfo leftItem = new ItemInfo(leftInfo);
            ScriptMemberInfo rightInfo = valueTypes[0].GetProperty("Proxy_Offset_Right", BindingFlags.Default | BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
            ItemInfo rightItem = new ItemInfo(rightInfo);
            ScriptMemberInfo topInfo = valueTypes[0].GetProperty("Proxy_Offset_Top", BindingFlags.Default | BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
            ItemInfo topItem = new ItemInfo(topInfo);
            ScriptMemberInfo bottomInfo = valueTypes[0].GetProperty("Proxy_Offset_Bottom", BindingFlags.Default | BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
            ItemInfo bottomItem = new ItemInfo(bottomInfo);

            LayoutElementsContainer xEl;
            LayoutElementsContainer yEl;
            LayoutElementsContainer hEl;
            LayoutElementsContainer vEl;
            if (xEq)
            {
                xEl = UniformPanelCapsuleForObjectWithText(horUp, "X: ", xItem.GetValues(Values));
                vEl = UniformPanelCapsuleForObjectWithText(horDown, "Width: ", widthItem.GetValues(Values));
            }
            else
            {
                xEl = UniformPanelCapsuleForObjectWithText(horUp, "Left: ", leftItem.GetValues(Values));
                vEl = UniformPanelCapsuleForObjectWithText(horDown, "Right: ", rightItem.GetValues(Values));
            }
            if (yEq)
            {
                yEl = UniformPanelCapsuleForObjectWithText(horUp, "Y: ", yItem.GetValues(Values));
                hEl = UniformPanelCapsuleForObjectWithText(horDown, "Height: ", heightItem.GetValues(Values));
            }
            else
            {
                yEl = UniformPanelCapsuleForObjectWithText(horUp, "Top: ", topItem.GetValues(Values));
                hEl = UniformPanelCapsuleForObjectWithText(horDown, "Bottom: ", bottomItem.GetValues(Values));
            }
            xEl.Control.AnchorMin = new Vector2(0, xEl.Control.AnchorMin.Y);
            xEl.Control.AnchorMax = new Vector2(0.5f, xEl.Control.AnchorMax.Y);

            vEl.Control.AnchorMin = new Vector2(0, xEl.Control.AnchorMin.Y);
            vEl.Control.AnchorMax = new Vector2(0.5f, xEl.Control.AnchorMax.Y);

            yEl.Control.AnchorMin = new Vector2(0.5f, xEl.Control.AnchorMin.Y);
            yEl.Control.AnchorMax = new Vector2(1, xEl.Control.AnchorMax.Y);

            hEl.Control.AnchorMin = new Vector2(0.5f, xEl.Control.AnchorMin.Y);
            hEl.Control.AnchorMax = new Vector2(1, xEl.Control.AnchorMax.Y);
        }

        private VerticalPanelElement VerticalPanelWithoutMargin(LayoutElementsContainer cont)
        {
            var horUp = cont.VerticalPanel();
            horUp.Panel.Margin = Margin.Zero;
            return horUp;
        }

        private CustomElementsContainer<UniformGridPanel> UniformGridTwoByOne(LayoutElementsContainer cont)
        {
            var horUp = cont.CustomContainer<UniformGridPanel>();
            horUp.CustomControl.SlotsHorizontally = 2;
            horUp.CustomControl.SlotsVertically = 1;
            horUp.CustomControl.SlotPadding = Margin.Zero;
            horUp.CustomControl.ClipChildren = false;
            return horUp;
        }

        private CustomElementsContainer<UniformGridPanel> UniformPanelCapsuleForObjectWithText(LayoutElementsContainer el, string text, ValueContainer values)
        {
            CustomElementsContainer<UniformGridPanel> hor = UniformGridTwoByOne(el);
            hor.CustomControl.SlotPadding = new Margin(5, 5, 0, 0);
            LabelElement lab = hor.Label(text);
            hor.Object(values);
            return hor;
        }

        private bool _cachedXEq;
        private bool _cachedYEq;

        /// <inheritdoc />
        public override void Refresh()
        {
            if (_cachedType != null)
            {
                // Automatic layout rebuild if control type gets changed
                var type = Values.HasNull ? null : Values[0].GetType();
                if (type != _cachedType)
                {
                    RebuildLayout();
                    return;
                }

                // Refresh anchors
                GetAnchorEquality(out bool xEq, out bool yEq, ValuesTypes);
                if (xEq != _cachedXEq || yEq != _cachedYEq)
                {
                    RebuildLayout();
                }
            }

            base.Refresh();
        }

        private void OnSetTypeButtonClicked(Button button)
        {
            var controlTypes = Editor.Instance.CodeEditing.Controls.Get();
            if (controlTypes.Count == 0)
                return;

            // Show context menu with list of controls to add
            var cm = new ItemsListContextMenu(180);
            for (int i = 0; i < controlTypes.Count; i++)
            {
                cm.AddItem(new TypeSearchPopup.TypeItemView(controlTypes[i]));
            }
            cm.ItemClicked += controlType => SetType((ScriptType)controlType.Tag);
            cm.SortChildren();
            cm.Show(button.Parent, button.BottomLeft);
        }

        private void SetType(ref ScriptType controlType, UIControl uiControl)
        {
            string previousName = uiControl.Control?.GetType().Name ?? nameof(UIControl);
            uiControl.Control = (Control)controlType.CreateInstance();
            if (uiControl.Name.StartsWith(previousName))
            {
                string newName = controlType.Name + uiControl.Name.Substring(previousName.Length);
                uiControl.Name = StringUtils.IncrementNameNumber(newName, x => uiControl.Parent.GetChild(x) == null);
            }
        }

        private void SetType(ScriptType controlType)
        {
            var uiControls = ParentEditor.Values;
            if (Presenter.Undo?.Enabled ?? false)
            {
                using (new UndoMultiBlock(Presenter.Undo, uiControls, "Set Control Type"))
                {
                    for (int i = 0; i < uiControls.Count; i++)
                        SetType(ref controlType, (UIControl)uiControls[i]);
                }
            }
            else
            {
                for (int i = 0; i < uiControls.Count; i++)
                    SetType(ref controlType, (UIControl)uiControls[i]);
            }

            ParentEditor.RebuildLayout();
        }
    }
}
