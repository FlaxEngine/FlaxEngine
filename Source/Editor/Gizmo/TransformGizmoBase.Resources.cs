// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEngine;
using System.Collections.Generic;

namespace FlaxEditor.Gizmo
{
    public partial class TransformGizmoBase
    {

        // Models
        internal static Model _modelPlaneAxisGizmo;
        internal static Model _modelLocationAxisGizmo;

        internal static Model _modelScaleAxisGizmo;

        internal static Model _modelRotationAxisGizmo;

        internal static Model _modelSphere;              //center model for Rotation Gizmo
        internal static Model _modelPlane;               //view place model

        // Materials
        internal List<MaterialInstance> _materialAxis = new List<MaterialInstance>();
        internal List<MaterialInstance> _materialAxisPlane = new List<MaterialInstance>();

        internal MaterialInstance _materialCenter;
        internal MaterialInstance _materialRotCenter;

        internal static readonly Color XAxisColor = Color.ParseHex("CB2F44FF");
        internal static readonly Color YAxisColor = Color.ParseHex("81CB1CFF");
        internal static readonly Color ZAxisColor = Color.ParseHex("2872CBFF");
        internal static readonly Color CenterColor = Color.ParseHex("CCCCCCFF");
        internal static readonly Color RotCenterColor = Color.ParseHex("FFFFFF13");

        internal void Load()
        {
            if (!_modelPlaneAxisGizmo || !_modelLocationAxisGizmo || !_modelScaleAxisGizmo || !_modelRotationAxisGizmo || !_modelSphere || !_modelPlane)
            {
                // note the LoadAsyncInternal can crash in some instances so use of LoadInternal makes more sence 
                _modelPlaneAxisGizmo = FlaxEngine.Content.LoadInternal<Model>("Editor/Gizmo/TransformGizmo/PlaneAxisGizmo");
                _modelLocationAxisGizmo = FlaxEngine.Content.LoadInternal<Model>("Editor/Gizmo/TransformGizmo/LocationAxisGizmo");
                _modelScaleAxisGizmo = FlaxEngine.Content.LoadInternal<Model>("Editor/Gizmo/TransformGizmo/ScaleAxisGizmo");
                _modelRotationAxisGizmo = FlaxEngine.Content.LoadInternal<Model>("Editor/Gizmo/TransformGizmo/RotationAxisGizmo");

                _modelSphere = FlaxEngine.Content.LoadInternal<Model>("Editor/Primitives/Sphere");
                _modelPlane = FlaxEngine.Content.LoadInternal<Model>("Editor/Primitives/Plane");
            }
            // Axis Materials
            var axisMaterialMaster = FlaxEngine.Content.LoadAsyncInternal<Material>("Editor/Gizmo/TransformGizmo/Materials/Axis");
            var axisPlaneMaterialMaster = FlaxEngine.Content.LoadAsyncInternal<Material>("Editor/Gizmo/TransformGizmo/Materials/AxisPlane");
            var CenterMaterialMaster = FlaxEngine.Content.LoadAsyncInternal<Material>("Editor/Gizmo/TransformGizmo/Materials/Center");
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
            _materialAxis.Add(axisMaterialMaster.CreateVirtualInstance());
            _materialAxis[0].SetParameterValue("Color", XAxisColor, true);
            _materialAxis[0].SetParameterValue("IsSelected", false, true);

            _materialAxis.Add(axisMaterialMaster.CreateVirtualInstance());
            _materialAxis[1].SetParameterValue("Color", YAxisColor, true);
            _materialAxis[1].SetParameterValue("IsSelected", false, true);

            _materialAxis.Add(axisMaterialMaster.CreateVirtualInstance());
            _materialAxis[2].SetParameterValue("Color", ZAxisColor, true);
            _materialAxis[2].SetParameterValue("IsSelected", false, true);

            _materialAxisPlane.Add(axisPlaneMaterialMaster.CreateVirtualInstance());
            _materialAxisPlane[0].SetParameterValue("Color", XAxisColor, true);
            _materialAxisPlane[0].SetParameterValue("IsSelected", false, true);

            _materialAxisPlane.Add(axisPlaneMaterialMaster.CreateVirtualInstance());
            _materialAxisPlane[1].SetParameterValue("Color", YAxisColor, true);
            _materialAxisPlane[1].SetParameterValue("IsSelected", false, true);

            _materialAxisPlane.Add(axisPlaneMaterialMaster.CreateVirtualInstance());
            _materialAxisPlane[2].SetParameterValue("Color", ZAxisColor, true);
            _materialAxisPlane[2].SetParameterValue("IsSelected", false, true);

            _materialCenter = CenterMaterialMaster.CreateVirtualInstance();
            _materialCenter.SetParameterValue("Color", CenterColor, true);
            _materialCenter.SetParameterValue("IsSelected", false, true);

            _materialRotCenter = axisMaterialMaster.CreateVirtualInstance();
            _materialRotCenter.SetParameterValue("Color", RotCenterColor, true);
            _materialRotCenter.SetParameterValue("IsSelected", false, true);

            if (!_modelPlaneAxisGizmo || !_modelLocationAxisGizmo || !_modelScaleAxisGizmo || !_modelRotationAxisGizmo || !_modelSphere || !_modelPlane)
            {
                Platform.Fatal("Failed to load transform gizmo resources.");
            }
        }
    }
}
