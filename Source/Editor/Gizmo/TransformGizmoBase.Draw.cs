// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.Options;
using FlaxEditor.SceneGraph;
using FlaxEngine;
using System;

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
        private MaterialInstance _materialAxisLocked;
        private MaterialBase _materialSphere;

        // Material Parameter Names
        const String _brightnessParamName = "Brightness";
        const String _opacityParamName = "Opacity";

        private void InitDrawing()
        {
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
            _materialAxisFocus = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisFocus");
            _materialAxisLocked = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialAxisLocked");
            _materialSphere = FlaxEngine.Content.LoadAsyncInternal<MaterialInstance>("Editor/Gizmo/MaterialSphere");

            // Ensure that every asset was loaded
            if (_modelTranslationAxis == null ||
                _modelScaleAxis == null ||
                _modelRotationAxis == null ||
                _modelSphere == null ||
                _modelCube == null ||
                _materialAxisX == null ||
                _materialAxisY == null ||
                _materialAxisZ == null ||
                _materialAxisFocus == null ||
                _materialSphere == null)
            {
                Platform.Fatal("Failed to load transform gizmo resources.");
            }

            // Setup editor options
            OnEditorOptionsChanged(Editor.Instance.Options.Options);
            Editor.Instance.Options.OptionsChanged += OnEditorOptionsChanged;
        }

        private void OnEditorOptionsChanged(EditorOptions options)
        {
            float brightness = options.Visual.TransformGizmoBrightness;
            _materialAxisX.SetParameterValue(_brightnessParamName, brightness);
            _materialAxisY.SetParameterValue(_brightnessParamName, brightness);
            _materialAxisZ.SetParameterValue(_brightnessParamName, brightness);
            _materialAxisLocked.SetParameterValue(_brightnessParamName, brightness);
            
            float opacity = options.Visual.TransformGizmoOpacity;
            _materialAxisX.SetParameterValue(_opacityParamName, opacity);
            _materialAxisY.SetParameterValue(_opacityParamName, opacity);
            _materialAxisZ.SetParameterValue(_opacityParamName, opacity);
            _materialAxisLocked.SetParameterValue(_opacityParamName, opacity);
        }

        /// <inheritdoc />
        public override void Draw(ref RenderContext renderContext)
        {
            if (!_isActive || !IsActive)
                return;
            if (!_modelCube || !_modelCube.IsLoaded)
                return;

            // Find out if any of the selected objects can not be moved
            bool gizmoLocked = false;
            if (Editor.Instance.StateMachine.IsPlayMode)
            {
                for (int i = 0; i < SelectionCount; i++)
                {
                    var obj = GetSelectedObject(i);
                    if (obj.CanTransform == false)
                    {
                        gizmoLocked = true;
                        break;
                    }
                }
            }

            // As all axisMesh have the same pivot, add a little offset to the x axisMesh, this way SortDrawCalls is able to sort the draw order
            // https://github.com/FlaxEngine/FlaxEngine/issues/680

            Matrix m1, m2, m3, mx1;
            float boxScale = 300f;
            float boxSize = 0.085f;
            bool isXAxis = _activeAxis == Axis.X || _activeAxis == Axis.XY || _activeAxis == Axis.ZX;
            bool isYAxis = _activeAxis == Axis.Y || _activeAxis == Axis.XY || _activeAxis == Axis.YZ;
            bool isZAxis = _activeAxis == Axis.Z || _activeAxis == Axis.YZ || _activeAxis == Axis.ZX;
            bool isCenter = _activeAxis == Axis.Center;
            renderContext.View.GetWorldMatrix(ref _gizmoWorld, out Matrix world);

            const sbyte sortOrder = 100; // Draw after any other editor shapes
            const float gizmoModelsScale2RealGizmoSize = 0.075f;
            Mesh cubeMesh = _modelCube.LODs[0].Meshes[0];
            Mesh sphereMesh = _modelSphere.LODs[0].Meshes[0];

            Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m3);
            Matrix.Multiply(ref m3, ref world, out m1);
            mx1 = m1;
            mx1.M41 += 0.05f;

            switch (_activeMode)
            {
            case Mode.Translate:
            {
                if (!_modelTranslationAxis || !_modelTranslationAxis.IsLoaded)
                    break;
                var transAxisMesh = _modelTranslationAxis.LODs[0].Meshes[0];

                // X axis
                Matrix.RotationY(-Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance xAxisMaterialTransform = gizmoLocked ? _materialAxisLocked : (isXAxis ? _materialAxisFocus : _materialAxisX);
                transAxisMesh.Draw(ref renderContext, xAxisMaterialTransform, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // Y axis
                Matrix.RotationX(Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance yAxisMaterialTransform = gizmoLocked ? _materialAxisLocked : (isYAxis ? _materialAxisFocus : _materialAxisY);
                transAxisMesh.Draw(ref renderContext, yAxisMaterialTransform, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // Z axis
                Matrix.RotationX(Mathf.Pi, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance zAxisMaterialTransform = gizmoLocked ? _materialAxisLocked : (isZAxis ? _materialAxisFocus : _materialAxisZ);
                transAxisMesh.Draw(ref renderContext, zAxisMaterialTransform, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // XY plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationX(Mathf.PiOverTwo), new Vector3(boxSize * boxScale, boxSize * boxScale, 0.0f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance xyPlaneMaterialTransform = gizmoLocked ? _materialAxisLocked : (_activeAxis == Axis.XY ? _materialAxisFocus : _materialAxisX);
                cubeMesh.Draw(ref renderContext, xyPlaneMaterialTransform, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // ZX plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.Identity, new Vector3(boxSize * boxScale, 0.0f, boxSize * boxScale));
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance zxPlaneMaterialTransform = gizmoLocked ? _materialAxisLocked : (_activeAxis == Axis.ZX ? _materialAxisFocus : _materialAxisY);
                cubeMesh.Draw(ref renderContext, zxPlaneMaterialTransform, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // YZ plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationZ(Mathf.PiOverTwo), new Vector3(0.0f, boxSize * boxScale, boxSize * boxScale));
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance yzPlaneMaterialTransform = gizmoLocked ? _materialAxisLocked : (_activeAxis == Axis.YZ ? _materialAxisFocus : _materialAxisZ);
                cubeMesh.Draw(ref renderContext, yzPlaneMaterialTransform, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // Center sphere
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                sphereMesh.Draw(ref renderContext, isCenter ? _materialAxisFocus : _materialSphere, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                break;
            }

            case Mode.Rotate:
            {
                if (!_modelRotationAxis || !_modelRotationAxis.IsLoaded)
                    break;
                var rotationAxisMesh = _modelRotationAxis.LODs[0].Meshes[0];

                // X axis
                Matrix.RotationZ(Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance xAxisMaterialRotate = gizmoLocked ? _materialAxisLocked : (isXAxis ? _materialAxisFocus : _materialAxisX);
                rotationAxisMesh.Draw(ref renderContext, xAxisMaterialRotate, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // Y axis
                MaterialInstance yAxisMaterialRotate = gizmoLocked ? _materialAxisLocked : (isYAxis ? _materialAxisFocus : _materialAxisY);
                rotationAxisMesh.Draw(ref renderContext, yAxisMaterialRotate, ref m1, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // Z axis
                Matrix.RotationX(-Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance zAxisMaterialRotate = gizmoLocked ? _materialAxisLocked : (isZAxis ? _materialAxisFocus : _materialAxisZ);
                rotationAxisMesh.Draw(ref renderContext, zAxisMaterialRotate, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // Center box
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                sphereMesh.Draw(ref renderContext, isCenter ? _materialAxisFocus : _materialSphere, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                break;
            }

            case Mode.Scale:
            {
                if (!_modelScaleAxis || !_modelScaleAxis.IsLoaded)
                    break;
                var scaleAxisMesh = _modelScaleAxis.LODs[0].Meshes[0];

                // X axis
                Matrix.RotationY(-Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref mx1, out m3);
                MaterialInstance xAxisMaterialRotate = gizmoLocked ? _materialAxisLocked : (isXAxis ? _materialAxisFocus : _materialAxisX);
                scaleAxisMesh.Draw(ref renderContext, xAxisMaterialRotate, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // Y axis
                Matrix.RotationX(Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance yAxisMaterialRotate = gizmoLocked ? _materialAxisLocked : (isYAxis ? _materialAxisFocus : _materialAxisY);
                scaleAxisMesh.Draw(ref renderContext, yAxisMaterialRotate, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // Z axis
                Matrix.RotationX(Mathf.Pi, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance zAxisMaterialRotate = gizmoLocked ? _materialAxisLocked : (isZAxis ? _materialAxisFocus : _materialAxisZ);
                scaleAxisMesh.Draw(ref renderContext, zAxisMaterialRotate, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // XY plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationX(Mathf.PiOverTwo), new Vector3(boxSize * boxScale, boxSize * boxScale, 0.0f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance xyPlaneMaterialScale = gizmoLocked ? _materialAxisLocked : (_activeAxis == Axis.XY ? _materialAxisFocus : _materialAxisX);
                cubeMesh.Draw(ref renderContext, xyPlaneMaterialScale, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // ZX plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.Identity, new Vector3(boxSize * boxScale, 0.0f, boxSize * boxScale));
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance zxPlaneMaterialScale = gizmoLocked ? _materialAxisLocked : (_activeAxis == Axis.ZX ? _materialAxisFocus : _materialAxisZ);
                cubeMesh.Draw(ref renderContext, zxPlaneMaterialScale, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // YZ plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationZ(Mathf.PiOverTwo), new Vector3(0.0f, boxSize * boxScale, boxSize * boxScale));
                Matrix.Multiply(ref m2, ref m1, out m3);
                MaterialInstance yzPlaneMaterialScale = gizmoLocked ? _materialAxisLocked : (_activeAxis == Axis.YZ ? _materialAxisFocus : _materialAxisY);
                cubeMesh.Draw(ref renderContext, yzPlaneMaterialScale, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                // Center box
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                sphereMesh.Draw(ref renderContext, isCenter ? _materialAxisFocus : _materialSphere, ref m3, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);

                break;
            }
            }

            // Vertex snapping
            if (_vertexSnapObject != null || _vertexSnapObjectTo != null)
            {
                Transform t = _vertexSnapObject?.Transform ?? _vertexSnapObjectTo.Transform;
                Vector3 p = t.LocalToWorld(_vertexSnapObject != null ? _vertexSnapPoint : _vertexSnapPointTo);
                Matrix matrix = new Transform(p, t.Orientation, new Float3(gizmoModelsScale2RealGizmoSize)).GetWorld();
                cubeMesh.Draw(ref renderContext, _materialSphere, ref matrix, StaticFlags.None, true, DrawPass.Default, 0.0f, sortOrder);
            }
        }
    }
}
