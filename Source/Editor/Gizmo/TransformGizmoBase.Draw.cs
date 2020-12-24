// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    public partial class TransformGizmoBase
    {
        private Model _modelTranslateAxis;
        private Model _modelScaleAxis;
        private Model _modelCircle;
        private Model _modelSphere;
        private Model _modelCube;
        private MaterialInstance _materialAxisX;
        private MaterialInstance _materialAxisY;
        private MaterialInstance _materialAxisZ;
        private MaterialInstance _materialAxisFocus;
        private MaterialBase _materialWire;
        private MaterialBase _materialWireFocus;
        private MaterialBase _materialSphere;

        private void InitDrawing()
        {
            // Load content (but async - don't wait and don't block execution)
            _modelTranslateAxis = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/TranslateAxis");
            _modelScaleAxis     = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/ScaleAxis");
            _modelCircle        = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/WireCircle");
            _modelSphere        = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Primitives/Sphere");
            _modelCube          = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Primitives/Cube");
            _materialAxisX      = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisX");
            _materialAxisY      = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisY");
            _materialAxisZ      = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisZ");
            _materialAxisFocus  = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisFocus");
            _materialWire       = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>("Editor/Gizmo/MaterialWire");
            _materialWireFocus  = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>("Editor/Gizmo/MaterialWireFocus");
            _materialSphere     = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>("Editor/Gizmo/MaterialSphere");

            // Ensure that every asset was loaded
            if (_modelTranslateAxis == null ||
                _modelScaleAxis     == null ||
                _modelCube          == null ||
                _modelCircle        == null ||
                _materialAxisX      == null ||
                _materialAxisY      == null ||
                _materialAxisZ      == null ||
                _materialAxisFocus  == null ||
                _materialWire       == null ||
                _materialWireFocus  == null
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
                if (!_modelTranslateAxis || !_modelTranslateAxis.IsLoaded || !_modelCube || !_modelCube.IsLoaded)
                    break;

                // Cache data
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m3);
                Matrix.Multiply(ref m3, ref _gizmoWorld, out m1);
                var axisMesh = _modelTranslateAxis.LODs[0].Meshes[0];
                var sphereMesh = _modelSphere.LODs[0].Meshes[0];
                var cubeMesh = _modelCube.LODs[0].Meshes[0];
                var boxSize = 0.085f;

                // XY plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationX(Mathf.PiOverTwo), new Vector3(boxSize * 300f, boxSize * 300f, 0.0f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                cubeMesh.Draw(ref renderContext, _activeAxis == Axis.XY ? _materialAxisFocus : _materialAxisX, ref m3);

                // ZX plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.Identity, new Vector3(boxSize * 300f, 0.0f, boxSize * 300f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                cubeMesh.Draw(ref renderContext, _activeAxis == Axis.ZX ? _materialAxisFocus : _materialAxisZ, ref m3);

                // YZ plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationZ(Mathf.PiOverTwo), new Vector3(0.0f, boxSize * 300f, boxSize * 300f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                cubeMesh.Draw(ref renderContext, _activeAxis == Axis.YZ ? _materialAxisFocus : _materialAxisY, ref m3);

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

                // Center box
                Matrix.Scaling(0.085f, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                sphereMesh.Draw(ref renderContext, isCenter ? _materialAxisFocus : _materialSphere, ref m3);
                break;
            }

            case Mode.Rotate:
            {
                if (!_modelCircle || !_modelCircle.IsLoaded || !_modelCube || !_modelCube.IsLoaded)
                    break;

                // Cache data
                var circleMesh = _modelCircle.LODs[0].Meshes[0];
                var sphereMesh = _modelSphere.LODs[0].Meshes[0];
                Matrix.Scaling(0.025f, out m3);
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

                Matrix.Scaling(0.25f, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                sphereMesh.Draw(ref renderContext, isCenter ? _materialAxisFocus : _materialSphere, ref m3);

                break;
            }

            case Mode.Scale:
            {
                if (!_modelScaleAxis || !_modelScaleAxis.IsLoaded || !_modelCube || !_modelCube.IsLoaded)
                    break;

                // Cache data
                
                var axisMesh   = _modelScaleAxis.LODs[0].Meshes[0];
                var cubeMesh = _modelCube.LODs[0].Meshes[0];
                var sphereMesh = _modelSphere.LODs[0].Meshes[0];
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m3);
                Matrix.Multiply(ref m3, ref _gizmoWorld, out m1);
                var boxSize = 0.085f;

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

                // XY plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationX(Mathf.PiOverTwo), new Vector3(boxSize * 300f, boxSize * 300f, 0.0f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                cubeMesh.Draw(ref renderContext, _activeAxis == Axis.XY ? _materialAxisFocus : _materialAxisX, ref m3);
                
                // ZX plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.Identity, new Vector3(boxSize * 300f, 0.0f, boxSize * 300f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                cubeMesh.Draw(ref renderContext, _activeAxis == Axis.ZX ? _materialAxisFocus : _materialAxisZ, ref m3);
                
                // YZ 300f
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationZ(Mathf.PiOverTwo), new Vector3(0.0f, boxSize * 300f, boxSize * 300f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                cubeMesh.Draw(ref renderContext, _activeAxis == Axis.YZ ? _materialAxisFocus : _materialAxisY, ref m3);

                // Center box
                Matrix.Scaling(0.085f, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                sphereMesh.Draw(ref renderContext, isCenter ? _materialAxisFocus : _materialSphere, ref m3);

                break;
            }
            }
        }
    }
}
