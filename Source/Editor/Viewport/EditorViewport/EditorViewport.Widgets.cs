using FlaxEditor.Content.Settings;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.Viewport.Widgets;
using FlaxEngine;
using FlaxEngine.GUI;
using Newtonsoft.Json;
using JsonSerializer = FlaxEngine.Json.JsonSerializer;

namespace FlaxEditor.Viewport
{
    public partial class EditorViewport
    {
        private FPSCounter _fpsCounter;
        private ContextMenuButton _showFpsButon;

        /// <summary>
        /// Gets or sets a value indicating whether show or hide FPS counter.
        /// </summary>
        public bool ShowFpsCounter
        {
            get
            {
                return _fpsCounter.Visible;
            }
            set
            {
                _fpsCounter.Visible = value;
                _fpsCounter.Enabled = value;
                _showFpsButon.Icon = (value ? Style.Current.CheckBoxTick : SpriteHandle.Invalid);
            }
        }

        /// <summary>
        /// The 'View' widget button context menu.
        /// </summary>
        public ContextMenu ViewWidgetButtonMenu;

        /// <summary>
        /// The 'View' widget 'Show' category context menu.
        /// </summary>
        public ContextMenu ViewWidgetShowMenu;

        /// <summary>
        /// The speed widget button.
        /// </summary>
        protected ViewportWidgetButton _speedWidget;

        /// <summary>
        /// caled once in constructor to create the Widgets
        /// </summary>
        protected virtual void ConstructWidgets()
        {
            var largestText = "Invert Panning";
            var textSize = Style.Current.FontMedium.MeasureText(largestText);
            var xLocationForExtras = textSize.X + 5;

            // Camera speed widget
            var camSpeed = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var camSpeedCM = new ContextMenu();
            var camSpeedButton = new ViewportWidgetButton(MovementSpeed.ToString(), Editor.Instance.Icons.CamSpeed32, camSpeedCM)
            {
                Tag = this,
                TooltipText = "Camera speed scale"
            };
            _speedWidget = camSpeedButton;
            //for (int i = 0; i < EditorViewportCameraSpeedValues.Length; i++)
            //{
            //    var v = EditorViewportCameraSpeedValues[i];
            //    var button = camSpeedCM.AddButton(v.ToString());
            //    button.Tag = v;
            //}
            camSpeedCM.ButtonClicked += button => MovementSpeed = (float)button.Tag;
            camSpeedCM.VisibleChanged += WidgetCamSpeedShowHide;
            camSpeedButton.Parent = camSpeed;
            camSpeed.Parent = this;

            // View mode widget
            var viewMode = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperLeft);
            ViewWidgetButtonMenu = new ContextMenu();
            var viewModeButton = new ViewportWidgetButton("View", SpriteHandle.Invalid, ViewWidgetButtonMenu)
            {
                TooltipText = "View properties",
                Parent = viewMode
            };
            viewMode.Parent = this;

            // Show
            {
                ViewWidgetShowMenu = ViewWidgetButtonMenu.AddChildMenu("Show").ContextMenu;

                // Show FPS
                {
                    _fpsCounter = new FPSCounter(10f, 32f)
                    {
                        Visible = false,
                        Enabled = false,
                        Parent = this,
                        ShowAvrage = true,
                        ShowMinMax = true,
                        SnapshotCount = 30
                    };
                    _showFpsButon = ViewWidgetShowMenu.AddButton("FPS Counter", () => ShowFpsCounter = !ShowFpsCounter);
                    _showFpsButon.CloseMenuOnClick = false;
                }
            }

