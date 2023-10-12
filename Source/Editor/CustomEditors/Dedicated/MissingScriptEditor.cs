using FlaxEditor.Actions;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using System;
using System.Collections.Generic;

namespace FlaxEditor.CustomEditors.Dedicated;

/// <summary>
/// The missing script editor.
/// </summary>
[CustomEditor(typeof(MissingScript)), DefaultEditor]
public class MissingScriptEditor : GenericEditor
{
    DropPanel _dropPanel;
    Button _replaceScriptButton;
    CheckBox _shouldReplaceAllCheckbox;
    CustomEditor _propertiesEditor;

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

        Panel replaceScriptPanel = new Panel
        {
            Parent = _dropPanel,
            Height = 64,
        };

        _replaceScriptButton = new Button
        {
            Text = "Replace Script",
            AnchorPreset = AnchorPresets.TopCenter,
            Width = 240,
            Height = 24,
            X = -120,
            Y = 0,
            Parent = replaceScriptPanel,
        };
        _replaceScriptButton.Clicked += OnReplaceScriptButtonClicked;

        Label replaceAllLabel = new Label
        {
            Text = "Replace All of Same Type",
            AnchorPreset = AnchorPresets.BottomCenter,
            Y = -34,
            Parent = replaceScriptPanel,
        };
        replaceAllLabel.X -= FlaxEngine.GUI.Style.Current.FontSmall.MeasureText(replaceAllLabel.Text).X;

        _shouldReplaceAllCheckbox = new CheckBox
        {
            TooltipText = "Wether or not to apply this script change to all scripts missing the same type.",
            AnchorPreset = AnchorPresets.BottomCenter,
            Y = -34,
            Parent = replaceScriptPanel,
        };

        float centerDifference = (_shouldReplaceAllCheckbox.Right - replaceAllLabel.Left) / 2;
        replaceAllLabel.X += centerDifference;
        _shouldReplaceAllCheckbox.X += centerDifference;

        base.Initialize(layout);
    }

    private List<MissingScript> FindActorsWithMatchingMissingScript(Actor parent = null)
    {
        List<MissingScript> missingScripts = new List<MissingScript>();
        if (parent != null)
        {
            for (int child = 0; child < parent.ChildrenCount; child++)
            {
                Actor actor = parent.Children[child];
                for (int scriptIndex = 0; scriptIndex < actor.ScriptsCount; scriptIndex++)
                {
                    Script actorScript = actor.Scripts[scriptIndex];
                    if (actorScript is not MissingScript missingActorScript)
                    {
                        continue;
                    }

                    MissingScript currentMissing = Values[0] as MissingScript;
                    if (missingActorScript.MissingTypeName != currentMissing.MissingTypeName)
                    {
                        continue;
                    }

                    Debug.Log("Found correct missing script.");

                    // Matching MissingScript.
                    missingScripts.Add(missingActorScript);
                }

                missingScripts.AddRange(FindActorsWithMatchingMissingScript(actor));
            }
        } else
        {
            foreach (Actor actor in Level.GetActors<Actor>())
            {
                missingScripts.AddRange(FindActorsWithMatchingMissingScript(actor));
            }
        }

        return missingScripts;
    }

    private void ReplaceScript(ScriptType script, bool replaceAllInScene)
    {
        var actions = new List<IUndoAction>(4);

        List<MissingScript> missingScripts = new List<MissingScript>();
        if (!replaceAllInScene)
        {
            missingScripts.Add(Values[0] as MissingScript);
        } else
        {
            missingScripts.AddRange(FindActorsWithMatchingMissingScript());
        }

        foreach (MissingScript missingScript in missingScripts)
        {
            actions.Add(AddRemoveScript.Add(missingScript.Actor, script));
            actions.Add(AddRemoveScript.Remove(missingScript));
        }

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
        {
            cm.AddItem(new TypeSearchPopup.TypeItemView(scripts[i]));
        }
        // Get the parent (actor properties editor) of the parent (Scripts Editor) of our editor.
        _propertiesEditor = ParentEditor.ParentEditor;

        cm.ItemClicked += item => ReplaceScript((ScriptType)item.Tag, _shouldReplaceAllCheckbox.Checked);
        cm.SortItems();
        cm.Show(_dropPanel, _replaceScriptButton.BottomLeft - new Float2((cm.Width - _replaceScriptButton.Width) / 2, 0));
    }
}
