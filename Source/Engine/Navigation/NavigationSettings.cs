// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    partial class NavigationSettings
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="NavigationSettings"/> class.
        /// </summary>
        public NavigationSettings()
        {
            // Init navmeshes
            NavMeshes = new NavMeshProperties[1];
            ref var navMesh = ref NavMeshes[0];
            navMesh.Name = "Default";
            navMesh.Color = Color.Green;
            navMesh.Rotation = Quaternion.Identity;
            navMesh.Agent.Radius = 34.0f;
            navMesh.Agent.Height = 144.0f;
            navMesh.Agent.StepHeight = 35.0f;
            navMesh.Agent.MaxSlopeAngle = 60.0f;

            // Init nav areas
            NavAreas = new NavAreaProperties[2];
            ref var areaNull = ref NavAreas[0];
            areaNull.Name = "Null";
            areaNull.Color = Color.Transparent;
            areaNull.Id = 0;
            areaNull.Cost = float.MaxValue;
            ref var areaWalkable = ref NavAreas[1];
            areaWalkable.Name = "Walkable";
            areaWalkable.Color = Color.Transparent;
            areaWalkable.Id = 63;
            areaWalkable.Cost = 1;
        }

        // [Deprecated on 12.01.2021, expires on 12.01.2022]
        private void UpgradeToNavMeshes()
        {
            if (NavMeshes != null && NavMeshes.Length == 1)
                return;
            NavMeshes = new NavMeshProperties[1];
            ref var navMesh = ref NavMeshes[0];
            navMesh.Name = "Default";
            navMesh.Color = Color.Green;
            navMesh.Rotation = Quaternion.Identity;
            navMesh.Agent.Radius = 34.0f;
            navMesh.Agent.Height = 144.0f;
            navMesh.Agent.StepHeight = 35.0f;
            navMesh.Agent.MaxSlopeAngle = 60.0f;
        }

        // [Deprecated on 12.01.2021, expires on 12.01.2022]
        [Serialize, Obsolete]
        private float WalkableRadius
        {
            get => throw new Exception();
            set
            {
                UpgradeToNavMeshes();
                NavMeshes[0].Agent.Radius = value;
            }
        }

        // [Deprecated on 12.01.2021, expires on 12.01.2022]
        [Serialize, Obsolete]
        private float WalkableHeight
        {
            get => throw new Exception();
            set
            {
                UpgradeToNavMeshes();
                NavMeshes[0].Agent.Height = value;
            }
        }

        // [Deprecated on 12.01.2021, expires on 12.01.2022]
        [Serialize, Obsolete]
        private float WalkableMaxClimb
        {
            get => throw new Exception();
            set
            {
                UpgradeToNavMeshes();
                NavMeshes[0].Agent.StepHeight = value;
            }
        }

        // [Deprecated on 12.01.2021, expires on 12.01.2022]
        [Serialize, Obsolete]
        private float WalkableMaxSlopeAngle
        {
            get => throw new Exception();
            set
            {
                UpgradeToNavMeshes();
                NavMeshes[0].Agent.MaxSlopeAngle = value;
            }
        }
    }
}

namespace FlaxEngine
{
    partial struct NavAgentProperties
    {
        /// <inheritdoc />
        public override string ToString()
        {
            return $"Radius: {Radius}, Height: {Height}, StepHeight: {StepHeight}, MaxSlopeAngle: {MaxSlopeAngle}";
        }
    }
}
