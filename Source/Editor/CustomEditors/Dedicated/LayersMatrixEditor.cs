// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Content.Settings;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for layers matrix editor. Used to define layer-based collision detection for <see cref="PhysicsSettings.LayerMasks"/>
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.CustomEditor" />
    public sealed class LayersMatrixEditor : CustomEditor
    {
        private int _layersCount;
        private List<CheckBox> _checkBoxes;
        private VerticalPanel _upperRightCell;
        private VerticalPanel _bottomLeftCell;
        private UniformGridPanel _grid;
        private Border _horizontalHighlight;
        private Border _verticalHighlight;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.InlineIntoParent;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            string[] layerNames = LayersAndTagsSettings.GetCurrentLayers();
            int layersCount = layerNames.Length;
            _checkBoxes = new List<CheckBox>();
            _layersCount = layersCount;

            float labelsWidth = 100.0f;
            float labelsHeight = 18;

            var panel = layout.Space(100).Spacer;
            var gridPanel = new GridPanel(0)
            {
                Parent = panel,
            };

            var style = FlaxEngine.GUI.Style.Current;
            _horizontalHighlight = new Border()
            {
                Parent = panel,
                BorderColor = style.Foreground,
                BorderWidth = 1.0f,
                Visible = false,
            };

            _verticalHighlight = new Border()
            {
                Parent = panel,
                BorderColor = style.Foreground,
                BorderWidth = 1.0f,
                Visible = false,
            };

            var upperLeftCell = new Label
            {
                Parent = gridPanel,
            };

            _upperRightCell = new VerticalPanel
            {
                ClipChildren = false,
                Pivot = new Float2(0.00001f, 0.0f),
                Offset = new Float2(-labelsWidth, 0),
                Rotation = -90,
                Spacing = 0,
                TopMargin = 0,
                BottomMargin = 0,
                Parent = gridPanel,
            };

            _bottomLeftCell = new VerticalPanel
            {
                Pivot = Float2.Zero,
                Spacing = 0,
                TopMargin = 0,
                BottomMargin = 0,
                Parent = gridPanel,
            };

            _grid = new UniformGridPanel(0)
            {
                SlotsHorizontally = layersCount,
                SlotsVertically = layersCount,
                Parent = gridPanel,
            };

            // Set layer names
            int layerIndex = 0;
            for (; layerIndex < layerNames.Length; layerIndex++)
            {
                _upperRightCell.AddChild(new Label
                {
                    Height = labelsHeight,
                    Text = layerNames[layerNames.Length - layerIndex - 1],
                    HorizontalAlignment = TextAlignment.Near,
                });
                _bottomLeftCell.AddChild(new Label
                {
                    Height = labelsHeight,
                    Text = layerNames[layerIndex],
                    HorizontalAlignment = TextAlignment.Far,
                });
            }
            for (; layerIndex < layersCount; layerIndex++)
            {
                string name = "Layer " + layerIndex;
                _upperRightCell.AddChild(new Label
                {
                    Height = labelsHeight,
                    Text = name,
                    HorizontalAlignment = TextAlignment.Near,
                });
                _bottomLeftCell.AddChild(new Label
                {
                    Height = labelsHeight,
                    Text = name,
                    HorizontalAlignment = TextAlignment.Far,
                });
            }

            // Arrange
            panel.Height = gridPanel.Height = gridPanel.Width = labelsWidth + layersCount * 18;
            gridPanel.RowFill[0] = gridPanel.ColumnFill[0] = labelsWidth / gridPanel.Width;
            gridPanel.RowFill[1] = gridPanel.ColumnFill[1] = 1 - gridPanel.ColumnFill[0];

            // Create matrix
            for (int row = 0; row < layersCount; row++)
            {
                int column = 0;
                for (; column < layersCount - row; column++)
                {
                    var box = new CheckBox(0, 0, true)
                    {
                        Tag = new Float2(_layersCount - column - 1, row),
                        Parent = _grid,
                        Checked = GetBit(column, row),
                    };
                    box.StateChanged += OnCheckBoxChanged;
                    _checkBoxes.Add(box);
                }
                for (; column < layersCount; column++)
                {
                    _grid.AddChild(new Label());
                }
            }
        }

        private void OnCheckBoxChanged(CheckBox box)
        {
            int column = (int)((Float2)box.Tag).X;
            int row = (int)((Float2)box.Tag).Y;
            SetBit(column, row, box.Checked);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            int selectedColumn = -1;
            int selectedRow = -1;
            var style = FlaxEngine.GUI.Style.Current;
            bool mouseOverGrid = _grid.IsMouseOver;

            // Only hide highlights if mouse is not over the grid to reduce flickering
            if (!mouseOverGrid)
            {
                _horizontalHighlight.Visible = false;
                _verticalHighlight.Visible = false;
            }

            // Sync check boxes
            for (int i = 0; i < _checkBoxes.Count; i++)
            {
                var box = _checkBoxes[i];
                int column = (int)((Float2)box.Tag).X;
                int row = (int)((Float2)box.Tag).Y;
                box.Checked = GetBit(column, row);
                                
                if (box.IsMouseOver)
                {
                    selectedColumn = column;
                    selectedRow = row;

                    _horizontalHighlight.X = _grid.X - _bottomLeftCell.Width;
                    _horizontalHighlight.Y = _grid.Y + box.Y;
                    _horizontalHighlight.Width = _bottomLeftCell.Width + box.Width + box.X;
                    _horizontalHighlight.Height = box.Height;
                    _horizontalHighlight.Visible = true;

                    _verticalHighlight.X = _grid.X + box.X;
                    _verticalHighlight.Y = _grid.Y - _upperRightCell.Height;
                    _verticalHighlight.Width = box.Width;
                    _verticalHighlight.Height = _upperRightCell.Height + box.Height + box.Y;
                    _verticalHighlight.Visible = true;
                }
            }

            for (int i = 0; i < _checkBoxes.Count; i++)
            {
                var box = _checkBoxes[i];
                int column = (int)((Float2)box.Tag).X;
                int row = (int)((Float2)box.Tag).Y;

                if (!mouseOverGrid)
                    box.ImageColor = style.BorderSelected;
                else if (selectedColumn > -1 && selectedRow > -1)
                {
                    bool isRowOrColumn = column == selectedColumn || row == selectedRow;
                    box.ImageColor = style.BorderSelected * (isRowOrColumn ? 1.2f : 0.75f);
                }
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            base.Deinitialize();

            _checkBoxes.Clear();
            _checkBoxes = null;
        }

        private bool GetBit(int column, int row)
        {
            var values = (int[])Values[0];
            var mask = 1 << column;
            return (values[row] & mask) != 0;
        }

        private void SetBit(int column, int row, bool flag)
        {
            var values = (int[])((int[])Values[0]).Clone();

            values[row] = SetMaskBit(values[row], 1 << column, flag);
            values[column] = SetMaskBit(values[column], 1 << row, flag);

            SetValue(values);
        }

        private static int SetMaskBit(int value, int mask, bool flag)
        {
            return (value & ~mask) | (flag ? mask : 0);
        }
    }
}