            // View Layers
            {
                var viewLayers = ViewWidgetButtonMenu.AddChildMenu("View Layers").ContextMenu;
                viewLayers.AddButton("Copy layers", () => Clipboard.Text = JsonSerializer.Serialize(Task.View.RenderLayersMask));
                viewLayers.AddButton("Paste layers", () =>
                {
                    try
                    {
                        Task.ViewLayersMask = JsonSerializer.Deserialize<LayersMask>(Clipboard.Text);
                    }
                    catch
                    {
                    }
                });
                viewLayers.AddButton("Reset layers", () => Task.ViewLayersMask = LayersMask.Default).Icon = Editor.Instance.Icons.Rotate32;
                viewLayers.AddButton("Disable layers", () => Task.ViewLayersMask = new LayersMask(0)).Icon = Editor.Instance.Icons.Rotate32;
                viewLayers.AddSeparator();
                var layers = LayersAndTagsSettings.GetCurrentLayers();
                if (layers != null && layers.Length > 0)
                {
                    for (int i = 0; i < layers.Length; i++)
                    {
                        var layer = layers[i];
                        var button = viewLayers.AddButton(layer);
                        button.CloseMenuOnClick = false;
                        button.Tag = 1 << i;
                    }
                }
                viewLayers.ButtonClicked += button =>
                {
                    if (button.Tag != null)
                    {
                        int layerIndex = (int)button.Tag;
                        LayersMask mask = new LayersMask(layerIndex);
                        RenderView view = Task.View;
                        view.RenderLayersMask ^= mask;
                        Task.View = view;
                        button.Icon = (Task.View.RenderLayersMask & mask) != 0 ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                    }
                };
                viewLayers.VisibleChanged += WidgetViewLayersShowHide;
            }

            // View Flags
            {
                var viewFlags = ViewWidgetButtonMenu.AddChildMenu("View Flags").ContextMenu;
                viewFlags.AddButton("Copy flags", () => Clipboard.Text = JsonSerializer.Serialize(Task.ViewFlags));
                viewFlags.AddButton("Paste flags", () =>
                {
                    try
                    {
                        Task.ViewFlags = JsonSerializer.Deserialize<ViewFlags>(Clipboard.Text);
                    }
                    catch
                    {
                    }
                });
                viewFlags.AddButton("Reset flags", () => Task.ViewFlags = ViewFlags.DefaultEditor).Icon = Editor.Instance.Icons.Rotate32;
                viewFlags.AddButton("Disable flags", () => Task.ViewFlags = ViewFlags.None).Icon = Editor.Instance.Icons.Rotate32;
                viewFlags.AddSeparator();
                for (int i = 0; i < EditorViewportViewFlagsValues.Length; i++)
                {
                    var v = EditorViewportViewFlagsValues[i];
                    var button = viewFlags.AddButton(v.Name);
                    button.CloseMenuOnClick = false;
                    button.Tag = v.Mode;
                }
                viewFlags.ButtonClicked += button =>
                {
                    if (button.Tag != null)
                    {
                        var v = (ViewFlags)button.Tag;
                        Task.ViewFlags ^= v;
                        button.Icon = (Task.View.Flags & v) != 0 ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                    }
                };
                viewFlags.VisibleChanged += WidgetViewFlagsShowHide;
            }

            // Debug View
            {
                var debugView = ViewWidgetButtonMenu.AddChildMenu("Debug View").ContextMenu;
                debugView.AddButton("Copy view", () => Clipboard.Text = JsonSerializer.Serialize(Task.ViewMode));
                debugView.AddButton("Paste view", () =>
                {
                    try
                    {
                        Task.ViewMode = JsonSerializer.Deserialize<ViewMode>(Clipboard.Text);
                    }
                    catch
                    {
                    }
                });
                debugView.AddSeparator();
                for (int i = 0; i < EditorViewportViewModeValues.Length; i++)
                {
                    ref var v = ref EditorViewportViewModeValues[i];
                    if (v.Options != null)
                    {
                        var childMenu = debugView.AddChildMenu(v.Name).ContextMenu;
                        childMenu.ButtonClicked += WidgetViewModeShowHideClicked;
                        childMenu.VisibleChanged += WidgetViewModeShowHide;
                        for (int j = 0; j < v.Options.Length; j++)
                        {
                            ref var vv = ref v.Options[j];
                            var button = childMenu.AddButton(vv.Name);
                            button.Tag = vv.Mode;
                        }
                    }
                    else
                    {
                        var button = debugView.AddButton(v.Name);
                        button.Tag = v.Mode;
                    }
                }
                debugView.ButtonClicked += WidgetViewModeShowHideClicked;
                debugView.VisibleChanged += WidgetViewModeShowHide;
            }

