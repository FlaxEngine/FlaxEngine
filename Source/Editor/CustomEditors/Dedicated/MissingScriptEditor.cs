// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEditor.Actions;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using System.Collections.Generic;

namespace FlaxEditor.CustomEditors.Dedicated;

/// <summary>
/// The missing script editor.
/// </summary>
[CustomEditor(typeof(MissingScript)), DefaultEditor]
public class MissingScriptEditor : GenericEditor
{
    private DropPanel _dropPanel;
    private Button _replaceScriptButton;
    private CheckBox _shouldReplaceAllCheckbox;

    /// <inheritdoc />
    public override void Initialize(LayoutElementsContainer layout)
    {
        if (layout.ContainerControl is not DropPanel dropPanel)
        {
            base.Initialize(layout);
            return;
        }
        _dropPanel = dropPanel;
        _dropPanel.HeaderTextColor = Color.OrangeRed;

        var replaceScriptPanel = new Panel
        {
            Parent = _dropPanel,
            Height = 64,
        };
        _replaceScriptButton = new Button
        {
            Text = "Replace Script",
            TooltipText = "Replaces the missing script with a given script type",
            AnchorPreset = AnchorPresets.TopCenter,
            Bounds = new Rectangle(-120, 0, 240, 24),
            Parent = replaceScriptPanel,
        };
        _replaceScriptButton.Clicked += OnReplaceScriptButtonClicked;
        var replaceAllLabel = new Label
        {
            Text = "Replace all matching missing scripts",
            TooltipText = "Whether or not to apply this script change to all scripts missing the same type.",
            AnchorPreset = AnchorPresets.BottomCenter,
            Y = -38,
            Parent = replaceScriptPanel,
        };
        _shouldReplaceAllCheckbox = new CheckBox
        {
            TooltipText = replaceAllLabel.TooltipText,
            AnchorPreset = AnchorPresets.BottomCenter,
            Y = -38,
            Parent = replaceScriptPanel,
        };
        _shouldReplaceAllCheckbox.X -= _replaceScriptButton.Width * 0.5f + 0.5f;
        replaceAllLabel.X -= 52;

        base.Initialize(layout);
    }

    private void FindActorsWithMatchingMissingScript(List<MissingScript> missingScripts)
    {
        foreach (Actor actor in Level.GetActors(typeof(Actor)))
        {
            for (int scriptIndex = 0; scriptIndex < actor.ScriptsCount; scriptIndex++)
            {
                Script actorScript = actor.Scripts[scriptIndex];
                if (actorScript is not MissingScript missingActorScript)
                    continue;

                MissingScript currentMissing = Values[0] as MissingScript;
                if (missingActorScript.MissingTypeName != currentMissing.MissingTypeName)
                    continue;

                missingScripts.Add(missingActorScript);
            }
        }
    }

    private void RunReplacementMultiCast(List<IUndoAction> actions)
    {
        if (actions.Count == 0)
        {
            Editor.LogWarning("Failed to replace scripts!");
            return;
        }

        var multiAction = new MultiUndoAction(actions);
        multiAction.Do();
        var presenter = ParentEditor.Presenter;
        if (presenter != null)
        {
            presenter.Undo.AddAction(multiAction);
            presenter.Control.Focus();
        }
    }

    private void ReplaceScript(ScriptType script, bool replaceAllInScene)
    {
        var actions = new List<IUndoAction>(4);

        var missingScripts = new List<MissingScript>();
        if (!replaceAllInScene)
            missingScripts.Add((MissingScript)Values[0]);
        else
            FindActorsWithMatchingMissingScript(missingScripts);

        foreach (var missingScript in missingScripts)
            actions.Add(AddRemoveScript.Add(missingScript.Actor, script));
        RunReplacementMultiCast(actions);

        for (int actionIdx = 0; actionIdx < actions.Count; actionIdx++)
        {
            AddRemoveScript addRemoveScriptAction = (AddRemoveScript)actions[actionIdx];
            int orderInParent = addRemoveScriptAction.GetOrderInParent();

            Script newScript = missingScripts[actionIdx].Actor.Scripts[orderInParent];
            missingScripts[actionIdx].ReferenceScript = newScript;
        }
        actions.Clear();

        foreach (var missingScript in missingScripts)
            actions.Add(AddRemoveScript.Remove(missingScript));
        RunReplacementMultiCast(actions);
    }

    private void OnReplaceScriptButtonClicked()
    {
        var scripts = Editor.Instance.CodeEditing.Scripts.Get();
        if (scripts.Count == 0)
        {
            // No scripts
            var cm1 = new ContextMenu();
            cm1.AddButton("No scripts in project");
            cm1.Show(_dropPanel, _replaceScriptButton.BottomLeft);
            return;
        }

        // Show context menu with list of scripts to add
        var cm = new ItemsListContextMenu(180);
        for (int i = 0; i < scripts.Count; i++)
            cm.AddItem(new TypeSearchPopup.TypeItemView(scripts[i]));
        cm.ItemClicked += item => ReplaceScript((ScriptType)item.Tag, _shouldReplaceAllCheckbox.Checked);
        cm.SortItems();
        cm.Show(_dropPanel, _replaceScriptButton.BottomLeft - new Float2((cm.Width - _replaceScriptButton.Width) / 2, 0));
    }
}
