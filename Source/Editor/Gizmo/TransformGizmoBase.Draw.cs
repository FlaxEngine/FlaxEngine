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

        private MaterialBase _materialWire;
        private MaterialBase _materialWireFocus;
        private MaterialBase _materialSphere;

        // Axis mesh
        private Mesh _transAxisMesh;
        private Mesh _rotationAxisMesh;
        private Mesh _scaleAxisMesh;

        // Center
        private Mesh _sphereMesh;

        // XYZ Scale box
        private Mesh _cubeMesh;

        // Flip gizmos based on view position
        bool flipX;
        bool flipY;
        bool flipZ;

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
            _materialWire = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>("Editor/Gizmo/MaterialWire");
            _materialWireFocus = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>("Editor/Gizmo/MaterialWireFocus");

            // Ensure that every asset was loaded
            if (_modelTranslationAxis == null ||
                _modelScaleAxis       == null ||
                _modelRotationAxis    == null ||
                _modelCube            == null ||
                _modelSphere          == null ||
                _materialAxisX        == null ||
                _materialAxisY        == null ||
                _materialAxisZ        == null ||
                _materialWire         == null ||
                _materialSphere       == null ||
                _materialAxisFocus    == null ||
                _materialWireFocus    == null)
            {
                // Error
                Platform.Fatal("Failed to load transform gizmo resources.");
            }

            _transAxisMesh = _modelTranslationAxis.LODs[0].Meshes[0];
            _sphereMesh = _modelSphere.LODs[0].Meshes[0];
            _scaleAxisMesh = _modelScaleAxis.LODs[0].Meshes[0];
            _rotationAxisMesh = _modelRotationAxis.LODs[0].Meshes[0];
            _cubeMesh = _modelCube.LODs[0].Meshes[0];
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

            // TODO: Invert condition to make more sense (When flip is true it should be the opposite not the normal value)
            // Flip gizmos based on view position
            flipX = Owner.ViewPosition.X > Position.X;
            flipY = Owner.ViewPosition.Y > Position.Y;
            flipZ = Owner.ViewPosition.Z < Position.Z;

            var unitX = flipX ? Vector3.UnitX : -Vector3.UnitX;
            var unitY = flipY ? Vector3.UnitY : -Vector3.UnitY;
            var unitZ = flipZ ? -Vector3.UnitZ : Vector3.UnitZ;

            float XRot = flipX ? -Mathf.PiOverTwo : Mathf.PiOverTwo;
            float YRot = flipY ? Mathf.PiOverTwo : -Mathf.PiOverTwo;
            float ZRot = flipZ ? 0 : Mathf.Pi;

            float xBox = flipX ? boxScale : -boxScale;
            float yBox = flipY ? boxScale : -boxScale;
            float zBox = flipZ ? -boxScale : boxScale;

            float xInner = flipX ? InnerExtend : -InnerExtend;
            float yInner = flipY ? InnerExtend : -InnerExtend;
            float zInner = flipZ ? -InnerExtend : InnerExtend;

            float xOuter = flipX ? OuterExtend : -OuterExtend;
            float yOuter = flipY ? OuterExtend : -OuterExtend;
            float zOuter = flipZ ? -OuterExtend : OuterExtend;

            XAxisBox = new BoundingBox(new Vector3(-AxisThickness), new Vector3(AxisThickness)).MakeOffsetted(AxisOffset * unitX).Merge(AxisLength * unitX);
            YAxisBox = new BoundingBox(new Vector3(-AxisThickness), new Vector3(AxisThickness)).MakeOffsetted(AxisOffset * unitY).Merge(AxisLength * unitY);
            ZAxisBox = new BoundingBox(new Vector3(-AxisThickness), new Vector3(AxisThickness)).MakeOffsetted(AxisOffset * unitZ).Merge(AxisLength * unitZ);

            XZBox = new BoundingBox(new Vector3(xInner, 0, zInner), new Vector3(xOuter, 0, zOuter));
            XYBox = new BoundingBox(new Vector3(xInner, yInner, 0), new Vector3(xOuter, yOuter, 0));
            YZBox = new BoundingBox(new Vector3(0, yInner, zInner), new Vector3(0, yOuter, zOuter));

            // Switch mode
            const float gizmoModelsScale2RealGizmoSize = 0.075f;
            switch (_activeMode)
            {
            case Mode.Translate:
            {
                if (!_modelTranslationAxis || !_modelTranslationAxis.IsLoaded || !_modelCube || !_modelCube.IsLoaded)
                    break;

                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m3);
                Matrix.Multiply(ref m3, ref _gizmoWorld, out m1);

                // X axis
                Matrix.RotationY(XRot, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _transAxisMesh.Draw(ref renderContext, isXAxis ? _materialAxisFocus : _materialAxisX, ref m3);

                // Y axis
                Matrix.RotationX(YRot, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _transAxisMesh.Draw(ref renderContext, isYAxis ? _materialAxisFocus : _materialAxisY, ref m3);

                // Z axis
                Matrix.RotationX(ZRot, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _transAxisMesh.Draw(ref renderContext, isZAxis ? _materialAxisFocus : _materialAxisZ, ref m3);

                // XY plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationX(Mathf.PiOverTwo), new Vector3(boxSize * xBox, boxSize * yBox, 0.0f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                _cubeMesh.Draw(ref renderContext, _activeAxis == Axis.XY ? _materialAxisFocus : _materialAxisX, ref m3);
                
                // ZX plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.Identity, new Vector3(boxSize * xBox, 0.0f, boxSize * zBox));
                Matrix.Multiply(ref m2, ref m1, out m3);
                _cubeMesh.Draw(ref renderContext, _activeAxis == Axis.ZX ? _materialAxisFocus : _materialAxisZ, ref m3);
                
                // YZ plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationZ(Mathf.PiOverTwo), new Vector3(0.0f, boxSize * yBox, boxSize * zBox));
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
                if (!_modelRotationAxis || !_modelRotationAxis.IsLoaded || !_modelCube || !_modelCube.IsLoaded)
                    break;

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
                if (!_modelScaleAxis || !_modelScaleAxis.IsLoaded || !_modelCube || !_modelCube.IsLoaded)
                    break;

                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m3);
                Matrix.Multiply(ref m3, ref _gizmoWorld, out m1);

                // X axis
                Matrix.RotationY(XRot, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _scaleAxisMesh.Draw(ref renderContext, isXAxis ? _materialAxisFocus : _materialAxisX, ref m3);

                // Y axis
                Matrix.RotationX(YRot, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _scaleAxisMesh.Draw(ref renderContext, isYAxis ? _materialAxisFocus : _materialAxisY, ref m3);

                // Z axis
                Matrix.RotationX(ZRot, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                _scaleAxisMesh.Draw(ref renderContext, isZAxis ? _materialAxisFocus : _materialAxisZ, ref m3);

                // XY plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationX(Mathf.PiOverTwo), new Vector3(boxSize * xBox, boxSize * yBox, 0.0f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                _cubeMesh.Draw(ref renderContext, _activeAxis == Axis.XY ? _materialAxisFocus : _materialAxisX, ref m3);
                
                // ZX plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.Identity, new Vector3(boxSize * xBox, 0.0f, boxSize * zBox));
                Matrix.Multiply(ref m2, ref m1, out m3);
                _cubeMesh.Draw(ref renderContext, _activeAxis == Axis.ZX ? _materialAxisFocus : _materialAxisZ, ref m3);
                
                // YZ plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationZ(Mathf.PiOverTwo), new Vector3(0.0f, boxSize * yBox, boxSize * zBox));
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
