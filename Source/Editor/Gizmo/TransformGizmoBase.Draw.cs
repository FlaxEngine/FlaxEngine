// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    public partial class TransformGizmoBase
    {
        // Models
        private Model _modelTranslationAxis;
        private Model _modelScaleAxis;
        private Model _modelRotationAxis;
        private Model _modelSphere;
        private Model _modelCube;

        // Materials
        private MaterialInstance _materialAxisX;
        private MaterialInstance _materialAxisY;
        private MaterialInstance _materialAxisZ;
        private MaterialInstance _materialAxisFocus;
        private MaterialBase _materialSphere;

        // Axis mesh
        private Mesh _transAxisMesh;
        private Mesh _rotationAxisMesh;
        private Mesh _scaleAxisMesh;
        private Mesh _sphereMesh;
        // XYZ Scale box
        private Mesh _cubeMesh;

        private void InitDrawing()
        {
            // Load content (but async - don't wait and don't block execution)

            // Axis Models
            _modelTranslationAxis = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/TranslationAxis");
            _modelScaleAxis = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/ScaleAxis");
            _modelRotationAxis = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/RotationAxis");
            _modelSphere = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Primitives/Sphere");
            _modelCube = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Primitives/Cube");

            // Axis Materials
            _materialAxisX = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisX");
            _materialAxisY = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisY");
            _materialAxisZ = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisZ");
            _materialSphere = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialSphere");
            _materialAxisFocus = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisFocus");

            // Ensure that every asset was loaded
            if (_modelTranslationAxis == null ||
                _modelScaleAxis       == null ||
                _modelRotationAxis    == null ||
                _modelCube            == null ||
                _modelSphere          == null ||
                _materialAxisX        == null ||
                _materialAxisY        == null ||
                _materialAxisZ        == null ||
                _materialSphere       == null ||
                _materialAxisFocus    == null )
            {
                // Error
                Platform.Fatal("Failed to load transform gizmo resources.");
            }
        }

        /// <inheritdoc />
        public override void Draw(ref RenderContext renderContext)
        {
            if (!_isActive || !IsActive)
                return;

            Matrix m1, m2, m3;
            const float boxScale = 300f;
            const float boxSize = 0.085f;

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
                if (!_modelTranslationAxis || !_modelTranslationAxis.IsLoaded || !_modelCube || !_modelCube.IsLoaded || !_modelSphere || !_modelSphere.IsLoaded)
                    break;

                _transAxisMesh = _modelTranslationAxis.LODs[0].Meshes[0];
                _cubeMesh = _modelCube.LODs[0].Meshes[0];
                _sphereMesh = _modelSphere.LODs[0].Meshes[0];

                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m3);
                Matrix.Multiply(ref m3, ref _gizmoWorld, out m1);

                // X axis
                Matrix.RotationY(-Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _transAxisMesh.Draw(ref renderContext, isXAxis ? _materialAxisFocus : _materialAxisX, ref m3);

                // Y axis
                Matrix.RotationX(Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _transAxisMesh.Draw(ref renderContext, isYAxis ? _materialAxisFocus : _materialAxisY, ref m3);

                // Z axis
                Matrix.RotationX(Mathf.Pi, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _transAxisMesh.Draw(ref renderContext, isZAxis ? _materialAxisFocus : _materialAxisZ, ref m3);

                // XY plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationX(Mathf.PiOverTwo), new Vector3(boxSize * boxScale, boxSize * boxScale, 0.0f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                _cubeMesh.Draw(ref renderContext, _activeAxis == Axis.XY ? _materialAxisFocus : _materialAxisX, ref m3);
                
                // ZX plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.Identity, new Vector3(boxSize * boxScale, 0.0f, boxSize * boxScale));
                Matrix.Multiply(ref m2, ref m1, out m3);
                _cubeMesh.Draw(ref renderContext, _activeAxis == Axis.ZX ? _materialAxisFocus : _materialAxisZ, ref m3);
                
                // YZ plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationZ(Mathf.PiOverTwo), new Vector3(0.0f, boxSize * boxScale, boxSize * boxScale));
                Matrix.Multiply(ref m2, ref m1, out m3);
                _cubeMesh.Draw(ref renderContext, _activeAxis == Axis.YZ ? _materialAxisFocus : _materialAxisY, ref m3);

                // Center sphere
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _sphereMesh.Draw(ref renderContext, isCenter ? _materialAxisFocus : _materialSphere, ref m3);

                break;
            }

            case Mode.Rotate:
            {
                if (!_modelRotationAxis || !_modelRotationAxis.IsLoaded || !_modelSphere || !_modelSphere.IsLoaded)
                    break;

                _rotationAxisMesh = _modelRotationAxis.LODs[0].Meshes[0];
                _sphereMesh = _modelSphere.LODs[0].Meshes[0];

                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m3);
                Matrix.Multiply(ref m3, ref _gizmoWorld, out m1);

                // X axis
                Matrix.RotationZ(Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _rotationAxisMesh.Draw(ref renderContext, isXAxis ? _materialAxisFocus : _materialAxisX, ref m3);

                // Y axis
                _rotationAxisMesh.Draw(ref renderContext, isYAxis ? _materialAxisFocus : _materialAxisY, ref m1);

                // Z axis
                Matrix.RotationX(-Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _rotationAxisMesh.Draw(ref renderContext, isZAxis ? _materialAxisFocus : _materialAxisZ, ref m3);

                // Center box
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _sphereMesh.Draw(ref renderContext, isCenter ? _materialAxisFocus : _materialSphere, ref m3);

                break;
            }

            case Mode.Scale:
            {
                if (!_modelScaleAxis || !_modelScaleAxis.IsLoaded || !_modelCube || !_modelCube.IsLoaded || !_modelSphere || !_modelSphere.IsLoaded)
                    break;

                _scaleAxisMesh = _modelScaleAxis.LODs[0].Meshes[0];
                _cubeMesh = _modelCube.LODs[0].Meshes[0];
                _sphereMesh = _modelSphere.LODs[0].Meshes[0];

                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m3);
                Matrix.Multiply(ref m3, ref _gizmoWorld, out m1);

                // X axis
                Matrix.RotationY(-Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _scaleAxisMesh.Draw(ref renderContext, isXAxis ? _materialAxisFocus : _materialAxisX, ref m3);

                // Y axis
                Matrix.RotationX(Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _scaleAxisMesh.Draw(ref renderContext, isYAxis ? _materialAxisFocus : _materialAxisY, ref m3);

                // Z axis
                Matrix.RotationX(Mathf.Pi, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _scaleAxisMesh.Draw(ref renderContext, isZAxis ? _materialAxisFocus : _materialAxisZ, ref m3);

                // XY plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationX(Mathf.PiOverTwo), new Vector3(boxSize * boxScale, boxSize * boxScale, 0.0f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                _cubeMesh.Draw(ref renderContext, _activeAxis == Axis.XY ? _materialAxisFocus : _materialAxisX, ref m3);
                
                // ZX plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.Identity, new Vector3(boxSize * boxScale, 0.0f, boxSize * boxScale));
                Matrix.Multiply(ref m2, ref m1, out m3);
                _cubeMesh.Draw(ref renderContext, _activeAxis == Axis.ZX ? _materialAxisFocus : _materialAxisZ, ref m3);
                
                // YZ plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationZ(Mathf.PiOverTwo), new Vector3(0.0f, boxSize * boxScale, boxSize * boxScale));
                Matrix.Multiply(ref m2, ref m1, out m3);
                _cubeMesh.Draw(ref renderContext, _activeAxis == Axis.YZ ? _materialAxisFocus : _materialAxisY, ref m3);

                // Center box
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _sphereMesh.Draw(ref renderContext, isCenter ? _materialAxisFocus : _materialSphere, ref m3);

                break;
            }
            }
        }
    }
}
