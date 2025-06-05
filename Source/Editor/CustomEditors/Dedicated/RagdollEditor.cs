// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="Ragdoll"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(Ragdoll)), DefaultEditor]
    public class RagdollEditor : ActorEditor
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            var ragdoll = Values.Count == 1 ? Values[0] as Ragdoll : null;
            if (ragdoll == null)
                return;

            var editorGroup = layout.Group("Ragdoll Editor");
            editorGroup.Panel.Open(false);

            // Info
            var text = $"Total mass: {Utils.RoundTo1DecimalPlace(ragdoll.TotalMass)}kg";
            var label = editorGroup.Label(text);
            label.Label.AutoHeight = true;

            if (ragdoll.Parent is AnimatedModel animatedModel && animatedModel.SkinnedModel)
            {
                // Builder
                var grid = editorGroup.UniformGrid();
                var gridControl = grid.CustomControl;
                gridControl.ClipChildren = false;
                gridControl.Height = Button.DefaultHeight;
                gridControl.SlotsHorizontally = 3;
                gridControl.SlotsVertically = 1;
                grid.Button("Rebuild").Button.ButtonClicked += OnRebuild;
                grid.Button("Rebuild/Add bone").Button.ButtonClicked += OnRebuildBone;
                grid.Button("Remove bone").Button.ButtonClicked += OnRemoveBone;
            }

            if (Presenter.Owner != null)
            {
                // Selection
                var grid = editorGroup.UniformGrid();
                var gridControl = grid.CustomControl;
                gridControl.ClipChildren = false;
                gridControl.Height = Button.DefaultHeight;
                gridControl.SlotsHorizontally = 3;
                gridControl.SlotsVertically = 1;
                grid.Button("Select all joints").Button.Clicked += OnSelectAllJoints;
                grid.Button("Select all colliders").Button.Clicked += OnSelectAllColliders;
                grid.Button("Select all bodies").Button.Clicked += OnSelectAllBodies;
            }
        }

        class RebuildContextMenu : ContextMenuBase
        {
            private Action<AnimatedModelNode.RebuildOptions> _rebuild;
            private AnimatedModelNode.RebuildOptions _options = new AnimatedModelNode.RebuildOptions();

            public RebuildContextMenu(Action<AnimatedModelNode.RebuildOptions> rebuild)
            {
                _rebuild = rebuild;

                const float width = 280.0f;
                const float height = 220.0f;
                Size = new Float2(width, height);

                // Title
                var title = new Label(2, 2, width - 4, 23.0f)
                {
                    Font = new FontReference(FlaxEngine.GUI.Style.Current.FontLarge),
                    Text = "Ragdoll Options",
                    Parent = this
                };

                // Buttons
                var rebuildButton = new Button(2.0f, title.Bottom + 2.0f, width - 4.0f, 20.0f)
                {
                    Text = "Rebuild",
                    Parent = this
                };
                rebuildButton.Clicked += OnRebuild;

                // Actual panel
                var panel1 = new Panel(ScrollBars.Vertical)
                {
                    Bounds = new Rectangle(0, rebuildButton.Bottom + 2.0f, width, height - rebuildButton.Bottom - 2.0f),
                    Parent = this
                };
                var editor = new CustomEditorPresenter(null);
                editor.Panel.AnchorPreset = AnchorPresets.HorizontalStretchTop;
                editor.Panel.IsScrollable = true;
                editor.Panel.Parent = panel1;

                editor.Select(_options);
            }

            private void OnRebuild()
            {
                Hide();
                _rebuild(_options);
            }

            protected override void OnShow()
            {
                Focus();

                base.OnShow();
            }

            public override void Hide()
            {
                if (!Visible)
                    return;

                Focus(null);

                base.Hide();
            }

            /// <inheritdoc />
            public override bool OnKeyDown(KeyboardKeys key)
            {
                if (key == KeyboardKeys.Escape)
                {
                    Hide();
                    return true;
                }

                return base.OnKeyDown(key);
            }
        }

        private void OnRebuild(Button button)
        {
            var cm = new RebuildContextMenu(Rebuild);
            cm.Show(button.Parent, button.BottomLeft);
        }

        private void Rebuild(AnimatedModelNode.RebuildOptions options)
        {
            var ragdoll = (Ragdoll)Values[0];
            var animatedModel = (AnimatedModel)ragdoll.Parent;

            // Remove existing bodies
            var bodies = ragdoll.Children.Where(x => x is RigidBody && x.IsActive);
            var actions = new List<IUndoAction>();
            foreach (var body in bodies)
            {
                var action = new Actions.DeleteActorsAction(body);
                action.Do();
                actions.Add(action);
            }
            Presenter.Undo?.AddAction(new MultiUndoAction(actions));

            // Build ragdoll
            AnimatedModelNode.BuildRagdoll(animatedModel, options, ragdoll);
        }

        private void OnRebuildBone(Button button)
        {
            PickBone(button, RebuildBone, false);
        }

        private void RebuildBone(string name)
        {
            var ragdoll = (Ragdoll)Values[0];
            var animatedModel = (AnimatedModel)ragdoll.Parent;

            // Remove existing body
            var bodies = ragdoll.Children.Where(x => x is RigidBody && x.IsActive);
            var body = bodies.FirstOrDefault(x => x.Name == name);
            if (body != null)
            {
                var action = new Actions.DeleteActorsAction(body);
                action.Do();
                Presenter.Undo?.AddAction(action);
            }

            // Build ragdoll
            AnimatedModelNode.BuildRagdoll(animatedModel, new AnimatedModelNode.RebuildOptions(), ragdoll, name);
        }

        private void OnRemoveBone(Button button)
        {
            PickBone(button, RemoveBone, true);
        }

        private void RemoveBone(string name)
        {
            var ragdoll = (Ragdoll)Values[0];
            var bodies = ragdoll.Children.Where(x => x is RigidBody && x.IsActive);
            var joints = bodies.SelectMany(y => y.Children).Where(z => z is Joint && z.IsActive).Cast<Joint>();
            var body = bodies.First(x => x.Name == name);
            var replacementJoint = joints.FirstOrDefault(x => x.Parent == body && x.Target != null);

            // Fix joints using this bone
            foreach (var joint in joints)
            {
                if (joint.Target == body)
                {
                    if (replacementJoint != null)
                    {
                        // Swap the joint target to the parent of the removed body to keep ragdoll connected
                        using (new UndoBlock(Presenter.Undo, joint, "Fix joint"))
                        {
                            joint.Target = replacementJoint.Target;
                            joint.EnableAutoAnchor = true;
                        }
                    }
                    else
                    {
                        // Remove joint that will no longer be valid
                        var action = new Actions.DeleteActorsAction(joint);
                        action.Do();
                        Presenter.Undo?.AddAction(action);
                    }
                }
            }

            // Remove body
            {
                var action = new Actions.DeleteActorsAction(body);
                action.Do();
                Presenter.Undo?.AddAction(action);
            }
        }

        private void PickBone(Button button, Action<string> action, bool showOnlyExisting)
        {
            // Show context menu with list of bones to pick
            var cm = new ItemsListContextMenu(280);
            var ragdoll = (Ragdoll)Values[0];
            var bodies = ragdoll.Children.Where(x => x is RigidBody && x.IsActive);
            var animatedModel = (AnimatedModel)ragdoll.Parent;
            animatedModel.SkinnedModel.WaitForLoaded();
            var nodes = animatedModel.SkinnedModel.Nodes;
            var bones = animatedModel.SkinnedModel.Bones;
            foreach (var bone in bones)
            {
                var node = nodes[bone.NodeIndex];
                if (showOnlyExisting && !bodies.Any(x => x.Name == node.Name))
                    continue;
                string prefix = string.Empty, tooltip = node.Name;
                var boneParentIndex = bone.ParentIndex;
                while (boneParentIndex != -1)
                {
                    prefix += "   ";
                    tooltip = nodes[bones[boneParentIndex].NodeIndex].Name + " > " + tooltip;
                    boneParentIndex = bones[boneParentIndex].ParentIndex;
                }
                var item = new ItemsListContextMenu.Item
                {
                    Name = prefix + node.Name,
                    TooltipText = tooltip,
                    Tag = node.Name,
                };
                if (!showOnlyExisting && !bodies.Any(x => x.Name == node.Name))
                    item.TintColor = new Color(1, 0.8f, 0.8f, 0.6f);
                cm.AddItem(item);
            }
            cm.ItemClicked += item => action((string)item.Tag);
            cm.SortItems();
            cm.Show(button.Parent, button.BottomLeft);
        }

        private void OnSelectAllJoints()
        {
            var ragdoll = (Ragdoll)Values[0];
            var bodies = ragdoll.Children.Where(x => x is RigidBody && x.IsActive);
            var joints = bodies.SelectMany(y => y.Children).Where(z => z is Joint && z.IsActive);
            Select(joints);
        }

        private void OnSelectAllColliders()
        {
            var ragdoll = (Ragdoll)Values[0];
            var bodies = ragdoll.Children.Where(x => x is RigidBody && x.IsActive);
            var colliders = bodies.SelectMany(y => y.Children).Where(z => z is Collider && z.IsActive);
            Select(colliders);
        }

        private void OnSelectAllBodies()
        {
            var ragdoll = (Ragdoll)Values[0];
            var bodies = ragdoll.Children.Where(x => x is RigidBody && x.IsActive);
            Select(bodies);
        }

        private void Select(IEnumerable<Actor> list)
        {
            var selection = new List<SceneGraphNode>();
            foreach (var e in list)
            {
                var node = SceneGraphFactory.FindNode(e.ID);
                if (node != null)
                    selection.Add(node);
            }
            Presenter.Owner.Select(selection);
        }
    }
}