            ViewWidgetButtonMenu.AddSeparator();

            // Orthographic
            {
                var ortho = ViewWidgetButtonMenu.AddButton("Orthographic");
                ortho.CloseMenuOnClick = false;
                var orthoValue = new CheckBox(xLocationForExtras, 2, Camera.UseOrthographicProjection)
                {
                    Parent = ortho
                };
                orthoValue.StateChanged += checkBox =>
                {
                    if (checkBox.Checked != Camera.UseOrthographicProjection)
                    {
                        Camera.UseOrthographicProjection = checkBox.Checked;
                        ViewWidgetButtonMenu.Hide();
                        if (Camera.UseOrthographicProjection)
                        {
                            var orient = Camera.Orientation;
                            OrientViewport(ref orient);
                        }
                    }
                };
                ViewWidgetButtonMenu.VisibleChanged += control => orthoValue.Checked = Camera.UseOrthographicProjection;
            }

            // Camera Viewpoints
            {
                var cameraView = ViewWidgetButtonMenu.AddChildMenu("Viewpoints").ContextMenu;
                for (int i = 0; i < EditorViewportCameraViewpointValues.Length; i++)
                {
                    var co = EditorViewportCameraViewpointValues[i];
                    var button = cameraView.AddButton(co.Name);
                    button.Tag = co.Orientation;
                }
                cameraView.ButtonClicked += button =>
                {
                    var orient = Quaternion.Euler((Float3)button.Tag);
                    OrientViewport(ref orient);
                };
            }

            // Field of View
            {
                var fov = ViewWidgetButtonMenu.AddButton("Field Of View");
                fov.CloseMenuOnClick = false;
                var fovValue = new FloatValueBox(1, xLocationForExtras, 2, 70.0f, 35.0f, 160.0f, 0.1f)
                {
                    Parent = fov
                };

                fovValue.ValueChanged += () => Camera.FieldOfView = fovValue.Value;
                ViewWidgetButtonMenu.VisibleChanged += control =>
                {
                    fov.Visible = !Camera.UseOrthographicProjection;
                    fovValue.Value = Camera.FieldOfView;
                };
            }

            // Ortho Scale
            {
                var orthoSize = ViewWidgetButtonMenu.AddButton("Ortho Scale");
                orthoSize.CloseMenuOnClick = false;
                var orthoSizeValue = new FloatValueBox(Camera.OrthographicScale, xLocationForExtras, 2, 70.0f, 0.001f, 100000.0f, 0.01f)
                {
                    Parent = orthoSize
                };

                orthoSizeValue.ValueChanged += () => Camera.SetOrthographicScale(orthoSizeValue.Value);
                ViewWidgetButtonMenu.VisibleChanged += control =>
                {
                    orthoSize.Visible = Camera.UseOrthographicProjection;
                    orthoSizeValue.Value = Camera.OrthographicScale;
                };
            }

            // Near Plane
            {
                var nearPlane = ViewWidgetButtonMenu.AddButton("Near Plane");
                nearPlane.CloseMenuOnClick = false;
                var nearPlaneValue = new FloatValueBox(2.0f, 90, 2, 70.0f, 0.001f, 1000.0f)
                {
                    Parent = nearPlane
                };
                nearPlaneValue.ValueChanged += () => Camera.NearPlane = nearPlaneValue.Value;
                ViewWidgetButtonMenu.VisibleChanged += control => nearPlaneValue.Value = Camera.NearPlane;
            }

            // Far Plane
            {
                var farPlane = ViewWidgetButtonMenu.AddButton("Far Plane");
                farPlane.CloseMenuOnClick = false;
                var farPlaneValue = new FloatValueBox(1000, xLocationForExtras, 2, 70.0f, 10.0f)
                {
                    Parent = farPlane
                };
                farPlaneValue.ValueChanged += () => Camera.FarPlane = farPlaneValue.Value;
                ViewWidgetButtonMenu.VisibleChanged += control => farPlaneValue.Value = Camera.FarPlane;
            }

