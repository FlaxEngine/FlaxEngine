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

            internal MaterialInstance _materialAxisRotX;
            internal MaterialInstance _materialAxisRotY;
            internal MaterialInstance _materialAxisRotZ;

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
                var master = FlaxEngine.Content.LoadInternal<Material>("Editor/Gizmo/Transformation Gizmo Master Material");
                // Ensure that every asset was loaded
                if (master == null)
                {
                    Platform.Fatal("Failed to load transform gizmo resource : Transformation Gizmo Master Material");
                }
                //Translecion and scale
                resources._materialAxisX = master.CreateVirtualInstance();
                resources._materialAxisX.SetParameterValue("MainColor", XAxisColor, true);
                resources._materialAxisX.SetParameterValue("IsRotator", false, true);
                resources._materialAxisX.SetParameterValue("IsTranformAxis", true, true);
                resources._materialAxisX.SetParameterValue("IsAxisPlane", false, true);
                resources._materialAxisX.SetParameterValue("Line Thickness", 1f, true);
                resources._materialAxisX.SetParameterValue("UseSphereMask", true, true);
                resources._materialAxisX.SetParameterValue("Forward", Float3.Forward, true);
                resources._materialAxisX.SetParameterValue("IsSelected", false, true);
                resources._materialAxisX.SetParameterValue("Left", Float3.Right, true);
                resources._materialAxisX.SetParameterValue("Up", Float3.Up, true);

                resources._materialAxisY = master.CreateVirtualInstance();
                resources._materialAxisY.SetParameterValue("MainColor", YAxisColor, true);
                resources._materialAxisY.SetParameterValue("IsRotator", false, true);
                resources._materialAxisY.SetParameterValue("IsTranformAxis", true, true);
                resources._materialAxisY.SetParameterValue("IsAxisPlane", false, true);
                resources._materialAxisY.SetParameterValue("Line Thickness", 1f, true);
                resources._materialAxisY.SetParameterValue("UseSphereMask", true, true);
                resources._materialAxisY.SetParameterValue("Forward", Float3.Forward, true);
                resources._materialAxisY.SetParameterValue("IsSelected", false, true);
                resources._materialAxisY.SetParameterValue("Left", Float3.Right, true);
                resources._materialAxisY.SetParameterValue("Up", Float3.Up, true);

                resources._materialAxisZ = master.CreateVirtualInstance();
                resources._materialAxisZ.SetParameterValue("MainColor", ZAxisColor, true);
                resources._materialAxisZ.SetParameterValue("IsRotator", false, true);
                resources._materialAxisZ.SetParameterValue("IsTranformAxis", true, true);
                resources._materialAxisZ.SetParameterValue("IsAxisPlane", false, true);
                resources._materialAxisZ.SetParameterValue("Line Thickness", 1f, true);
                resources._materialAxisZ.SetParameterValue("UseSphereMask", true, true);
                resources._materialAxisZ.SetParameterValue("Forward", Float3.Forward, true);
                resources._materialAxisZ.SetParameterValue("IsSelected", false, true);
                resources._materialAxisZ.SetParameterValue("Left", Float3.Right, true);
                resources._materialAxisZ.SetParameterValue("Up", Float3.Up, true);

                resources._materialAxisPlaneX = master.CreateVirtualInstance();
                resources._materialAxisPlaneX.SetParameterValue("MainColor", XAxisColor, true);
                resources._materialAxisPlaneX.SetParameterValue("IsRotator", false, true);
                resources._materialAxisPlaneX.SetParameterValue("IsTranformAxis", true, true);
                resources._materialAxisPlaneX.SetParameterValue("IsAxisPlane", true, true);
                resources._materialAxisPlaneX.SetParameterValue("Line Thickness", 2.5f, true);
                resources._materialAxisPlaneX.SetParameterValue("UseSphereMask", false, true);
                resources._materialAxisPlaneX.SetParameterValue("Forward", Float3.Up, true);
                resources._materialAxisPlaneX.SetParameterValue("IsSelected", false, true);
                resources._materialAxisPlaneX.SetParameterValue("Left", Float3.Right, true);
                resources._materialAxisPlaneX.SetParameterValue("Up", Float3.Up, true);

                resources._materialAxisPlaneY = master.CreateVirtualInstance();
                resources._materialAxisPlaneY.SetParameterValue("MainColor", YAxisColor, true);
                resources._materialAxisPlaneY.SetParameterValue("IsRotator", false, true);
                resources._materialAxisPlaneY.SetParameterValue("IsTranformAxis", true, true);
                resources._materialAxisPlaneY.SetParameterValue("IsAxisPlane", true, true);
                resources._materialAxisPlaneY.SetParameterValue("Line Thickness", 2.5f, true);
                resources._materialAxisPlaneY.SetParameterValue("UseSphereMask", false, true);
                resources._materialAxisPlaneY.SetParameterValue("Forward", Float3.Up, true);
                resources._materialAxisPlaneY.SetParameterValue("IsSelected", false, true);
                resources._materialAxisPlaneY.SetParameterValue("Left", Float3.Right, true);
                resources._materialAxisPlaneY.SetParameterValue("Up", Float3.Up, true);

                resources._materialAxisPlaneZ = master.CreateVirtualInstance();
                resources._materialAxisPlaneZ.SetParameterValue("MainColor", ZAxisColor, true);
                resources._materialAxisPlaneZ.SetParameterValue("IsRotator", false, true);
                resources._materialAxisPlaneZ.SetParameterValue("IsTranformAxis", true, true);
                resources._materialAxisPlaneZ.SetParameterValue("IsAxisPlane", true, true);
                resources._materialAxisPlaneZ.SetParameterValue("Line Thickness", 2.5f, true);
                resources._materialAxisPlaneZ.SetParameterValue("UseSphereMask", false, true);
                resources._materialAxisPlaneZ.SetParameterValue("Forward", Float3.Up, true);
                resources._materialAxisPlaneZ.SetParameterValue("IsSelected", false, true);
                resources._materialAxisPlaneZ.SetParameterValue("Left", Float3.Right, true);
                resources._materialAxisPlaneZ.SetParameterValue("Up", Float3.Up, true);

                resources._materialAxisRotX = master.CreateVirtualInstance();
                resources._materialAxisRotX.SetParameterValue("MainColor", XAxisColor, true);
                resources._materialAxisRotX.SetParameterValue("IsRotator", true, true);
                resources._materialAxisRotX.SetParameterValue("IsTranformAxis", true, true);
                resources._materialAxisRotX.SetParameterValue("IsAxisPlane", false, true);
                resources._materialAxisRotX.SetParameterValue("Line Thickness", 1f, true);
                resources._materialAxisRotX.SetParameterValue("UseSphereMask", true, true);
                resources._materialAxisRotX.SetParameterValue("Forward", Float3.Up, true);
                resources._materialAxisRotX.SetParameterValue("IsSelected", false, true);
                resources._materialAxisRotX.SetParameterValue("Left", Float3.Forward, true);
                resources._materialAxisRotX.SetParameterValue("Up", Float3.Right, true);

                resources._materialAxisRotY = master.CreateVirtualInstance();
                resources._materialAxisRotY.SetParameterValue("MainColor", YAxisColor, true);
                resources._materialAxisRotY.SetParameterValue("IsRotator", true, true);
                resources._materialAxisRotY.SetParameterValue("IsTranformAxis", true, true);
                resources._materialAxisRotY.SetParameterValue("IsAxisPlane", false, true);
                resources._materialAxisRotY.SetParameterValue("Line Thickness", 1f, true);
                resources._materialAxisRotY.SetParameterValue("UseSphereMask", true, true);
                resources._materialAxisRotY.SetParameterValue("Forward", Float3.Up, true);
                resources._materialAxisRotY.SetParameterValue("IsSelected", false, true);
                resources._materialAxisRotY.SetParameterValue("Left", Float3.Forward, true);
                resources._materialAxisRotY.SetParameterValue("Up", Float3.Right, true);

                resources._materialAxisRotZ = master.CreateVirtualInstance();
                resources._materialAxisRotZ.SetParameterValue("MainColor", ZAxisColor, true);
                resources._materialAxisRotZ.SetParameterValue("IsRotator", true, true);
                resources._materialAxisRotZ.SetParameterValue("IsTranformAxis", true, true);
                resources._materialAxisRotZ.SetParameterValue("IsAxisPlane", false, true);
                resources._materialAxisRotZ.SetParameterValue("Line Thickness", 1f, true);
                resources._materialAxisRotZ.SetParameterValue("UseSphereMask", true, true);
                resources._materialAxisRotZ.SetParameterValue("Forward", Float3.Up, true);
                resources._materialAxisRotZ.SetParameterValue("IsSelected", false, true);
                resources._materialAxisRotZ.SetParameterValue("Left", Float3.Forward, true);
                resources._materialAxisRotZ.SetParameterValue("Up", Float3.Right, true);

                resources._materialCenter = master.CreateVirtualInstance();
                resources._materialCenter.SetParameterValue("MainColor", CenterColor, true);
                resources._materialCenter.SetParameterValue("IsRotator", false, true);
                resources._materialCenter.SetParameterValue("IsTranformAxis", false, true);
                resources._materialCenter.SetParameterValue("IsAxisPlane", true, true);
                resources._materialCenter.SetParameterValue("Line Thickness", 1f, true);
                resources._materialCenter.SetParameterValue("UseSphereMask", true, true);
                resources._materialCenter.SetParameterValue("Forward", Float3.Up, true);
                resources._materialCenter.SetParameterValue("IsSelected", false, true);
                resources._materialCenter.SetParameterValue("Left", Float3.Forward, true);
                resources._materialCenter.SetParameterValue("Up", Float3.Right, true);

                resources._materialRotCenter = master.CreateVirtualInstance();
                resources._materialRotCenter.SetParameterValue("MainColor", RotCenterColor, true);
                resources._materialRotCenter.SetParameterValue("IsRotator", false, true);
                resources._materialRotCenter.SetParameterValue("IsTranformAxis", false, true);
                resources._materialRotCenter.SetParameterValue("IsAxisPlane", false, true);
                resources._materialRotCenter.SetParameterValue("Line Thickness", 1f, true);
                resources._materialRotCenter.SetParameterValue("UseSphereMask", false, true);
                resources._materialRotCenter.SetParameterValue("Forward", Float3.Up, true);
                resources._materialRotCenter.SetParameterValue("IsSelected", false, true);
                resources._materialRotCenter.SetParameterValue("Left", Float3.Forward, true);
                resources._materialRotCenter.SetParameterValue("Up", Float3.Right, true);
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
