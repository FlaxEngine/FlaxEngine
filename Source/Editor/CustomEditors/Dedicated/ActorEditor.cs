// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Actions;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Dedicated custom editor for <see cref="Actor"/> objects.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.Editors.GenericEditor" />
    [CustomEditor(typeof(Actor)), DefaultEditor]
    public class ActorEditor : GenericEditor
    {
        private Guid _linkedPrefabId;

        /// <inheritdoc />
        protected override void SpawnProperty(LayoutElementsContainer itemLayout, ValueContainer itemValues, ItemInfo item)
        {
            // Note: we cannot specify actor properties editor types directly because we want to keep editor classes in FlaxEditor assembly
            int order = item.Order?.Order ?? int.MinValue;
            switch (order)
            {
            // Override static flags editor
            case -80:
                item.CustomEditor = new CustomEditorAttribute(typeof(ActorStaticFlagsEditor));
                break;

            // Override layer editor
            case -69:
                item.CustomEditor = new CustomEditorAttribute(typeof(ActorLayerEditor));
                break;

            // Override tag editor
            case -68:
                item.CustomEditor = new CustomEditorAttribute(typeof(ActorTagEditor));
                break;

            // Override position/scale editor
            case -30:
            case -10:
                item.CustomEditor = new CustomEditorAttribute(typeof(ActorTransformEditor.PositionScaleEditor));
                break;

            // Override orientation editor
            case -20:
                item.CustomEditor = new CustomEditorAttribute(typeof(ActorTransformEditor.OrientationEditor));
                break;
            }

            base.SpawnProperty(itemLayout, itemValues, item);
        }

        /// <inheritdoc />
        protected override List<ItemInfo> GetItemsForType(ScriptType type)
        {
            var items = base.GetItemsForType(type);

            // Inject scripts editor
            var scriptsMember = type.GetProperty("Scripts");
            if (scriptsMember != ScriptMemberInfo.Null)
            {
                var item = new ItemInfo(scriptsMember)
                {
                    CustomEditor = new CustomEditorAttribute(typeof(ScriptsEditor))
                };
                items.Add(item);
            }

            return items;
        }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            // Check for prefab link
            if (Values.IsSingleObject && Values[0] is Actor actor && actor.HasPrefabLink)
            {
                // TODO: consider editing more than one instance of the same prefab asset at once

                var prefab = FlaxEngine.Content.LoadAsync<Prefab>(actor.PrefabID);
                // TODO: don't stall here?
                if (prefab && !prefab.WaitForLoaded())
                {
                    var prefabObjectId = actor.PrefabObjectID;
                    var prefabInstance = prefab.GetDefaultInstance(ref prefabObjectId);
                    if (prefabInstance != null)
                    {
                        // Use default prefab instance as a reference for the editor
                        Values.SetReferenceValue(prefabInstance);

                        // Add some UI
                        var panel = layout.CustomContainer<UniformGridPanel>();
                        panel.CustomControl.Height = 20.0f;
                        panel.CustomControl.SlotsVertically = 1;
                        panel.CustomControl.SlotsHorizontally = 2;

                        // Selecting actor prefab asset
                        var selectPrefab = panel.Button("Select Prefab");
                        selectPrefab.Button.Clicked += () => Editor.Instance.Windows.ContentWin.Select(prefab);

                        // Viewing changes applied to this actor
                        var viewChanges = panel.Button("View Changes");
                        viewChanges.Button.Clicked += () => ViewChanges(viewChanges.Button, new Vector2(0.0f, 20.0f));

                        // Link event to update editor on prefab apply
                        _linkedPrefabId = prefab.ID;
                        Editor.Instance.Prefabs.PrefabApplying += OnPrefabApplying;
                        Editor.Instance.Prefabs.PrefabApplied += OnPrefabApplied;
                    }
                }
            }

            base.Initialize(layout);

            // Add custom settings button to General group
            for (int i = 0; i < layout.Children.Count; i++)
            {
                if (layout.Children[i] is GroupElement group && group.Panel.HeaderText == "General")
                {
                    const float settingsButtonSize = 14;
                    var settingsButton = new Image
                    {
                        TooltipText = "Settings",
                        AutoFocus = true,
                        AnchorPreset = AnchorPresets.TopRight,
                        Parent = group.Panel,
                        Bounds = new Rectangle(group.Panel.Width - settingsButtonSize, 0, settingsButtonSize, settingsButtonSize),
                        IsScrollable = false,
                        Color = FlaxEngine.GUI.Style.Current.ForegroundGrey,
                        Margin = new Margin(1),
                        Brush = new SpriteBrush(FlaxEngine.GUI.Style.Current.Settings),
                    };
                    settingsButton.Clicked += OnSettingsButtonClicked;
                    break;
                }
            }
        }

        private void OnSettingsButtonClicked(Image image, MouseButton mouseButton)
        {
            if (mouseButton != MouseButton.Left)
                return;

            var cm = new ContextMenu();
            var actor = (Actor)Values[0];
            var scriptType = TypeUtils.GetType(actor.TypeName);
            var item = scriptType.ContentItem;
            cm.AddButton("Copy ID", OnClickCopyId);
            cm.AddButton("Edit actor type", OnClickEditActorType).Enabled = item != null;
            var showButton = cm.AddButton("Show in content window", OnClickShowActorType);
            showButton.Enabled = item != null;
            showButton.Icon = Editor.Instance.Icons.Search12;
            cm.Show(image, image.Size);
        }

        private void OnClickCopyId()
        {
            var actor = (Actor)Values[0];
            Clipboard.Text = JsonSerializer.GetStringID(actor.ID);
        }

        private void OnClickEditActorType()
        {
            var actor = (Actor)Values[0];
            var scriptType = TypeUtils.GetType(actor.TypeName);
            var item = scriptType.ContentItem;
            if (item != null)
                Editor.Instance.ContentEditing.Open(item);
        }

        private void OnClickShowActorType()
        {
            var actor = (Actor)Values[0];
            var scriptType = TypeUtils.GetType(actor.TypeName);
            var item = scriptType.ContentItem;
            if (item != null)
                Editor.Instance.Windows.ContentWin.Select(item);
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            base.Deinitialize();

            if (_linkedPrefabId != Guid.Empty)
            {
                _linkedPrefabId = Guid.Empty;
                Editor.Instance.Prefabs.PrefabApplied -= OnPrefabApplying;
                Editor.Instance.Prefabs.PrefabApplied -= OnPrefabApplied;
            }
        }

        private void OnPrefabApplied(Prefab prefab, Actor instance)
        {
            if (prefab.ID == _linkedPrefabId)
            {
                // This works fine but in PrefabWindow when using live update it crashes on using color picker/float slider because UI is being rebuild
                //Presenter.BuildLayoutOnUpdate();

                // Better way is to just update the reference value using the new default instance of the prefab, created after changes apply
                if (prefab && !prefab.WaitForLoaded())
                {
                    var actor = (Actor)Values[0];
                    var prefabObjectId = actor.PrefabObjectID;
                    var prefabInstance = prefab.GetDefaultInstance(ref prefabObjectId);
                    if (prefabInstance != null)
                    {
                        Values.SetReferenceValue(prefabInstance);
                        RefreshReferenceValue();
                    }
                }
            }
        }

        private void OnPrefabApplying(Prefab prefab, Actor instance)
        {
            if (prefab.ID == _linkedPrefabId)
            {
                // Unlink reference value (it gets deleted by the prefabs system during apply)
                ClearReferenceValueAll();
            }
        }

        private TreeNode CreateDiffNode(CustomEditor editor)
        {
            var node = new TreeNode(false)
            {
                Tag = editor
            };

            // Removed Script
            if (editor is RemovedScriptDummy removed)
            {
                node.TextColor = Color.OrangeRed;
                node.Text = CustomEditorsUtil.GetPropertyNameUI(removed.PrefabObject.GetType().Name);
            }
            // Actor or Script
            else if (editor.Values[0] is ISceneObject sceneObject)
            {
                node.TextColor = sceneObject.HasPrefabLink ? FlaxEngine.GUI.Style.Current.ProgressNormal : FlaxEngine.GUI.Style.Current.BackgroundSelected;
                node.Text = CustomEditorsUtil.GetPropertyNameUI(sceneObject.GetType().Name);
            }
            // Array Item
            else if (editor.ParentEditor?.Values?.Type.IsArray ?? false)
            {
                node.Text = "Element " + editor.ParentEditor.ChildrenEditors.IndexOf(editor);
            }
            // Common type
            else if (editor.Values.Info != ScriptMemberInfo.Null)
            {
                node.Text = CustomEditorsUtil.GetPropertyNameUI(editor.Values.Info.Name);
            }
            // Custom type
            else if (editor.Values[0] != null)
            {
                node.Text = editor.Values[0].ToString();
            }

            node.Expand(true);

            return node;
        }

        private class RemovedScriptDummy : CustomEditor
        {
            /// <summary>
            /// The removed prefab object (from the prefab default instance).
            /// </summary>
            public Script PrefabObject;

            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                // Not used
            }
        }

        private TreeNode ProcessDiff(CustomEditor editor, bool skipIfNotModified = true)
        {
            // Special case for new Script added to actor
            if (editor.Values[0] is Script script && !script.HasPrefabLink)
            {
                return CreateDiffNode(editor);
            }

            // Skip if no change detected
            if (!editor.Values.IsReferenceValueModified && skipIfNotModified)
                return null;

            TreeNode result = null;

            if (editor.ChildrenEditors.Count == 0)
                result = CreateDiffNode(editor);

            bool isScriptEditorWithRefValue = editor is ScriptsEditor && editor.Values.HasReferenceValue;

            for (int i = 0; i < editor.ChildrenEditors.Count; i++)
            {
                var child = ProcessDiff(editor.ChildrenEditors[i], !isScriptEditorWithRefValue);
                if (child != null)
                {
                    if (result == null)
                        result = CreateDiffNode(editor);

                    result.AddChild(child);
                }
            }

            // Show scripts removed from prefab instance (user may want to restore them)
            if (editor is ScriptsEditor && editor.Values.HasReferenceValue && editor.Values.ReferenceValue is Script[] prefabObjectScripts)
            {
                for (int j = 0; j < prefabObjectScripts.Length; j++)
                {
                    var prefabObjectScript = prefabObjectScripts[j];
                    bool isRemoved = true;

                    for (int i = 0; i < editor.ChildrenEditors.Count; i++)
                    {
                        if (editor.ChildrenEditors[i].Values is ScriptsEditor.ScriptsContainer container && container.PrefabObjectId == prefabObjectScript.PrefabObjectID)
                        {
                            // Found
                            isRemoved = false;
                            break;
                        }
                    }

                    if (isRemoved)
                    {
                        var dummy = new RemovedScriptDummy
                        {
                            PrefabObject = prefabObjectScript
                        };

                        var child = CreateDiffNode(dummy);
                        if (result == null)
                            result = CreateDiffNode(editor);
                        result.AddChild(child);
                    }
                }
            }

            return result;
        }

        private void ViewChanges(Control target, Vector2 targetLocation)
        {
            // Build a tree out of modified properties
            var rootNode = ProcessDiff(this, false);

            // Skip if no changes detected
            if (rootNode == null || rootNode.ChildrenCount == 0)
            {
                var cm1 = new ContextMenu();
                cm1.AddButton("No changes detected");
                cm1.Show(target, targetLocation);
                return;
            }

            // Create context menu
            var cm = new PrefabDiffContextMenu();
            cm.Tree.AddChild(rootNode);
            cm.Tree.RightClick += OnDiffNodeRightClick;
            cm.Tree.Tag = cm;
            cm.RevertAll += OnDiffRevertAll;
            cm.ApplyAll += OnDiffApplyAll;
            cm.Show(target, targetLocation);
        }

        private void OnDiffNodeRightClick(TreeNode node, Vector2 location)
        {
            var diffMenu = (PrefabDiffContextMenu)node.ParentTree.Tag;

            var menu = new ContextMenu();
            menu.AddButton("Revert", () => OnDiffRevert((CustomEditor)node.Tag));
            menu.AddSeparator();
            menu.AddButton("Revert All", OnDiffRevertAll);
            menu.AddButton("Apply All", OnDiffApplyAll);

            diffMenu.ShowChild(menu, node.PointToParent(diffMenu, new Vector2(location.X, node.HeaderHeight)));
        }

        private void OnDiffRevertAll()
        {
            RevertToReferenceValue();
        }

        private void OnDiffApplyAll()
        {
            Editor.Instance.Prefabs.ApplyAll((Actor)Values[0]);

            // Ensure to refresh the layout
            Presenter.BuildLayoutOnUpdate();
        }

        private void OnDiffRevert(CustomEditor editor)
        {
            // Special case for removed Script from actor
            if (editor is RemovedScriptDummy removed)
            {
                Editor.Log("Reverting removed script changes to prefab (adding it)");

                var actor = (Actor)Values[0];
                var restored = actor.AddScript(removed.PrefabObject.GetType());
                var prefabId = actor.PrefabID;
                var prefabObjectId = restored.PrefabObjectID;
                Script.Internal_LinkPrefab(FlaxEngine.Object.GetUnmanagedPtr(restored), ref prefabId, ref prefabObjectId);
                string data = JsonSerializer.Serialize(removed.PrefabObject);
                JsonSerializer.Deserialize(restored, data);

                var action = AddRemoveScript.Added(restored);
                Presenter.Undo?.AddAction(action);

                return;
            }

            // Special case for new Script added to actor
            if (editor.Values[0] is Script script && !script.HasPrefabLink)
            {
                Editor.Log("Reverting added script changes to prefab (removing it)");

                var action = AddRemoveScript.Remove(script);
                action.Do();
                Presenter.Undo?.AddAction(action);

                return;
            }

            editor.RevertToReferenceValue();
        }
    }
}
