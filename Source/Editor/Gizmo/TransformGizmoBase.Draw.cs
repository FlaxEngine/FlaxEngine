// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    public partial class TransformGizmoBase
    {
        private Model _modelTranslateAxis;
        private Model _modelScaleAxis;
        private Model _modelBox;
        private Model _modelCircle;
        private MaterialInstance _materialAxisX;
        private MaterialInstance _materialAxisY;
        private MaterialInstance _materialAxisZ;
        private MaterialInstance _materialAxisFocus;
        private MaterialBase _materialWire;
        private MaterialBase _materialWireFocus;

        private void InitDrawing()
        {
            // Load content (but async - don't wait and don't block execution)
            _modelTranslateAxis = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/TranslateAxis");
            _modelScaleAxis = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/ScaleAxis");
            _modelBox = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/WireBox");
            _modelCircle = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/WireCircle");
            _materialAxisX = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisX");
            _materialAxisY = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisY");
            _materialAxisZ = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisZ");
            _materialAxisFocus = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisFocus");
            _materialWire = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>("Editor/Gizmo/MaterialWire");
            _materialWireFocus = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>("Editor/Gizmo/MaterialWireFocus");

            // Ensure that every asset was loaded
            if (_modelTranslateAxis == null ||
                _modelScaleAxis == null ||
                _modelBox == null ||
                _modelCircle == null ||
                _materialAxisX == null ||
                _materialAxisY == null ||
                _materialAxisZ == null ||
                _materialAxisFocus == null ||
                _materialWire == null ||
                _materialWireFocus == null
            )
            {
                // Error
                Platform.Fatal("Failed to load Transform Gizmo resources.");
            }
        }

        /// <inheritdoc />
        public override void Draw(ref RenderContext renderContext)
        {
            if (!_isActive || !IsActive)
                return;

            Matrix m1, m2, m3;

            bool isXAxis = _activeAxis == Axis.X || _activeAxis == Axis.XY || _activeAxis == Axis.ZX;
            bool isYAxis = _activeAxis == Axis.Y || _activeAxis == Axis.XY || _activeAxis == Axis.YZ;
            bool isZAxis = _activeAxis == Axis.Z || _activeAxis == Axis.YZ || _activeAxis == Axis.ZX;
            bool isCenter = _activeAxis == Axis.Center;

            // Switch mode
            const float gizmoModelsScale2RealGizmoSize = 0.075f;
            switch (_activeMode)
            {
            case Mode.Translate:
            {
                if (!_modelTranslateAxis || !_modelTranslateAxis.IsLoaded || !_modelBox || !_modelBox.IsLoaded)
                    break;

                // Cache data
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m3);
                Matrix.Multiply(ref m3, ref _gizmoWorld, out m1);
                var axisMesh = _modelTranslateAxis.LODs[0].Meshes[0];
                var boxMesh = _modelBox.LODs[0].Meshes[0];
                var boxSize = 10.0f;

                // XY plane
                m2 = Matrix.Transformation(new Vector3(boxSize, 1.0f, boxSize), Quaternion.RotationX(Mathf.PiOverTwo), new Vector3(boxSize * 0.5f, boxSize * 0.5f, 0.0f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                boxMesh.Draw(ref renderContext, _activeAxis == Axis.XY ? _materialWireFocus : _materialWire, ref m3);

                // ZX plane
                m2 = Matrix.Transformation(new Vector3(boxSize, 1.0f, boxSize), Quaternion.Identity, new Vector3(boxSize * 0.5f, 0.0f, boxSize * 0.5f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                boxMesh.Draw(ref renderContext, _activeAxis == Axis.ZX ? _materialWireFocus : _materialWire, ref m3);

                // YZ plane
                m2 = Matrix.Transformation(new Vector3(boxSize, 1.0f, boxSize), Quaternion.RotationZ(Mathf.PiOverTwo), new Vector3(0.0f, boxSize * 0.5f, boxSize * 0.5f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                boxMesh.Draw(ref renderContext, _activeAxis == Axis.YZ ? _materialWireFocus : _materialWire, ref m3);

                // X axis
                axisMesh.Draw(ref renderContext, isXAxis ? _materialAxisFocus : _materialAxisX, ref m1);

                // Y axis
                Matrix.RotationZ(Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                axisMesh.Draw(ref renderContext, isYAxis ? _materialAxisFocus : _materialAxisY, ref m3);

                // Z axis
                Matrix.RotationY(-Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                axisMesh.Draw(ref renderContext, isZAxis ? _materialAxisFocus : _materialAxisZ, ref m3);

                break;
            }

            case Mode.Rotate:
            {
                if (!_modelCircle || !_modelCircle.IsLoaded || !_modelBox || !_modelBox.IsLoaded)
                    break;

                // Cache data
                var circleMesh = _modelCircle.LODs[0].Meshes[0];
                var boxMesh = _modelBox.LODs[0].Meshes[0];
                Matrix.Scaling(8.0f, out m3);
                Matrix.Multiply(ref m3, ref _gizmoWorld, out m1);

                // X axis
                Matrix.RotationZ(Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                circleMesh.Draw(ref renderContext, isXAxis ? _materialAxisFocus : _materialAxisX, ref m3);

                // Y axis
                circleMesh.Draw(ref renderContext, isYAxis ? _materialAxisFocus : _materialAxisY, ref m1);

                // Z axis
                Matrix.RotationX(-Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                circleMesh.Draw(ref renderContext, isZAxis ? _materialAxisFocus : _materialAxisZ, ref m3);

                // Center box
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m3);
                Matrix.Multiply(ref m3, ref _gizmoWorld, out m1);
                Matrix.Scaling(1.0f, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                boxMesh.Draw(ref renderContext, isCenter ? _materialWireFocus : _materialWire, ref m3);

                break;
            }

            case Mode.Scale:
            {
                if (!_modelScaleAxis || !_modelScaleAxis.IsLoaded || !_modelBox || !_modelBox.IsLoaded)
                    break;

                // Cache data
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m3);
                Matrix.Multiply(ref m3, ref _gizmoWorld, out m1);
                var axisMesh = _modelScaleAxis.LODs[0].Meshes[0];
                var boxMesh = _modelBox.LODs[0].Meshes[0];

                // X axis
                Matrix.RotationY(-Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                axisMesh.Draw(ref renderContext, isXAxis ? _materialAxisFocus : _materialAxisX, ref m3);

                // Y axis
                Matrix.RotationX(Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                axisMesh.Draw(ref renderContext, isYAxis ? _materialAxisFocus : _materialAxisY, ref m3);

                // Z axis
                Matrix.RotationX(Mathf.Pi, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                axisMesh.Draw(ref renderContext, isZAxis ? _materialAxisFocus : _materialAxisZ, ref m3);

                // Center box
                Matrix.Scaling(10.0f, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                boxMesh.Draw(ref renderContext, isCenter ? _materialWireFocus : _materialWire, ref m3);

                break;
            }
            }
        }
    }
}
