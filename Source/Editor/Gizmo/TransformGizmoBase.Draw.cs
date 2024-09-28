// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
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
        }

        /// <summary>
        /// The Transform Gizmo Renderer
        /// </summary>
        [HideInEditor]
        public sealed class TransformGizmoEditorRenderer : PostProcessEffect
        {
            /// <summary>
            /// The transform gizmo to draw.
            /// </summary>
            public TransformGizmo Gizmo;

            /// <summary>
            /// Default constructor for Transform Gizmo EditorRenderer
            /// </summary>
            public TransformGizmoEditorRenderer()
            {
                Order = 1000;
            }

             /// <inheritdoc />
            public override void Render(GPUContext context, ref RenderContext renderContext, GPUTexture input, GPUTexture output)
            {
                if (Gizmo == null)
                    throw new NullReferenceException();

                Profiler.BeginEventGPU("Editor Transform Primitives");

                // Check if use MSAA
                var format = output.Format;
                var formatFeatures = GPUDevice.Instance.GetFormatFeatures(format);
                bool enableMsaa = formatFeatures.MSAALevelMax >= MSAALevel.X4 && Editor.Instance.Options.Options.Visual.EnableMSAAForDebugDraw;

                // Prepare
                var msaaLevel = enableMsaa ? MSAALevel.X4 : MSAALevel.None;
                var width = output.Width;
                var height = output.Height;
                var desc = GPUTextureDescription.New2D(width, height, format, GPUTextureFlags.RenderTarget | GPUTextureFlags.ShaderResource, 1, 1, msaaLevel);
                var target = RenderTargetPool.Get(ref desc);
                desc = GPUTextureDescription.New2D(width, height, PixelFormat.D24_UNorm_S8_UInt, GPUTextureFlags.DepthStencil, 1, 1, msaaLevel);
                var targetDepth = RenderTargetPool.Get(ref desc);

                // Copy frame and clear depth
                context.Draw(target, input);
                context.ClearDepth(targetDepth.View());
                context.SetViewport(width, height);
                context.SetRenderTarget(targetDepth.View(), target.View());

                // Draw gizmos and other editor primitives
                var renderList = RenderList.GetFromPool();
                var prevList = renderContext.List;
                renderContext.List = renderList;
                renderContext.View.Pass = DrawPass.GBuffer | DrawPass.Forward;
                try
                {
                    Gizmo.Draw(ref renderContext);
                }
                catch (Exception ex)
                {
                    Editor.LogWarning(ex);
                }

                // Sort draw calls
                renderList.SortDrawCalls(ref renderContext, false, DrawCallsListType.GBuffer);
                renderList.SortDrawCalls(ref renderContext, false, DrawCallsListType.GBufferNoDecals);
                renderList.SortDrawCalls(ref renderContext, true, DrawCallsListType.Forward);

                // Perform the rendering
                renderContext.View.Pass = DrawPass.GBuffer;
                renderList.ExecuteDrawCalls(ref renderContext, DrawCallsListType.GBuffer);
                renderList.ExecuteDrawCalls(ref renderContext, DrawCallsListType.GBufferNoDecals);
                renderContext.View.Pass = DrawPass.Forward;
                renderList.ExecuteDrawCalls(ref renderContext, DrawCallsListType.Forward);

                // Resolve MSAA texture
                if (enableMsaa)
                    context.ResolveMultisample(target, output);
                else
                    context.Draw(output, target);

                // Cleanup
                RenderTargetPool.Release(targetDepth);
                RenderTargetPool.Release(target);
                RenderList.ReturnToPool(renderList);
                renderContext.List = prevList;

                Profiler.EndEventGPU();
            }
        }

        /// <inheritdoc />
        public override void Draw(ref RenderContext renderContext)
        {
            if (!_isActive || !IsActive)
                return;
            if (!_modelCube || !_modelCube.IsLoaded)
                return;

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
                transAxisMesh.Draw(ref renderContext, isXAxis ? _materialAxisFocus : _materialAxisX, ref m3);

                // Y axis
                Matrix.RotationX(Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                transAxisMesh.Draw(ref renderContext, isYAxis ? _materialAxisFocus : _materialAxisY, ref m3);

                // Z axis
                Matrix.RotationX(Mathf.Pi, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                transAxisMesh.Draw(ref renderContext, isZAxis ? _materialAxisFocus : _materialAxisZ, ref m3);

                // XY plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationX(Mathf.PiOverTwo), new Vector3(boxSize * boxScale, boxSize * boxScale, 0.0f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                cubeMesh.Draw(ref renderContext, _activeAxis == Axis.XY ? _materialAxisFocus : _materialAxisX, ref m3);

                // ZX plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.Identity, new Vector3(boxSize * boxScale, 0.0f, boxSize * boxScale));
                Matrix.Multiply(ref m2, ref m1, out m3);
                cubeMesh.Draw(ref renderContext, _activeAxis == Axis.ZX ? _materialAxisFocus : _materialAxisZ, ref m3);

                // YZ plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationZ(Mathf.PiOverTwo), new Vector3(0.0f, boxSize * boxScale, boxSize * boxScale));
                Matrix.Multiply(ref m2, ref m1, out m3);
                cubeMesh.Draw(ref renderContext, _activeAxis == Axis.YZ ? _materialAxisFocus : _materialAxisY, ref m3);

                // Center sphere
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                sphereMesh.Draw(ref renderContext, isCenter ? _materialAxisFocus : _materialSphere, ref m3);

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
                rotationAxisMesh.Draw(ref renderContext, isXAxis ? _materialAxisFocus : _materialAxisX, ref m3);

                // Y axis
                rotationAxisMesh.Draw(ref renderContext, isYAxis ? _materialAxisFocus : _materialAxisY, ref m1);

                // Z axis
                Matrix.RotationX(-Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                rotationAxisMesh.Draw(ref renderContext, isZAxis ? _materialAxisFocus : _materialAxisZ, ref m3);

                // Center box
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                sphereMesh.Draw(ref renderContext, isCenter ? _materialAxisFocus : _materialSphere, ref m3);

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
                scaleAxisMesh.Draw(ref renderContext, isXAxis ? _materialAxisFocus : _materialAxisX, ref m3);

                // Y axis
                Matrix.RotationX(Mathf.PiOverTwo, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                scaleAxisMesh.Draw(ref renderContext, isYAxis ? _materialAxisFocus : _materialAxisY, ref m3);

                // Z axis
                Matrix.RotationX(Mathf.Pi, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                scaleAxisMesh.Draw(ref renderContext, isZAxis ? _materialAxisFocus : _materialAxisZ, ref m3);

                // XY plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationX(Mathf.PiOverTwo), new Vector3(boxSize * boxScale, boxSize * boxScale, 0.0f));
                Matrix.Multiply(ref m2, ref m1, out m3);
                cubeMesh.Draw(ref renderContext, _activeAxis == Axis.XY ? _materialAxisFocus : _materialAxisX, ref m3);

                // ZX plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.Identity, new Vector3(boxSize * boxScale, 0.0f, boxSize * boxScale));
                Matrix.Multiply(ref m2, ref m1, out m3);
                cubeMesh.Draw(ref renderContext, _activeAxis == Axis.ZX ? _materialAxisFocus : _materialAxisZ, ref m3);

                // YZ plane
                m2 = Matrix.Transformation(new Vector3(boxSize, boxSize * 0.1f, boxSize), Quaternion.RotationZ(Mathf.PiOverTwo), new Vector3(0.0f, boxSize * boxScale, boxSize * boxScale));
                Matrix.Multiply(ref m2, ref m1, out m3);
                cubeMesh.Draw(ref renderContext, _activeAxis == Axis.YZ ? _materialAxisFocus : _materialAxisY, ref m3);

                // Center box
                Matrix.Scaling(gizmoModelsScale2RealGizmoSize, out m2);
                Matrix.Multiply(ref m2, ref m1, out m3);
                sphereMesh.Draw(ref renderContext, isCenter ? _materialAxisFocus : _materialSphere, ref m3);

                break;
            }
            }

            // Vertex snapping
            if (_vertexSnapObject != null || _vertexSnapObjectTo != null)
            {
                Transform t = _vertexSnapObject?.Transform ?? _vertexSnapObjectTo.Transform;
                Vector3 p = t.LocalToWorld(_vertexSnapObject != null ? _vertexSnapPoint : _vertexSnapPointTo);
                Matrix matrix = new Transform(p, t.Orientation, new Float3(gizmoModelsScale2RealGizmoSize)).GetWorld();
                cubeMesh.Draw(ref renderContext, _materialSphere, ref matrix);
            }
        }
    }
}
