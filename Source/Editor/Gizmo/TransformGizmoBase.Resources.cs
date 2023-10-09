// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    public partial class TransformGizmoBase
    {
        internal Resources resources;

        internal struct Resources
        {
            // Models
            internal static Model _modelTranslationAxis;
            internal static Model _modelScaleAxis;
            internal static Model _modelRotationAxis;
            internal static Model _modelSphere;
            internal static Model _modelPlane;

            // Materials
            internal MaterialInstance _materialAxisX;
            internal MaterialInstance _materialAxisY;
            internal MaterialInstance _materialAxisZ;

            internal MaterialInstance _materialAxisPlaneX;
            internal MaterialInstance _materialAxisPlaneY;
            internal MaterialInstance _materialAxisPlaneZ;

            internal MaterialInstance _materialCenter;
            internal MaterialInstance _materialRotCenter;

            internal static readonly Color XAxisColor = Color.ParseHex("CB2F44FF");
            internal static readonly Color YAxisColor = Color.ParseHex("81CB1CFF");
            internal static readonly Color ZAxisColor = Color.ParseHex("2872CBFF");
            internal static readonly Color CenterColor = Color.ParseHex("CCCCCCFF");
            internal static readonly Color RotCenterColor = Color.ParseHex("FFFFFF13");

            internal static Resources Load()
            {
                if (!_modelTranslationAxis || !_modelScaleAxis || !_modelRotationAxis || !_modelSphere || !_modelPlane)
                {
                    _modelTranslationAxis   = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/TranslationAxis");
                    _modelScaleAxis         = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/ScaleAxis");
                    _modelRotationAxis      = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Gizmo/RotationAxis");
                    _modelSphere            = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Primitives/Sphere");
                    _modelPlane             = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Primitives/Plane");
                }
                Resources resources = new Resources();
                // Axis Materials
                var axisMaterialMaster = FlaxEngine.Content.LoadAsyncInternal<Material>("Editor/Gizmo/TransformGizmo/Axis");
                var axisPlaneMaterialMaster = FlaxEngine.Content.LoadAsyncInternal<Material>("Editor/Gizmo/TransformGizmo/AxisPlane");
                var CenterMaterialMaster = FlaxEngine.Content.LoadAsyncInternal<Material>("Editor/Gizmo/TransformGizmo/Center");
                // Ensure that every asset was loaded
                if (axisMaterialMaster == null)
                {
                    Platform.Fatal("Failed to load transform gizmo resource : Axis Material");
                }
                if (axisMaterialMaster == null)
                {
                    Platform.Fatal("Failed to load transform gizmo resource : AxisPlane Material");
                }
                if (axisMaterialMaster == null)
                {
                    Platform.Fatal("Failed to load transform gizmo resource : Center Material");
                }
                //Translecion and scale
                resources._materialAxisX = axisMaterialMaster.CreateVirtualInstance();
                resources._materialAxisX.SetParameterValue("Color", XAxisColor, true);
                resources._materialAxisX.SetParameterValue("IsSelected", false, true);

                resources._materialAxisY = axisMaterialMaster.CreateVirtualInstance();
                resources._materialAxisY.SetParameterValue("Color", YAxisColor, true);
                resources._materialAxisY.SetParameterValue("IsSelected", false, true);

                resources._materialAxisZ = axisMaterialMaster.CreateVirtualInstance();
                resources._materialAxisZ.SetParameterValue("Color", ZAxisColor, true);
                resources._materialAxisZ.SetParameterValue("IsSelected", false, true);

                resources._materialAxisPlaneX = axisPlaneMaterialMaster.CreateVirtualInstance();
                resources._materialAxisPlaneX.SetParameterValue("Color", XAxisColor, true);
                resources._materialAxisPlaneX.SetParameterValue("IsSelected", false, true);

                resources._materialAxisPlaneY = axisPlaneMaterialMaster.CreateVirtualInstance();
                resources._materialAxisPlaneY.SetParameterValue("Color", YAxisColor, true);
                resources._materialAxisPlaneY.SetParameterValue("IsSelected", false, true);

                resources._materialAxisPlaneZ = axisPlaneMaterialMaster.CreateVirtualInstance();
                resources._materialAxisPlaneZ.SetParameterValue("Color", ZAxisColor, true);
                resources._materialAxisPlaneZ.SetParameterValue("IsSelected", false, true);

                resources._materialCenter = CenterMaterialMaster.CreateVirtualInstance();
                resources._materialCenter.SetParameterValue("Color", CenterColor, true);
                resources._materialCenter.SetParameterValue("IsSelected", false, true);

                resources._materialRotCenter = axisMaterialMaster.CreateVirtualInstance();
                resources._materialRotCenter.SetParameterValue("Color", RotCenterColor, true);
                resources._materialRotCenter.SetParameterValue("IsSelected", false, true);

                bool flag3 = _modelTranslationAxis == null || _modelScaleAxis == null || _modelRotationAxis == null || _modelSphere == null;
                if (flag3)
                {
                    Platform.Fatal("Failed to load transform gizmo resources.");
                }
                return resources;
            }

        }
    }
}
