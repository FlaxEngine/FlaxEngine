// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Actions;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Scripting;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Dedicated custom editor for <see cref="Actor"/> objects.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.Editors.GenericEditor" />
    [CustomEditor(typeof(Actor)), DefaultEditor]
    public class ActorEditor : ScriptingObjectEditor
    {
        private Guid _linkedPrefabId;

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
            var actor = Values.Count == 1 ? Values[0] as Actor : null;
            if (actor != null && actor.HasPrefabLink)
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

                        // Display prefab UI (when displaying object inside Prefab Window then display only nested prefabs)
                        prefab.GetNestedObject(ref prefabObjectId, out var nestedPrefabId, out var nestedPrefabObjectId);
                        var nestedPrefab = FlaxEngine.Content.Load<Prefab>(nestedPrefabId);
                        var panel = layout.UniformGrid();
                        panel.CustomControl.Height = 20.0f;
                        panel.CustomControl.SlotsVertically = 1;
                        if (Presenter == Editor.Instance.Windows.PropertiesWin.Presenter || nestedPrefab)
                        {
                            var targetPrefab = nestedPrefab ?? prefab;
                            panel.CustomControl.SlotsHorizontally = 3;

                            // Selecting actor prefab asset
                            var selectPrefab = panel.Button("Select Prefab");
                            selectPrefab.Button.Clicked += () =>
                            {
                                Editor.Instance.Windows.ContentWin.ClearItemsSearch();
                                Editor.Instance.Windows.ContentWin.Select(targetPrefab);
                            };

                            // Edit selected prefab asset
                            var editPrefab = panel.Button("Edit Prefab");
                            editPrefab.Button.Clicked += () => Editor.Instance.Windows.ContentWin.Open(Editor.Instance.ContentDatabase.FindAsset(targetPrefab.ID));
                        }
                        else
                        {
                            panel.CustomControl.SlotsHorizontally = 1;
                        }

                        // Viewing changes applied to this actor
                        var viewChanges = panel.Button("View Changes");
                        viewChanges.Button.Clicked += () => ViewChanges(viewChanges.Button, new Float2(0.0f, 20.0f));

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
                    if (actor != null)
                        group.Panel.TooltipText = Surface.SurfaceUtils.GetVisualScriptTypeDescription(TypeUtils.GetObjectType(actor));
                    var settingsButton = group.AddSettingsButton();
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
            if (Presenter.Owner != null)
            {
                var lockButton = cm.AddButton(Presenter.Owner.LockSelection ? "Unlock" : "Lock");
                lockButton.ButtonClicked += button =>
                {
                    var owner = Presenter?.Owner;
                    if (owner == null)
                        return;
                    owner.LockSelection = !owner.LockSelection;

                    // Reselect current selection
                    if (!owner.LockSelection && owner.Selection.Count > 0)
                    {
                        var cachedSelection = owner.Selection.ToList();
                        owner.Select(null);
                        owner.Select(cachedSelection);
                    }
                };
            }
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
                Editor.Instance.Prefabs.PrefabApplying -= OnPrefabApplying;
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
                if (Values != null && prefab && !prefab.WaitForLoaded())
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
                node.Text = Utilities.Utils.GetPropertyNameUI(removed.PrefabObject.GetType().Name);
            }
            // Actor or Script
            else if (editor.Values[0] is SceneObject sceneObject)
            {
                node.TextColor = sceneObject.HasPrefabLink ? FlaxEngine.GUI.Style.Current.ProgressNormal : FlaxEngine.GUI.Style.Current.BackgroundSelected;
                if (editor.Values.Info != ScriptMemberInfo.Null)
                {
                    if (editor.Values.GetAttributes().FirstOrDefault(x => x is EditorDisplayAttribute) is EditorDisplayAttribute editorDisplayAttribute && !string.IsNullOrEmpty(editorDisplayAttribute.Name))
                        node.Text = $"{Utilities.Utils.GetPropertyNameUI(editorDisplayAttribute.Name)} ({Utilities.Utils.GetPropertyNameUI(editor.Values.Info.Name)})";
                    else
                        node.Text = Utilities.Utils.GetPropertyNameUI(editor.Values.Info.Name);
                }
                else if (sceneObject is Actor actor)
                    node.Text = $"{actor.Name} ({Utilities.Utils.GetPropertyNameUI(sceneObject.GetType().Name)})";
                else
                    node.Text = Utilities.Utils.GetPropertyNameUI(sceneObject.GetType().Name);
            }
            // Array Item
            else if (editor.ParentEditor is CollectionEditor)
            {
                node.Text = "Element " + editor.ParentEditor.ChildrenEditors.IndexOf(editor);
            }
            // Common type
            else if (editor.Values.Info != ScriptMemberInfo.Null)
            {
                if (editor.Values.GetAttributes().FirstOrDefault(x => x is EditorDisplayAttribute) is EditorDisplayAttribute editorDisplayAttribute
                    && !string.IsNullOrEmpty(editorDisplayAttribute.Name)
                    && !editorDisplayAttribute.Name.Contains("_inline"))
                    node.Text = $"{Utilities.Utils.GetPropertyNameUI(editorDisplayAttribute.Name)} ({Utilities.Utils.GetPropertyNameUI(editor.Values.Info.Name)})";
                else
                    node.Text = Utilities.Utils.GetPropertyNameUI(editor.Values.Info.Name);
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
                return CreateDiffNode(editor);

            // Skip if no change detected
            var isRefEdited = editor.Values.IsReferenceValueModified;
            if (!isRefEdited && skipIfNotModified)
                return null;

            TreeNode result = null;
            if (editor.ChildrenEditors.Count == 0 || (isRefEdited && editor is CollectionEditor))
                result = CreateDiffNode(editor);
            bool isScriptEditorWithRefValue = editor is ScriptsEditor && editor.Values.HasReferenceValue;
            bool isActorEditorInLevel = editor is ActorEditor && editor.Values[0] is Actor actor && actor.IsPrefabRoot && actor.HasScene;
            for (int i = 0; i < editor.ChildrenEditors.Count; i++)
            {
                var childEditor = editor.ChildrenEditors[i];

                // Special case for root actor transformation (can be applied only in Prefab editor, not in Level)
                if (isActorEditorInLevel && childEditor.Values.Info.Name is "LocalPosition" or "LocalOrientation" or "LocalScale" or "Name")
                    continue;

                var child = ProcessDiff(childEditor, !isScriptEditorWithRefValue);
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

        private TreeNode CreateDiffTree(Actor actor, CustomEditorPresenter presenter, LayoutElementsContainer layout)
        {
            var actorNode = Editor.Instance.Scene.GetActorNode(actor);
            ValueContainer vc = new ValueContainer(ScriptMemberInfo.Null);
            vc.SetType(new ScriptType(actorNode.EditableObject.GetType()));
            vc.Add(actorNode.EditableObject);
            var editor = CustomEditorsUtil.CreateEditor(vc, null, false);
            editor.Initialize(presenter, layout, vc);
            var node = ProcessDiff(editor, false);
            layout.ClearLayout();
            foreach (var child in actor.Children)
            {
                var childNode = CreateDiffTree(child, presenter, layout);
                if (childNode == null)
                    continue;
                if (node == null)
                    node = CreateDiffNode(editor);
                node.AddChild(childNode);
            }
            return node;
        }

        private void ViewChanges(Control target, Float2 targetLocation)
        {
            // Build a tree out of modified properties
            var thisActor = (Actor)Values[0];
            var rootActor = thisActor.IsPrefabRoot ? thisActor : thisActor.GetPrefabRoot();
            var presenter = new CustomEditorPresenter(null);
            var layout = new CustomElementsContainer<ContainerControl>();
            var rootNode = CreateDiffTree(rootActor, presenter, layout);

            // Skip if no changes detected
            if (rootNode == null)
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

        private void OnDiffNodeRightClick(TreeNode node, Float2 location)
        {
            var diffMenu = (PrefabDiffContextMenu)node.ParentTree.Tag;

            var menu = new ContextMenu();
            menu.AddButton("Revert", () => OnDiffRevert((CustomEditor)node.Tag));
            menu.AddSeparator();
            menu.AddButton("Revert All", OnDiffRevertAll);
            menu.AddButton("Apply All", OnDiffApplyAll);

            diffMenu.ShowChild(menu, node.PointToParent(diffMenu, new Float2(location.X, node.HeaderHeight)));
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