            // Brightness
            {
                var brightness = ViewWidgetButtonMenu.AddButton("Brightness");
                brightness.CloseMenuOnClick = false;
                var brightnessValue = new FloatValueBox(1.0f, xLocationForExtras, 2, 70.0f, 0.001f, 10.0f, 0.001f)
                {
                    Parent = brightness
                };
                brightnessValue.ValueChanged += () => Brightness = brightnessValue.Value;
                ViewWidgetButtonMenu.VisibleChanged += control => brightnessValue.Value = Brightness;
            }

            // Resolution
            {
                var resolution = ViewWidgetButtonMenu.AddButton("Resolution");
                resolution.CloseMenuOnClick = false;
                var resolutionValue = new FloatValueBox(1.0f, xLocationForExtras, 2, 70.0f, 0.1f, 4.0f, 0.001f)
                {
                    Parent = resolution
                };
                resolutionValue.ValueChanged += () => ResolutionScale = resolutionValue.Value;
                ViewWidgetButtonMenu.VisibleChanged += control => resolutionValue.Value = ResolutionScale;
            }

            // Invert Panning
            {
                var invert = ViewWidgetButtonMenu.AddButton("Invert Panning");
                invert.CloseMenuOnClick = false;
                var invertValue = new CheckBox(xLocationForExtras, 2, Camera.InvertPanning)
                {
                    Parent = invert
                };

                invertValue.StateChanged += checkBox =>
                {
                    if (checkBox.Checked != Camera.InvertPanning)
                    {
                        Camera.InvertPanning = checkBox.Checked;
                    }
                };
                ViewWidgetButtonMenu.VisibleChanged += control => invertValue.Checked = Camera.InvertPanning;
            }
        }
        private void WidgetViewLayersShowHide(Control cm)
        {
            if (!cm.Visible)
                return;

            var ccm = (ContextMenu)cm;
            foreach (var e in ccm.Items)
            {
                if (e is ContextMenuButton b && b != null && b.Tag != null)
                {
                    int layerIndex = (int)b.Tag;
                    LayersMask mask = new LayersMask(layerIndex);
                    b.Icon = (Task.View.RenderLayersMask & mask) != 0
                             ? Style.Current.CheckBoxTick
                             : SpriteHandle.Invalid;
                }
            }
        }

        private void WidgetCamSpeedShowHide(Control cm)
        {
            if (cm.Visible)
            {
                ContextMenu ccm = (ContextMenu)cm;
                foreach (ContextMenuItem e in ccm.Items)
                {
                    ContextMenuButton b = e as ContextMenuButton;
                    if (b != null)
                    {
                        float v = (float)b.Tag;
                        b.Icon = ((Mathf.Abs(MovementSpeed - v) < 0.001f) ? Style.Current.CheckBoxTick : SpriteHandle.Invalid);
                    }
                }
            }
        }

        private void WidgetViewModeShowHideClicked(ContextMenuButton button)
        {
            object tag = button.Tag;
            if (tag is ViewMode)
            {
                Task.ViewMode = (ViewMode)tag;
            }
        }

        private void WidgetViewModeShowHide(Control cm)
        {
            if (!cm.Visible)
            {
                ContextMenu ccm = (ContextMenu)cm;
                foreach (ContextMenuItem e in ccm.Items)
                {
                    ContextMenuButton b = e as ContextMenuButton;
                    if (b != null)
                    {
                        if (b.Tag is ViewMode)
                        {
                            b.Icon = ((Task.View.Mode == (ViewMode)b.Tag) ? Style.Current.CheckBoxTick : SpriteHandle.Invalid);
                        }
                    }
                }
            }
        }

        private void WidgetViewFlagsShowHide(Control cm)
        {
            if (!cm.Visible)
            {
                ContextMenu ccm = (ContextMenu)cm;
                foreach (ContextMenuItem e in ccm.Items)
                {
                    ContextMenuButton b = e as ContextMenuButton;
                    if (b != null && b.Tag != null)
                    {
                        b.Icon = (((Task.View.Flags & (ViewFlags)b.Tag) != ViewFlags.None) ? Style.Current.CheckBoxTick : SpriteHandle.Invalid);
                    }
                }
            }
        }
    }
}
