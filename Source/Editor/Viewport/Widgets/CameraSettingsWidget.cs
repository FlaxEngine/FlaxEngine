using System.Collections.Generic;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport.Widgets
{

    public class CameraSettingsWidget : ViewportWidget
    {
        /// <summary>
        /// The camera settings widget.
        /// </summary>
        protected ViewportWidgetsContainer _cameraWidget;

        /// <summary>
        /// The camera settings widget button.
        /// </summary>
        protected ViewportWidgetButton _cameraButton;

        /// <summary>
        /// The orthographic mode widget button.
        /// </summary>
        protected ViewportWidgetButton _orthographicProjectionButton;

        /// <summary>
        /// Format of the text for the camera move speed.
        /// </summary>
        private string MovementSpeedTextFormat
        {
            get
            {
                if (Mathf.Abs(Viewport.MovementSpeed - Viewport.MaxMovementSpeed) < Mathf.Epsilon || Mathf.Abs(Viewport.MovementSpeed - Viewport.MinMovementSpeed) < Mathf.Epsilon)
                    return "{0:0.##}";
                if (Viewport.MovementSpeed < 10.0f)
                    return "{0:0.00}";
                if (Viewport.MovementSpeed < 100.0f)
                    return "{0:0.0}";
                return "{0:#}";
            }
        }

        public CameraSettingsWidget(float x, float y, EditorViewport viewport)
        : base(x, y, 64, 32, viewport)
        {
        }

        protected override void OnAttach()
        {
            //todo
            var largestText = "Relative Panning";
            var textSize = Style.Current.FontMedium.MeasureText(largestText);
            var xLocationForExtras = textSize.X + 5;
            var cameraSpeedTextWidth = Style.Current.FontMedium.MeasureText("0.00").X;

            // Camera Settings Widget
            _cameraWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);

            // Camera Settings Menu
            var cameraCM = new ContextMenu();
            _cameraButton = new ViewportWidgetButton(string.Format(MovementSpeedTextFormat, Viewport.MovementSpeed), Editor.Instance.Icons.Camera64, cameraCM, false, cameraSpeedTextWidth)
            {
                Tag = this,
                TooltipText = "Camera Settings",
                Parent = _cameraWidget
            };
            _cameraWidget.Parent = Viewport;

            // Orthographic/Perspective Mode Widget
            _orthographicProjectionButton = new ViewportWidgetButton(string.Empty, Editor.Instance.Icons.CamSpeed32, null, true)
            {
                Checked = !Viewport.OrthographicProjection,
                TooltipText = "Toggle Orthographic/Perspective Mode",
                Parent = _cameraWidget
            };
            _orthographicProjectionButton.Toggled += (sender) =>
                {
                    Viewport.OrthographicProjection = !Viewport.OrthographicProjection;
                    _orthographicProjectionButton.Checked = !Viewport.OrthographicProjection;
                };

            // Camera Speed
            var camSpeedButton = cameraCM.AddButton("Camera Speed");
            camSpeedButton.CloseMenuOnClick = false;
            var camSpeedValue = new FloatValueBox(Viewport.MovementSpeed, xLocationForExtras, 2, 70.0f, Viewport.MinMovementSpeed, Viewport.MaxMovementSpeed, 0.5f)
            {
                Parent = camSpeedButton
            };

            camSpeedValue.ValueChanged += () =>
                    {
                        Viewport.MovementSpeed = camSpeedValue.Value;
                        if (_cameraButton != null)
                            _cameraButton.Text = string.Format(MovementSpeedTextFormat, Viewport.MovementSpeed);
                    };
            cameraCM.VisibleChanged += control => camSpeedValue.Value = Viewport.MovementSpeed;

            // Minimum & Maximum Camera Speed
            var minCamSpeedButton = cameraCM.AddButton("Min Cam Speed");
            minCamSpeedButton.CloseMenuOnClick = false;
            var minCamSpeedValue = new FloatValueBox(Viewport.MinMovementSpeed, xLocationForExtras, 2, 70.0f, 0.05f, Viewport.MaxMovementSpeed, 0.5f)
            {
                Parent = minCamSpeedButton
            };
            var maxCamSpeedButton = cameraCM.AddButton("Max Cam Speed");
            maxCamSpeedButton.CloseMenuOnClick = false;
            var maxCamSpeedValue = new FloatValueBox(Viewport.MaxMovementSpeed, xLocationForExtras, 2, 70.0f, Viewport.MinMovementSpeed, 1000.0f, 0.5f)
            {
                Parent = maxCamSpeedButton
            };

            minCamSpeedValue.ValueChanged += () =>
            {
                Viewport.MinMovementSpeed = minCamSpeedValue.Value;
                minCamSpeedValue.Value = Viewport.MinMovementSpeed;
            };
            cameraCM.VisibleChanged += control => minCamSpeedValue.Value = Viewport.MinMovementSpeed;

            maxCamSpeedValue.ValueChanged += () =>
            {
                Viewport.MaxMovementSpeed = maxCamSpeedValue.Value;
                maxCamSpeedValue.Value = Viewport.MaxMovementSpeed;
            };
            cameraCM.VisibleChanged += control => maxCamSpeedValue.Value = Viewport.MaxMovementSpeed;

            // Camera Easing
            {
                var useCameraEasing = cameraCM.AddButton("Camera Easing");
                useCameraEasing.CloseMenuOnClick = false;
                var useCameraEasingValue = new CheckBox(xLocationForExtras, 2, Viewport.UseCameraEasing)
                {
                    Parent = useCameraEasing
                };

                useCameraEasingValue.StateChanged += checkBox => { Viewport.UseCameraEasing = !Viewport.UseCameraEasing; };
                cameraCM.VisibleChanged += control => useCameraEasingValue.Checked = Viewport.UseCameraEasing;
            }

            // Panning Speed
            {
                var panningSpeed = cameraCM.AddButton("Panning Speed");
                panningSpeed.CloseMenuOnClick = false;
                var panningSpeedValue = new FloatValueBox(Viewport.PanningSpeed, xLocationForExtras, 2, 70.0f, 0.01f, 128.0f, 0.1f)
                {
                    Parent = panningSpeed
                };

                panningSpeedValue.ValueChanged += () => { Viewport.PanningSpeed = panningSpeedValue.Value; };//OnPanningSpeedChanged(panningSpeedValue);
                cameraCM.VisibleChanged += control =>
                {
                    panningSpeed.Visible = !Viewport.RelativePanning;
                    panningSpeedValue.Value = Viewport.PanningSpeed;
                };
            }

            // Relative Panning
            {
                var relativePanning = cameraCM.AddButton("Relative Panning");
                relativePanning.CloseMenuOnClick = false;
                var relativePanningValue = new CheckBox(xLocationForExtras, 2, Viewport.RelativePanning)
                {
                    Parent = relativePanning
                };

                relativePanningValue.StateChanged += checkBox =>
                {
                    Viewport.RelativePanning = checkBox.Checked;
                    cameraCM.Hide();
                };
                cameraCM.VisibleChanged += control => relativePanningValue.Checked = Viewport.RelativePanning;
            }

            // Invert Panning
            {
                var invertPanning = cameraCM.AddButton("Invert Panning");
                invertPanning.CloseMenuOnClick = false;
                var invertPanningValue = new CheckBox(xLocationForExtras, 2, Viewport.InvertPanning)
                {
                    Parent = invertPanning
                };

                invertPanningValue.StateChanged += checkBox => { Viewport.InvertPanning = !Viewport.InvertPanning; };
                cameraCM.VisibleChanged += control => invertPanningValue.Checked = Viewport.InvertPanning;
            }

            cameraCM.AddSeparator();

            // Camera Viewpoints
            {
                var cameraView = cameraCM.AddChildMenu("Viewpoints").ContextMenu;
                foreach (var (key, value) in EditorViewport.CameraViewpoints)
                {
                    var button = cameraView.AddButton(key.ToString());
                    button.Tag = value;
                }

                cameraView.ButtonClicked += (button) =>
                {
                    Viewport.OrientViewport(Quaternion.Euler((Float3)button.Tag));
                };
            }

            // Orthographic Mode
            {
                var ortho = cameraCM.AddButton("Orthographic");
                ortho.CloseMenuOnClick = false;
                var orthoValue = new CheckBox(xLocationForExtras, 2, Viewport.OrthographicProjection)
                {
                    Parent = ortho
                };

                orthoValue.StateChanged += checkBox =>
                {
                    Viewport.OrthographicProjection = checkBox.Checked;
                    cameraCM.Hide();
                };
                cameraCM.VisibleChanged += control => orthoValue.Checked = Viewport.OrthographicProjection;
            }

            // Field of View
            {
                var fov = cameraCM.AddButton("Field Of View");
                fov.CloseMenuOnClick = false;
                var fovValue = new FloatValueBox(Viewport.FieldOfView, xLocationForExtras, 2, 70.0f, 35.0f, 160.0f, 0.1f)
                {
                    Parent = fov
                };

                fovValue.ValueChanged += () => Viewport.FieldOfView = fovValue.Value;
                cameraCM.VisibleChanged += control =>
                {
                    fov.Visible = !Viewport.OrthographicProjection;
                    fovValue.Value = Viewport.FieldOfView;
                };
            }

            // Orthographic Scale
            {
                var orthoSize = cameraCM.AddButton("Ortho Scale");
                orthoSize.CloseMenuOnClick = false;
                var orthoSizeValue = new FloatValueBox(Viewport.OrthographicScale, xLocationForExtras, 2, 70.0f, 0.001f, 100000.0f, 0.01f)
                {
                    Parent = orthoSize
                };

                orthoSizeValue.ValueChanged += () => Viewport.OrthographicScale = orthoSizeValue.Value;
                cameraCM.VisibleChanged += control =>
                {
                    orthoSize.Visible = Viewport.OrthographicProjection;
                    orthoSizeValue.Value = Viewport.OrthographicScale;
                };
            }

            // Near Plane
            {
                var nearPlane = cameraCM.AddButton("Near Plane");
                nearPlane.CloseMenuOnClick = false;
                var nearPlaneValue = new FloatValueBox(Viewport.NearPlane, xLocationForExtras, 2, 70.0f, 0.001f, 1000.0f)
                {
                    Parent = nearPlane
                };

                nearPlaneValue.ValueChanged += () => { Viewport.NearPlane = nearPlaneValue.Value; };
                cameraCM.VisibleChanged += control => nearPlaneValue.Value = Viewport.NearPlane;
            }

            // Far Plane
            {
                var farPlane = cameraCM.AddButton("Far Plane");
                farPlane.CloseMenuOnClick = false;
                var farPlaneValue = new FloatValueBox(Viewport.FarPlane, xLocationForExtras, 2, 70.0f, 10.0f)
                {
                    Parent = farPlane
                };

                farPlaneValue.ValueChanged += () => { Viewport.FarPlane = farPlaneValue.Value; };
                cameraCM.VisibleChanged += control => farPlaneValue.Value = Viewport.FarPlane;
            }

            cameraCM.AddSeparator();

            //todo i have no idea why this would be here.
            // Reset Button
            {
                var reset = cameraCM.AddButton("Reset to default");
                reset.ButtonClicked += button =>
                {
                    Viewport.GetViewportOptions();

                    // if the context menu is opened without triggering the value changes beforehand,
                    // the movement speed will not be correctly reset to its default value in certain cases
                    // therefore, a UI update needs to be triggered here
                    minCamSpeedValue.Value = Viewport.MinMovementSpeed;
                    camSpeedValue.Value = Viewport.MovementSpeed;
                    maxCamSpeedValue.Value = Viewport.MaxMovementSpeed;
                };
            }
        }
    }
}