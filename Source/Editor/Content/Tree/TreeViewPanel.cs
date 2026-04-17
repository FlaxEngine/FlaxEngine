using System.Collections.Generic;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Options;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content;

/// <summary>
/// The content tree view panel.
/// </summary>
public class TreeViewPanel : Panel
{
    /// <summary>
    /// The content tree assigned to this panel.
    /// </summary>
    public Tree ContentTree;
    
    private InputActionsContainer _inputActions;
    private bool _isCutting;
    private List<ContentItem> _cutItems = new List<ContentItem>();

    /// <summary>
    /// Initializes a new instance of the <see cref="TreeViewPanel"/> class.
    /// </summary>
    public TreeViewPanel()
    : base(ScrollBars.None)
    {
        // Setup input actions
        _inputActions = new InputActionsContainer(new[]
        {
            new InputActionsContainer.Binding(options => options.Rename, Rename),
            new InputActionsContainer.Binding(options => options.Delete, Delete),
            new InputActionsContainer.Binding(options => options.Duplicate, Duplicate),
            new InputActionsContainer.Binding(options => options.Copy, Copy),
            new InputActionsContainer.Binding(options => options.Paste, Paste),
            new InputActionsContainer.Binding(options => options.Cut, Cut),
        });
    }

    private void Rename()
    {
        if (ContentTree == null || !Visible)
            return;
        
        var selection = ContentTree.Selection;
        if (selection.Count > 0)
        {
            var node = selection[0];
            if (node is ContentItemTreeNode contentNode)
            {
                Editor.Instance.Windows.ContentWin.Rename(contentNode.Item);
            }
        }
    }

    private void Delete()
    {
        if (ContentTree == null || !Visible)
            return;
        
        var selection = ContentTree.Selection;
        if (selection.Count > 0)
        {
            var items = new List<ContentItem>();
            foreach (var node in selection)
            {
                if (node is ContentItemTreeNode contentNode)
                {
                    items.Add(contentNode.Item);
                }
            }

            Editor.Instance.Windows.ContentWin.Delete(items);
        }
    }
    
    private void Duplicate()
    {
        if (ContentTree == null || !Visible)
            return;

        var selection = ContentTree.Selection;
        if (selection.Count > 0)
        {
            var items = new List<ContentItem>();
            foreach (var node in selection)
            {
                if (node is ContentItemTreeNode contentNode)
                {
                    items.Add(contentNode.Item);
                }
            }

            Editor.Instance.Windows.ContentWin.Duplicate(items);
        }
    }
    
    private void Copy()
    {
        if (ContentTree == null || !Visible)
            return;

        var selection = ContentTree.Selection;
        if (selection.Count == 0)
            return;
        var filePaths = new List<string>();
        foreach (var node in selection)
            if (node is ContentItemTreeNode contentNode)
                filePaths.Add(contentNode.Item.Path);

        Clipboard.Files = filePaths.ToArray();
        UpdateContentItemCut(false);
    }
    
    private void Paste()
    {
        if (ContentTree == null || !Visible)
            return;
 
        var files = Clipboard.Files;
        if (files == null || files.Length == 0)
            return;

        Editor.Instance.Windows.ContentWin.Paste(files, _isCutting);
        UpdateContentItemCut(false);
    }

    private void Cut()
    {
        if (ContentTree == null || !Visible)
            return;

        Copy();
        UpdateContentItemCut(true);
    }
    
    private void UpdateContentItemCut(bool cut)
    {
        _isCutting = cut;
        
        // Add selection to cut list
        if (cut)
        {
            var selection = ContentTree.Selection;
            foreach (var node in selection)
            {
                if (node is ContentItemTreeNode contentNode)
                    _cutItems.Add(contentNode.Item);
            }
        }
            
        // Update item with if it is being cut.
        foreach (var item in _cutItems)
        {
            item.IsBeingCut = cut;
        }
            
        // Clean up cut items
        if (!cut)
            _cutItems.Clear();
    }

    /// <inheritdoc />
    public override bool OnKeyDown(KeyboardKeys key)
    {
        if (!Visible)
            return false;
        if (ContentTree == null)
            return base.OnKeyDown(key);

        if (_inputActions.Process(Editor.Instance, this, key))
            return true;
        
        var selection = ContentTree.Selection;
        if (selection.Count > 0)
        {
            if (key == KeyboardKeys.Return)
            {
                foreach (var node in selection)
                {
                    if (node is ContentItemTreeNode contentNode)
                        Editor.Instance.Windows.ContentWin.Open(contentNode.Item);
                }
            }
        }
        return base.OnKeyDown(key);
    }
}
