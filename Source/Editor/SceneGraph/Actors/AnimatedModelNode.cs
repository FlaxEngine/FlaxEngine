// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Windows;
using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="AnimatedModel"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class AnimatedModelNode : ActorNode
    {
        /// <inheritdoc />
        public AnimatedModelNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override void OnContextMenu(ContextMenu contextMenu, EditorWindow window)
        {
            base.OnContextMenu(contextMenu, window);

            var actor = (AnimatedModel)Actor;
            if (actor && actor.SkinnedModel)
            {
                var b = contextMenu.AddButton("Create ragdoll", OnCreateRagdoll);
                b.TooltipText = "Adds ragdoll actor and setups the ragdoll physical structure based on skeleton bones hierarchy.";
            }
        }

        private void OnCreateRagdoll()
        {
            BuildRagdoll((AnimatedModel)Actor, null);
            TreeNode.ExpandAll(true);
        }

        internal class RebuildOptions
        {
            [DefaultValue(20.0f), Limit(0), Tooltip("The minimum size for the bone bounds to be used for physical bodies generation.")]
            public float MinBoneSize = 20.0f;

            [DefaultValue(0.0001f), Limit(0), Tooltip("The minimum size of the bone bounds to be included (used to skip too small, degenerated or invalid bones).")]
            public float MinValidSize = 0.0001f;

            [DefaultValue(1.01f), Limit(0.001f, 2.0f), Tooltip("The scale of the collision body dimensions (relative to the visual dimensions of the bones).")]
            public float CollisionMargin = 1.01f;
        }

        internal static void BuildRagdoll(AnimatedModel actor, RebuildOptions options = null, Ragdoll ragdoll = null, string boneNameToBuild = null)
        {
            if (options == null)
                options = new RebuildOptions();
            var model = actor.SkinnedModel;
            if (!model || model.WaitForLoaded())
            {
                Editor.LogError("Missing or not loaded model.");
                return;
            }
            var bones = model.Bones;
            var nodes = model.Nodes;
            actor.GetCurrentPose(out var localNodesPose);
            if (bones.Length == 0 || localNodesPose.Length == 0)
            {
                Editor.LogError("Empty skeleton.");
                return;
            }
            var skinningMatrices = new Matrix[bones.Length];
            for (int boneIndex = 0; boneIndex < bones.Length; boneIndex++)
            {
                ref var bone = ref bones[boneIndex];
                skinningMatrices[boneIndex] = bone.OffsetMatrix * localNodesPose[bone.NodeIndex];
            }

            // Get vertex data for each mesh
            var meshes = model.LODs[0].Meshes;
            var bonesVertices = new List<Float3>[bones.Length];
            var indicesLimit = new Int4(bones.Length - 1);
            for (int i = 0; i < meshes.Length; i++)
            {
                var assessor = new MeshAccessor();
                if (assessor.LoadMesh(meshes[i]))
                    return;
                var positionStream = assessor.Position();
                var blendIndicesStream = assessor.BlendIndices();
                var blendWeightsStream = assessor.BlendWeights();
                if (!positionStream.IsValid || !blendIndicesStream.IsValid || !blendWeightsStream.IsValid)
                    continue;
                var count = positionStream.Count;
                for (int j = 0; j < count; j++)
                {
                    var weights = blendWeightsStream.GetFloat4(j);
                    var indices = Int4.Min((Int4)blendIndicesStream.GetFloat4(j), indicesLimit);

                    // Find the bone with the highest influence on the vertex
                    var maxWeightIndex = 0;
                    for (int l = 0; l < 4; l++)
                    {
                        if (weights[l] > weights[maxWeightIndex])
                            maxWeightIndex = l;
                    }
                    var maxWeightBone = indices[maxWeightIndex];

                    // Skin vertex position with the current pose
                    var position = positionStream.GetFloat3(j);
                    Float3.Transform(ref position, ref skinningMatrices[indices[0]], out Float3 pos0);
                    Float3.Transform(ref position, ref skinningMatrices[indices[1]], out Float3 pos1);
                    Float3.Transform(ref position, ref skinningMatrices[indices[2]], out Float3 pos2);
                    Float3.Transform(ref position, ref skinningMatrices[indices[3]], out Float3 pos3);
                    position = pos0 * weights[0] + pos1 * weights[1] + pos2 * weights[2] + pos3 * weights[3];

                    // Add vertex to the bone list
                    ref var boneVertices = ref bonesVertices[maxWeightBone];
                    if (boneVertices == null)
                        boneVertices = new List<Float3>();
                    boneVertices.Add(position);
                }
            }

            // Find small bones and try to merge them into parent (from back to end because the bones are always ordered from children to parents)
            var bonesMergedSizes = new float[bones.Length];
            for (int boneIndex = bones.Length - 1; boneIndex >= 0; boneIndex--)
            {
                ref var bone = ref bones[boneIndex];
                ref var boneVertices = ref bonesVertices[boneIndex];
                if (boneVertices == null)
                    continue; // Skip not used bones

                // Compute bounds of the vertices using this bone (in local space of the actor)
                Float3 boneBoundsMin = boneVertices[0], boneBoundsMax = boneVertices[0];
                for (int i = 1; i < boneVertices.Count; i++)
                {
                    var pos = boneVertices[i];
                    boneBoundsMin = Float3.Min(boneBoundsMin, pos);
                    boneBoundsMax = Float3.Max(boneBoundsMax, pos);
                }
                var boneBoxSize = ((boneBoundsMax - boneBoundsMin) * 0.5f).Length;
                var boneMergedSize = bonesMergedSizes[boneIndex] += boneBoxSize;
                if (boneMergedSize < options.MinBoneSize && boneMergedSize >= options.MinValidSize)
                {
                    // Don't merge bone that was selected for rebuild
                    if (boneNameToBuild != null && boneNameToBuild == nodes[bone.NodeIndex].Name)
                        continue;

                    if (bone.ParentIndex != -1)
                    {
                        // Merge it into parent
                        bonesMergedSizes[bone.ParentIndex] += boneMergedSize;
                        ref var parentVertices = ref bonesVertices[bone.ParentIndex];
                        if (parentVertices == null)
                            parentVertices = boneVertices;
                        else
                            parentVertices.AddRange(boneVertices);
                    }
                    boneVertices = null;
                }
            }

            // Calculate the final sizes for the bone shapes
            var bonesBounds = new BoundingBox[bones.Length];
            for (int boneIndex = 0; boneIndex < bones.Length; ++boneIndex)
            {
                ref var boneVertices = ref bonesVertices[boneIndex];
                var boneBounds = BoundingBox.Zero;
                if (boneVertices != null)
                {
                    boneBounds = new BoundingBox(boneVertices[0], boneVertices[0]);
                    for (int i = 1; i < boneVertices.Count; i++)
                    {
                        var pos = boneVertices[i];
                        boneBounds.Minimum = Float3.Min(boneBounds.Minimum, pos);
                        boneBounds.Minimum = Float3.Max(boneBounds.Maximum, pos);
                    }
                }
                bonesBounds[boneIndex] = boneBounds;
            }

            // In case of problematic skeleton find the first bone to be used as a root
            int forcedRootBoneIndex = -1, firstParentBoneIndex = -1;
            for (int boneIndex = 0; boneIndex < bones.Length; ++boneIndex)
            {
                if (bonesMergedSizes[boneIndex] > options.MinBoneSize)
                {
                    var parentIndex = bones[boneIndex].ParentIndex;
                    if (parentIndex == -1)
                        break; // The root node has a body

                    if (firstParentBoneIndex == -1)
                    {
                        firstParentBoneIndex = parentIndex; // Cache the first parent for case sof multiple roots
                    }
                    else if (parentIndex == firstParentBoneIndex)
                    {
                        forcedRootBoneIndex = parentIndex; // In case of multiple roots use their parent as a root
                        break;
                    }
                }
            }

            // TODO: code above /\ could be async, then code below \/ could be executed on main thread to be safe

            // TODO: add undo support

            var boneBodies = new RigidBody[bones.Length];

            if (ragdoll == null)
            {
                // Spawn ragdoll actor
                ragdoll = new Ragdoll
                {
                    StaticFlags = StaticFlags.None,
                    Name = "Ragdoll",
                    Parent = actor,
                };
            }
            else
            {
                // Reuse existing bones for joints
                var children = ragdoll.Children;
                for (int boneIndex = 0; boneIndex < bones.Length; ++boneIndex)
                {
                    ref var bone = ref bones[boneIndex];
                    var node = nodes[bone.NodeIndex];
                    boneBodies[boneIndex] = (RigidBody)children.FirstOrDefault(x => x is RigidBody && x.Name == node.Name);
                }
            }

            // Spawn physical bodies for bones
            for (int boneIndex = 0; boneIndex < bones.Length; ++boneIndex)
            {
                ref var boneVertices = ref bonesVertices[boneIndex];
                if (boneVertices == null || boneVertices.Count == 0)
                    continue;
                var boneBounds = bonesBounds[boneIndex];
                if (bonesMergedSizes[boneIndex] < options.MinBoneSize && boneIndex != forcedRootBoneIndex && boneNameToBuild == null)
                    continue;
                ref var bone = ref bones[boneIndex];
                ref var node = ref nodes[bone.NodeIndex];
                if (boneNameToBuild != null && boneNameToBuild != node.Name)
                    continue;
                if (boneBodies[boneIndex] != null)
                    continue;

                // Calculate bone orientation based on the variance of the vertices
                var covarianceMatrix = CalculateCovarianceMatrix(boneVertices);
                var direction = ComputeEigenVector(ref covarianceMatrix);
                var boneOrientation = Quaternion.FromDirection(direction);

                // Spawn body
                var body = new RigidBody
                {
                    StaticFlags = StaticFlags.None,
                    Name = node.Name,
                    LocalPosition = boneBounds.Center,
                    LocalOrientation = boneOrientation,
                    Parent = ragdoll,
                };
                boneBodies[boneIndex] = body;
                var boneTransform = body.LocalTransform;

                // Find the bounding box of the vertices in the local space of the bone
                var boneLocalBounds = BoundingBox.Zero;
                for (int i = 0; i < boneVertices.Count; i++)
                {
                    var pos = boneTransform.WorldToLocal(boneVertices[i]);
                    Vector3.Min(ref boneLocalBounds.Minimum, ref pos, out boneLocalBounds.Minimum);
                    Vector3.Max(ref boneLocalBounds.Maximum, ref pos, out boneLocalBounds.Maximum);
                }

                // Add collision shape
                Float3 boneLocalBoundsSize = boneLocalBounds.Size;
#if false
                var collider = new BoxCollider
                {
                    Name = "Box",
                    Size = boneLocalBoundsSize * options.CollisionMargin,
                };
#elif false
                var collider = new SphereCollider
                {
                    Name = "Sphere",
                    Radius = boneLocalBoundsSize.MaxValue * 0.5f * options.CollisionMargin,
                };
#elif true
                var collider = new CapsuleCollider
                {
                    Name = "Capsule",
                };
                if (boneLocalBoundsSize.X > boneLocalBoundsSize.Y && boneLocalBoundsSize.X > boneLocalBoundsSize.Z)
                {
                    collider.Height = boneLocalBoundsSize.X * options.CollisionMargin;
                    collider.Radius = Mathf.Max(boneLocalBoundsSize.Y, boneLocalBoundsSize.Z) * 0.5f * options.CollisionMargin;
                }
                else if (boneLocalBoundsSize.Y > boneLocalBoundsSize.X && boneLocalBoundsSize.Y > boneLocalBoundsSize.Z)
                {
                    collider.LocalOrientation = Quaternion.Euler(0, 0, 90);
                    collider.Height = boneLocalBoundsSize.Y * options.CollisionMargin;
                    collider.Radius = Mathf.Max(boneLocalBoundsSize.X, boneLocalBoundsSize.Z) * 0.5f * options.CollisionMargin;
                }
                else
                {
                    collider.LocalOrientation = Quaternion.Euler(0, 90, 0);
                    collider.Height = boneLocalBoundsSize.Z * options.CollisionMargin;
                    collider.Radius = Mathf.Max(boneLocalBoundsSize.X, boneLocalBoundsSize.Y) * 0.5f * options.CollisionMargin;
                }
                collider.Height = Mathf.Max(collider.Height - collider.Radius * 2.0f, 0.0f);
#endif
                collider.StaticFlags = StaticFlags.None;
                collider.Parent = body;

                // Crate joint with parent body
                int parentBoneIndex = bone.ParentIndex;
                while (parentBoneIndex != -1)
                {
                    if (boneBodies[parentBoneIndex] != null)
                        break;
                    parentBoneIndex = bones[parentBoneIndex].ParentIndex;
                }
                if (parentBoneIndex != -1)
                {
                    var parentBody = boneBodies[parentBoneIndex];
                    var jointPose = localNodesPose[bone.NodeIndex];
#if false
                    var joint = new FixedJoint();
#else
                    var joint = new D6Joint
                    {
                        LimitSwing = new LimitConeRange(45.0f, 45.0f),
                        LimitTwist = new LimitAngularRange(-15.0f, 15.0f),
                    };
                    joint.SetMotion(D6JointAxis.X, D6JointMotion.Locked);
                    joint.SetMotion(D6JointAxis.Y, D6JointMotion.Locked);
                    joint.SetMotion(D6JointAxis.Z, D6JointMotion.Locked);
                    joint.SetMotion(D6JointAxis.SwingY, D6JointMotion.Limited);
                    joint.SetMotion(D6JointAxis.SwingZ, D6JointMotion.Limited);
                    joint.SetMotion(D6JointAxis.Twist, D6JointMotion.Limited);
#endif
                    joint.StaticFlags = StaticFlags.None;
                    joint.EnableCollision = false;
#if true
                    // Child -> Parent
                    joint.Name = "Joint";
                    joint.Target = parentBody;
                    joint.Parent = body;
                    //joint.Orientation = Quaternion.FromDirection(Float3.Normalize(parentBody.Position - body.Position));
#else
                    // Parent -> Child
                    joint.Name = "Joint to " + body.Name;
                    joint.Target = body;
                    joint.Parent = parentBody;
                    //joint.Orientation = Quaternion.FromDirection(Float3.Normalize(body.Position - parentBody.Position));
#endif
                    joint.SetJointLocation(actor.Transform.LocalToWorld(jointPose.TranslationVector));
                    joint.SetJointOrientation(actor.Transform.Orientation * Quaternion.RotationMatrix(jointPose));
                    joint.EnableAutoAnchor = true; // Use automatic target anchor to make it easier to setup joint in editor when working with ragdolls
                }
            }

            Editor.Instance.Scene.MarkSceneEdited(actor.Scene);
        }

        private static unsafe Matrix CalculateCovarianceMatrix(List<Float3> vertices)
        {
            // [Reference: https://en.wikipedia.org/wiki/Covariance_matrix]

            // Calculate average point
            var avg = Float3.Zero;
            for (int i = 0; i < vertices.Count; i++)
                avg += vertices[i];
            avg /= vertices.Count;

            // Calculate distance to average for every point
            var errors = new Float3[vertices.Count];
            for (int i = 0; i < vertices.Count; i++)
                errors[i] = vertices[i] - avg;

            var covariance = Matrix.Identity;
            var cj = stackalloc float[3];
            for (int j = 0; j < 3; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    // Average of the squared errors sum
                    for (int i = 0; i < vertices.Count; i++)
                    {
                        var error = errors[i];
                        cj[k] += error[j] * error[k];
                    }
                    cj[k] /= vertices.Count;
                }

                var row = new Float4(cj[0], cj[1], cj[2], 0.0f);
                switch (j)
                {
                case 0: covariance.Row1 = row; break;
                case 1: covariance.Row2 = row; break;
                case 2: covariance.Row3 = row; break;
                }
            }
            return covariance;
        }

        private static Float3 ComputeEigenVector(ref Matrix matrix)
        {
            // [Reference: http://en.wikipedia.org/wiki/Power_iteration]
            var bk = new Float3(0, 0, 1);
            for (int i = 0; i < 32; ++i)
            {
                float bkLength = bk.Length;
                if (bkLength > 0.0f)
                {
                    Float3.Transform(ref bk, ref matrix, out Float3 bkA);
                    bk = bkA / bkLength;
                }
            }
            return bk.Normalized;
        }
    }
}
