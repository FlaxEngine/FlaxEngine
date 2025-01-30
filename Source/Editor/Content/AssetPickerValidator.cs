// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.Content;

/// <summary>
/// Manages and converts the selected content item to the appropriate types. Useful for drag operations.
/// </summary>
public class AssetPickerValidator : IContentItemOwner
{
    private Asset _selected;
    private ContentItem _selectedItem;
    private ScriptType _type;
    private string _fileExtension;

    /// <summary>
    /// Gets or sets the selected item.
    /// </summary>
    public ContentItem SelectedItem
    {
        get => _selectedItem;
        set
        {
            if (_selectedItem == value)
                return;
            if (value == null)
            {
                if (_selected == null && _selectedItem is SceneItem)
                {
                    // Deselect scene reference
                    _selectedItem.RemoveReference(this);
                    _selectedItem = null;
                    _selected = null;
                    OnSelectedItemChanged();
                    return;
                }

                // Deselect
                _selectedItem?.RemoveReference(this);
                _selectedItem = null;
                _selected = null;
                OnSelectedItemChanged();
            }
            else if (value is SceneItem item)
            {
                if (_selectedItem == item)
                    return;
                if (!IsValid(item))
                    item = null;

                // Change value to scene reference (cannot load asset because scene can be already loaded - duplicated ID issue)
                _selectedItem?.RemoveReference(this);
                _selectedItem = item;
                _selected = null;
                _selectedItem?.AddReference(this);
                OnSelectedItemChanged();
            }
            else if (value is AssetItem assetItem)
            {
                SelectedAsset = FlaxEngine.Content.LoadAsync(assetItem.ID);
            }
            else
            {
                // Change value
                _selectedItem?.RemoveReference(this);
                _selectedItem = value;
                _selected = null;
                OnSelectedItemChanged();
            }
        }
    }

    /// <summary>
    /// Gets or sets the selected asset identifier.
    /// </summary>
    public Guid SelectedID
    {
        get
        {
            if (_selected != null)
                return _selected.ID;
            if (_selectedItem is AssetItem assetItem)
                return assetItem.ID;
            return Guid.Empty;
        }
        set => SelectedItem = Editor.Instance.ContentDatabase.FindAsset(value);
    }

    /// <summary>
    /// Gets or sets the selected content item path.
    /// </summary>
    public string SelectedPath
    {
        get => Utilities.Utils.ToPathProject(_selectedItem?.Path ?? _selected?.Path);
        set => SelectedItem = string.IsNullOrEmpty(value) ? null : Editor.Instance.ContentDatabase.Find(Utilities.Utils.ToPathAbsolute(value));
    }

    /// <summary>
    /// Gets or sets the selected asset object.
    /// </summary>
    public Asset SelectedAsset
    {
        get => _selected;
        set
        {
            // Check if value won't change
            if (value == _selected)
                return;

            // Find item from content database and check it
            var item = value ? Editor.Instance.ContentDatabase.FindAsset(value.ID) : null;
            if (item != null && !IsValid(item))
                item = null;

            // Change value
            _selectedItem?.RemoveReference(this);
            _selectedItem = item;
            _selected = value;
            _selectedItem?.AddReference(this);
            OnSelectedItemChanged();
        }
    }

    /// <summary>
    /// Gets or sets the assets types that this picker accepts (it supports types derived from the given type). Use <see cref="ScriptType.Null"/> for generic file picker.
    /// </summary>
    public ScriptType AssetType
    {
        get => _type;
        set
        {
            if (_type != value)
            {
                _type = value;

                // Auto deselect if the current value is invalid
                if (_selectedItem != null && !IsValid(_selectedItem))
                    SelectedItem = null;
            }
        }
    }

    /// <summary>
    /// Gets or sets the content items extensions filter. Null if unused.
    /// </summary>
    public string FileExtension
    {
        get => _fileExtension;
        set
        {
            if (_fileExtension != value)
            {
                _fileExtension = value;

                // Auto deselect if the current value is invalid
                if (_selectedItem != null && !IsValid(_selectedItem))
                    SelectedItem = null;
            }
        }
    }

    /// <summary>
    /// Occurs when selected item gets changed.
    /// </summary>
    public event Action SelectedItemChanged;

    /// <summary>
    /// The custom callback for assets validation. Cane be used to implement a rule for assets to pick.
    /// </summary>
    public Func<ContentItem, bool> CheckValid;

    /// <summary>
    /// Returns whether item is valid.
    /// </summary>
    /// <param name="item"></param>
    /// <returns></returns>
    public bool IsValid(ContentItem item)
    {
        if (_fileExtension != null && !item.Path.EndsWith(_fileExtension))
            return false;
        if (CheckValid != null && !CheckValid(item))
            return false;
        if (_type == ScriptType.Null)
            return true;

        if (item is AssetItem assetItem)
        {
            // Faster path for binary items (in-built)
            if (assetItem is BinaryAssetItem binaryItem)
                return _type.IsAssignableFrom(new ScriptType(binaryItem.Type));

            // Type filter
            var type = TypeUtils.GetType(assetItem.TypeName);
            if (_type.IsAssignableFrom(type))
                return true;

            // Json assets can contain any type of the object defined by the C# type (data oriented design)
            if (assetItem is JsonAssetItem && (_type.Type == typeof(JsonAsset) || _type.Type == typeof(Asset)))
                return true;

            // Special case for scene asset references
            if (_type.Type == typeof(SceneReference) && assetItem is SceneItem)
                return true;
        }

        return false;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="AssetPickerValidator"/> class.
    /// </summary>
    public AssetPickerValidator()
    : this(new ScriptType(typeof(Asset)))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="AssetPickerValidator"/> class.
    /// </summary>
    /// <param name="assetType">The asset types that this picker accepts.</param>
    public AssetPickerValidator(ScriptType assetType)
    {
        _type = assetType;
    }

    /// <summary>
    /// Called when selected item gets changed.
    /// </summary>
    protected virtual void OnSelectedItemChanged()
    {
        SelectedItemChanged?.Invoke();
    }

    /// <inheritdoc />
    public void OnItemDeleted(ContentItem item)
    {
        // Deselect item
        SelectedItem = null;
    }

    /// <inheritdoc />
    public void OnItemRenamed(ContentItem item)
    {
    }

    /// <inheritdoc />
    public void OnItemReimported(ContentItem item)
    {
    }

    /// <inheritdoc />
    public void OnItemDispose(ContentItem item)
    {
        // Deselect item
        SelectedItem = null;
    }

    /// <summary>
    /// Call to remove reference from the selected item.
    /// </summary>
    public void OnDestroy()
    {
        _selectedItem?.RemoveReference(this);
        _selectedItem = null;
        _selected = null;
    }
}
