// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors
{
    /// <summary>
    /// Represents a container control for <see cref="LayoutElement"/>. Can contain child elements.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    [HideInEditor]
    public abstract partial class LayoutElementsContainer : LayoutElement
    {
        /// <summary>
        /// Helper flag that is set to true if this container is in root presenter area, otherwise it's one of child groups.
        /// It's used to collapse all child groups and open the root ones by auto.
        /// </summary>
        internal bool isRootGroup = true;

        /// <summary>
        /// The children.
        /// </summary>
        public readonly List<LayoutElement> Children = new List<LayoutElement>();

        /// <summary>
        /// The child custom editors.
        /// </summary>
        public readonly List<CustomEditor> Editors = new List<CustomEditor>();

        /// <summary>
        /// Gets the control represented by this element.
        /// </summary>
        public abstract ContainerControl ContainerControl { get; }

        /// <summary>
        /// Adds new group element.
        /// </summary>
        /// <param name="title">The title.</param>
        /// <param name="linkedEditor">The custom editor to be linked for a group. Used to provide more utility functions for a drop panel UI via context menu.</param>
        /// <param name="useTransparentHeader">True if use drop down icon and transparent group header, otherwise use normal style.</param>
        /// <returns>The created element.</returns>
        public virtual GroupElement Group(string title, CustomEditor linkedEditor, bool useTransparentHeader)
        {
            var element = Group(title, useTransparentHeader);
            element.Panel.Tag = linkedEditor;
            element.Panel.MouseButtonRightClicked += OnGroupPanelMouseButtonRightClicked;
            return element;
        }

        /// <summary>
        /// Adds new group element.
        /// </summary>
        /// <param name="title">The title.</param>
        /// <param name="linkedEditor">The custom editor to be linked for a group. Used to provide more utility functions for a drop panel UI via context menu.</param>
        /// <returns>The created element.</returns>
        public virtual GroupElement Group(string title, CustomEditor linkedEditor)
        {
            var element = Group(title, linkedEditor, useTransparentHeader:false);         
            return element;
        }

        /// <summary>
        /// Adds new group element.
        /// </summary>
        /// <param name="title">The title.</param>
        /// <param name="useTransparentHeader">True if use drop down icon and transparent group header, otherwise use normal style.</param>
        /// <returns>The created element.</returns>
        public virtual GroupElement Group(string title, bool useTransparentHeader)
        {
            var element = new GroupElement();
            if (!isRootGroup)
            {
                element.Panel.Close(false);
            }
            else if (this is CustomEditorPresenter presenter && (presenter.Features & FeatureFlags.CacheExpandedGroups) != 0)
            {
                if (Editor.Instance.ProjectCache.IsCollapsedGroup(title))
                    element.Panel.Close(false);
                element.Panel.IsClosedChanged += OnPanelIsClosedChanged;
            }
            element.isRootGroup = false;
            element.Panel.HeaderText = title;
            if (useTransparentHeader)
            {
                element.Panel.EnableDropDownIcon = true;
                element.Panel.EnableContainmentLines = false;
                element.Panel.HeaderColor = element.Panel.HeaderColorMouseOver = Color.Transparent;
            }
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new group element.
        /// </summary>
        /// <param name="title">The title.</param>
        /// <returns>The created element.</returns>
        public virtual GroupElement Group(string title)
        {
            var element = Group(title, useTransparentHeader:false);
            return element;
        }

        /// <summary>
        /// Adds new group element.
        /// </summary>
        /// <param name="title">The title.</param>
        /// <param name="linkedEditor">The custom editor to be linked for a group. Used to provide more utility functions for a drop panel UI via context menu.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public GroupElement Group(string title, CustomEditor linkedEditor, Action<GroupElement> onAdd)
        {
            var e = Group(title, linkedEditor);
            onAdd?.Invoke(e);
            return e;
        }

        /// <summary>
        /// Adds new group element.
        /// </summary>
        /// <param name="title">The title.</param>
        /// <param name="useTransparentHeader">True if use drop down icon and transparent group header, otherwise use normal style.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public GroupElement Group(string title, bool useTransparentHeader, Action<GroupElement> onAdd)
        {
            var e = Group(title, useTransparentHeader);
            onAdd?.Invoke(e);
            return e;
        }

        /// <summary>
        /// Adds new group element.
        /// </summary>
        /// <param name="title">The title.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public GroupElement Group(string title, Action<GroupElement> onAdd)
        {
            var e = Group(title);
            onAdd?.Invoke(e);
            return e;
        }
     
        private void OnGroupPanelMouseButtonRightClicked(DropPanel groupPanel, Float2 location)
        {
            var linkedEditor = (CustomEditor)groupPanel.Tag;
            var menu = new ContextMenu();

            var revertToPrefab = menu.AddButton("Revert to Prefab", linkedEditor.RevertToReferenceValue);
            revertToPrefab.Enabled = linkedEditor.CanRevertReferenceValue;
            var resetToDefault = menu.AddButton("Reset to default", linkedEditor.RevertToDefaultValue);
            resetToDefault.Enabled = linkedEditor.CanRevertDefaultValue;
            menu.AddSeparator();
            menu.AddButton("Copy", linkedEditor.Copy);
            var paste = menu.AddButton("Paste", linkedEditor.Paste);
            paste.Enabled = linkedEditor.CanPaste;

            menu.Show(groupPanel, location);
        }

        private void OnPanelIsClosedChanged(DropPanel panel)
        {
            Editor.Instance.ProjectCache.SetCollapsedGroup(panel.HeaderText, panel.IsClosed);
        }


        #region HorizontalPanel

        /// <summary>
        /// Adds new horizontal panel element.
        /// </summary>
        /// <returns>The created element.</returns>
        public virtual HorizontalPanelElement HorizontalPanel()
        {
            var element = new HorizontalPanelElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new horizontal panel element.
        /// </summary>
        /// <returns>The created element.</returns>
        public HorizontalPanelElement HorizontalPanel(Action<HorizontalPanelElement> onAdd)
        {
            var element = HorizontalPanel();
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region VerticallPanel

        /// <summary>
        /// Adds new vertical panel element.
        /// </summary>
        /// <returns>The created element.</returns>
        public virtual VerticalPanelElement VerticalPanel()
        {
            var element = new VerticalPanelElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new vertical panel element.
        /// </summary>
        /// <returns>The created element.</returns>
        public VerticalPanelElement VerticalPanel(Action<VerticalPanelElement> onAdd)
        {
            var element = VerticalPanel();
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region Button

        /// <summary>
        /// Adds new button element.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="tooltip">The tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual ButtonElement Button(string text, string tooltip)
        {
            var element = new ButtonElement();
            element.Button.Text = text;
            element.Button.TooltipText = tooltip;
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new button element.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>The created element.</returns>
        public virtual ButtonElement Button(string text)
        {
            var element = Button(text, tooltip:null);          
            return element;
        }

        /// <summary>
        /// Adds new button element with custom color.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="color">The color.</param>
        /// <param name="tooltip">The tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual ButtonElement Button(string text, Color color, string tooltip)
        {
            ButtonElement element = new ButtonElement();
            element.Button.Text = text;
            element.Button.TooltipText = tooltip;
            element.Button.SetColors(color);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new button element with custom color.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="color">The color.</param>
        /// <returns>The created element.</returns>
        public virtual ButtonElement Button(string text, Color color)
        {
            var element = Button(text, color, tooltip: null);
            return element;
        }

        /// <summary>
        /// Adds new button element.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="tooltip">The tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public ButtonElement Button(string text, string tooltip, Action<ButtonElement> onAdd)
        {
            var element = Button(text, tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new button element.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public ButtonElement Button(string text, Action<ButtonElement> onAdd)
        {
            var element = Button(text);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new button element with custom color.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="color">The color.</param>
        /// <param name="tooltip">The tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public ButtonElement Button(string text, Color color, string tooltip, Action<ButtonElement> onAdd)
        {
            var element = Button(text, color, tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new button element with custom color.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="color">The color.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public ButtonElement Button(string text, Color color, Action<ButtonElement> onAdd)
        {
            var element = Button(text, color);
            onAdd?.Invoke(element);
            return element;
        }

        #endregion Button

        #region CustomElement

        /// <summary>
        /// Adds new custom element.
        /// </summary>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <returns>The created element.</returns>
        public virtual CustomElement<T> Custom<T>()
        where T : Control, new()
        {
            var element = new CustomElement<T>();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new custom element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual CustomElement<T> Custom<T>(string name, string tooltip)
        where T : Control, new()
        {
            var property = AddPropertyItem(name, tooltip);
            return property.Custom<T>();
        }

        /// <summary>
        /// Adds new custom element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <returns>The created element.</returns>
        public virtual CustomElement<T> Custom<T>(string name)
        where T : Control, new()
        {
            var property = AddPropertyItem(name, null);
            return property.Custom<T>();
        }

        /// <summary>
        /// Adds new custom element.
        /// </summary>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <returns>The created element.</returns>
        public virtual CustomElement<T> Custom<T>(Action<CustomElement<T>> onAdd)
        where T : Control, new()
        {
            var element = Custom<T>();
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new custom element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public virtual CustomElement<T> Custom<T>(string name, string tooltip, Action<CustomElement<T>> onAdd)
        where T : Control, new()
        {
            var element = Custom<T>(name, tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new custom element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public virtual CustomElement<T> Custom<T>(string name, Action<CustomElement<T>> onAdd)
        where T : Control, new()
        {
            var element = Custom<T>(name);
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region CustomContainer

        /// <summary>
        /// Adds new custom elements container.
        /// </summary>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <returns>The created element.</returns>
        public virtual CustomElementsContainer<T> CustomContainer<T>()
        where T : ContainerControl, new()
        {
            var element = new CustomElementsContainer<T>();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new custom elements container with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual CustomElementsContainer<T> CustomContainer<T>(string name, string tooltip)
        where T : ContainerControl, new()
        {
            var property = AddPropertyItem(name,tooltip);
            return property.CustomContainer<T>();
        }

        /// <summary>
        /// Adds new custom elements container with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <typeparam name="T">The custom control.</typeparam>
        /// <returns>The created element.</returns>
        public virtual CustomElementsContainer<T> CustomContainer<T>(string name)
        where T : ContainerControl, new()
        {
            var element = CustomContainer<T>(name, tooltip:null);
            return element;
        }

        #endregion

        #region Space

        /// <summary>
        /// Adds new space.
        /// </summary>
        /// <param name="height">The space height.</param>
        /// <returns>The created element.</returns>
        public virtual SpaceElement Space(float height)
        {
            var element = new SpaceElement();
            element.Init(height);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new space.
        /// </summary>
        /// <param name="height">The space height.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public virtual SpaceElement Space(float height, Action<SpaceElement> onAdd)
        {
            var element = Space(height);
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region Image

        /// <summary>
        /// Adds sprite image to the layout.
        /// </summary>
        /// <param name="sprite">The sprite.</param>
        /// <returns>The created element.</returns>
        public virtual ImageElement Image(SpriteHandle sprite)
        {
            var element = new ImageElement();
            element.Image.Brush = new SpriteBrush(sprite);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds texture image to the layout.
        /// </summary>
        /// <param name="texture">The texture.</param>
        /// <returns>The created element.</returns>
        public virtual ImageElement Image(Texture texture)
        {
            var element = new ImageElement();
            element.Image.Brush = new TextureBrush(texture);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds GPU texture image to the layout.
        /// </summary>
        /// <param name="texture">The GPU texture.</param>
        /// <returns>The created element.</returns>
        public virtual ImageElement Image(GPUTexture texture)
        {
            var element = new ImageElement();
            element.Image.Brush = new GPUTextureBrush(texture);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds sprite image to the layout.
        /// </summary>
        /// <param name="sprite">The sprite.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public virtual ImageElement Image(SpriteHandle sprite, Action<ImageElement> onAdd)
        {
            var element = Image(sprite);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds texture image to the layout.
        /// </summary>
        /// <param name="texture">The texture.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public virtual ImageElement Image(Texture texture, Action<ImageElement> onAdd)
        {
            var element = Image(texture);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds GPU texture image to the layout.
        /// </summary>
        /// <param name="texture">The GPU texture.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public virtual ImageElement Image(GPUTexture texture, Action<ImageElement> onAdd)
        {
            var element = Image(texture);
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region Header

        /// <summary>
        /// Adds new header control.
        /// </summary>
        /// <param name="text">The header text.</param>
        /// <returns>The created element.</returns>
        public virtual LabelElement Header(string text)
        {
            var element = Label(text);
            element.Label.Font = new FontReference(Style.Current.FontLarge);
            return element;
        }

        internal virtual LabelElement Header(HeaderAttribute header)
        {
            var element = Header(header.Text);
            if (header.FontSize != -1)
                element.Label.Font = new FontReference(element.Label.Font.Font, header.FontSize);
            if (header.Color != 0)
                element.Label.TextColor = Color.FromRGBA(header.Color);
            return element;
        }

        /// <summary>
        /// Adds new header control.
        /// </summary>
        /// <param name="text">The header text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public virtual LabelElement Header(string text, Action<LabelElement> onAdd)
        {
            var element = Header(text);
            onAdd?.Invoke(element);
            return element;
        }

        internal virtual LabelElement Header(HeaderAttribute header, Action<LabelElement> onAdd)
        {
            var element = Header(header);
            onAdd?.Invoke(element);
            return element;
        }


        #endregion

        #region TextBox

        /// <summary>
        /// Adds new text box element.
        /// </summary>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        /// <returns>The created element.</returns>
        public virtual TextBoxElement TextBox(bool isMultiline)
        {
            var element = new TextBoxElement(isMultiline);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new text box element.
        /// </summary>
        /// <returns>The created element.</returns>
        public virtual TextBoxElement TextBox()
        {
            var element = TextBox(false);
            return element;
        }

        /// <summary>
        /// Adds new text box element.
        /// </summary>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public TextBoxElement TextBox(bool isMultiline, Action<TextBoxElement> onAdd)
        {
            var element = TextBox(isMultiline);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new text box element.
        /// </summary>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public TextBoxElement TextBox(Action<TextBoxElement> onAdd)
        {
            var element = TextBox();
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region Checkbox

        /// <summary>
        /// Adds new check box element.
        /// </summary>
        /// <returns>The created element.</returns>
        public virtual CheckBoxElement Checkbox()
        {
            var element = new CheckBoxElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new check box element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual CheckBoxElement Checkbox(string name, string tooltip)
        {
            var property = AddPropertyItem(name:name, tooltip:tooltip);
            return property.Checkbox();
        }

        /// <summary>
        /// Adds new check box element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The created element.</returns>
        public virtual CheckBoxElement Checkbox(string name)
        {
            var element = Checkbox(name:name, tooltip:null);
            return element;
        }

        /// <summary>
        /// Adds new check box element.
        /// </summary>
        /// <returns>The created element.</returns>
        public CheckBoxElement Checkbox(Action<CheckBoxElement> onAdd)
        {
            var element = Checkbox();
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new check box element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CheckBoxElement Checkbox(string name, string tooltip, Action<CheckBoxElement> onAdd)
        {
            var element = Checkbox(name,tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new check box element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CheckBoxElement Checkbox(string name, Action<CheckBoxElement> onAdd)
        {
            var element = Checkbox(name);
            onAdd?.Invoke(element);
            return element;
        }


        #endregion

        #region Tree

        /// <summary>
        /// Adds new tree element.
        /// </summary>
        /// <returns>The created element.</returns>
        public virtual TreeElement Tree()
        {
            var element = new TreeElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new tree element.
        /// </summary>
        /// <returns>The created element.</returns>
        public TreeElement Tree(Action<TreeElement> onAdd)
        {
            var element = Tree();
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region Label

        /// <summary>
        /// Adds new label element.
        /// </summary>
        /// <param name="text">The label text.</param>
        /// <param name="horizontalAlignment">The label text horizontal alignment.</param>
        /// <returns>The created element.</returns>
        public virtual LabelElement Label(string text, TextAlignment horizontalAlignment)
        {
            var element = new LabelElement();
            element.Label.Text = text;
            element.Label.HorizontalAlignment = horizontalAlignment;
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new label element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="text">The label text.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual LabelElement Label(string name, string text, string tooltip)
        {
            var property = AddPropertyItem(name:name, tooltip:tooltip);
            return property.Label(text);
        }

        /// <summary>
        /// Adds new label element.
        /// </summary>
        /// <param name="text">The label text.</param>
        /// <returns>The created element.</returns>
        public virtual LabelElement Label(string text)
        {
            var element = Label(text:text, horizontalAlignment:TextAlignment.Near);      
            return element;
        }

        /// <summary>
        /// Adds new label element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="text">The label text.</param>
        /// <returns>The created element.</returns>
        public virtual LabelElement Label(string name, string text)
        {
            var element = Label(name:name, text:text, tooltip:null);
            return element;
        }

        /// <summary>
        /// Adds new label element.
        /// </summary>
        /// <param name="text">The label text.</param>
        /// <param name="horizontalAlignment">The label text horizontal alignment.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public LabelElement Label(string text, TextAlignment horizontalAlignment, Action<LabelElement> onAdd)
        {
            var element = Label(text, horizontalAlignment);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new label element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="text">The label text.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public LabelElement Label(string name, string text, string tooltip, Action<LabelElement> onAdd)
        {
            var element = Label(name, text, tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new label element.
        /// </summary>
        /// <param name="text">The label text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public LabelElement Label(string text, Action<LabelElement> onAdd)
        {
            var element = Label(text);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new label element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="text">The label text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public LabelElement Label(string name, string text, Action<LabelElement> onAdd)
        {
            var element = Label(name, text);
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region ClickableLabel

        /// <summary>
        /// Adds new label element.
        /// </summary>
        /// <param name="text">The label text.</param>
        /// <param name="horizontalAlignment">The label text horizontal alignment.</param>
        /// <returns>The created element.</returns>
        public virtual CustomElement<ClickableLabel> ClickableLabel(string text, TextAlignment horizontalAlignment)
        {
            var element = new CustomElement<ClickableLabel>();
            element.CustomControl.Height = 18.0f;
            element.CustomControl.Text = text;
            element.CustomControl.HorizontalAlignment = horizontalAlignment;
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new label element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="text">The label text.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual CustomElement<ClickableLabel> ClickableLabel(string name, string text, string tooltip)
        {
            var property = AddPropertyItem(name:name, tooltip:tooltip);
            return property.ClickableLabel(text);
        }

        /// <summary>
        /// Adds new label element.
        /// </summary>
        /// <param name="text">The label text.</param>
        /// <returns>The created element.</returns>
        public virtual CustomElement<ClickableLabel> ClickableLabel(string text)
        {
            var element = ClickableLabel(text:text, horizontalAlignment:TextAlignment.Near);        
            return element;
        }

        /// <summary>
        /// Adds new label element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="text">The label text.</param>
        /// <returns>The created element.</returns>
        public virtual CustomElement<ClickableLabel> ClickableLabel(string name, string text)
        {
            var element = ClickableLabel(name:name, text:text, tooltip:null);
            return element;
        }

        /// <summary>
        /// Adds new label element.
        /// </summary>
        /// <param name="text">The label text.</param>
        /// <param name="horizontalAlignment">The label text horizontal alignment.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomElement<ClickableLabel> ClickableLabel(string text, TextAlignment horizontalAlignment, Action<CustomElement<ClickableLabel>> onAdd)
        {
            var element = ClickableLabel(text:text, horizontalAlignment:horizontalAlignment);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new label element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="text">The label text.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomElement<ClickableLabel> ClickableLabel(string name, string text, string tooltip, Action<CustomElement<ClickableLabel>> onAdd)
        {
            var element = ClickableLabel(name:name, text:text, tooltip:tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new label element.
        /// </summary>
        /// <param name="text">The label text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomElement<ClickableLabel> ClickableLabel(string text, Action<CustomElement<ClickableLabel>> onAdd)
        {
            var element = ClickableLabel(text:text);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new label element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="text">The label text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomElement<ClickableLabel> ClickableLabel(string name, string text, Action<CustomElement<ClickableLabel>> onAdd)
        {
            var element = ClickableLabel(name:name, text:text);
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region Float

        /// <summary>
        /// Adds new float value element.
        /// </summary>
        /// <returns>The created element.</returns>
        public virtual FloatValueElement FloatValue()
        {
            var element = new FloatValueElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new float value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual FloatValueElement FloatValue(string name, string tooltip)
        {
            var property = AddPropertyItem(name, tooltip);
            return property.FloatValue();
        }

        /// <summary>
        /// Adds new float value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The created element.</returns>
        public virtual FloatValueElement FloatValue(string name)
        {
            var element = FloatValue(name, tooltip:null);
            return element;
        }

        /// <summary>
        /// Adds new float value element.
        /// </summary>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public FloatValueElement FloatValue(Action<FloatValueElement> onAdd)
        {
            var element = FloatValue();
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new float value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public FloatValueElement FloatValue(string name, string tooltip, Action<FloatValueElement> onAdd)
        {
            var element = FloatValue(name,tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new float value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public FloatValueElement FloatValue(string name, Action<FloatValueElement> onAdd)
        {
            var element = FloatValue(name);
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region Double

        /// <summary>
        /// Adds new double value element.
        /// </summary>
        /// <returns>The created element.</returns>
        public virtual DoubleValueElement DoubleValue()
        {
            var element = new DoubleValueElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new double value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual DoubleValueElement DoubleValue(string name, string tooltip)
        {
            var property = AddPropertyItem(name, tooltip);
            return property.DoubleValue();
        }

        /// <summary>
        /// Adds new double value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The created element.</returns>
        public virtual DoubleValueElement DoubleValue(string name)
        {
            var element = DoubleValue(name, tooltip: null);
            return element;
        }

        /// <summary>
        /// Adds new double value element.
        /// </summary>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public DoubleValueElement DoubleValue(Action<DoubleValueElement> onAdd)
        {
            var element = DoubleValue();
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new double value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public DoubleValueElement DoubleValue(string name, string tooltip, Action<DoubleValueElement> onAdd)
        {
            var element = DoubleValue(name,tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new double value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public DoubleValueElement DoubleValue(string name, Action<DoubleValueElement> onAdd)
        {
            var element = DoubleValue(name);
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region Slide

        /// <summary>
        /// Adds new slider element.
        /// </summary>
        /// <returns>The created element.</returns>
        public virtual SliderElement Slider()
        {
            var element = new SliderElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new slider element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual SliderElement Slider(string name, string tooltip)
        {
            var property = AddPropertyItem(name:name, tooltip:tooltip);
            return property.Slider();
        }

        /// <summary>
        /// Adds new slider element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>

        /// <returns>The created element.</returns>
        public virtual SliderElement Slider(string name)
        {
            var element = Slider(name:name, tooltip:null);
            return element;
        }


        /// <summary>
        /// Adds new slider element.
        /// </summary>
        /// <returns>The created element.</returns>
        public SliderElement Slider(Action<SliderElement> onAdd)
        {
            var element = Slider();
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new slider element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public SliderElement Slider(string name, string tooltip, Action<SliderElement> onAdd)
        {
            var element = Slider(name:name, tooltip:tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new slider element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="onAdd">Invoke after child is added.</param>

        /// <returns>The created element.</returns>
        public SliderElement Slider(string name, Action<SliderElement> onAdd)
        {
            var element = Slider(name:name);
            onAdd?.Invoke(element);
            return element;
        }


        #endregion

        #region SignedIntegerValue

        /// <summary>
        /// Adds new signed integer (up to long range) value element.
        /// </summary>
        /// <returns>The created element.</returns>
        public virtual SignedIntegerValueElement SignedIntegerValue()
        {
            var element = new SignedIntegerValueElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new signed integer (up to long range) value element.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual SignedIntegerValueElement SignedIntegerValue(string name, string tooltip)
        {
            var property = AddPropertyItem(name:name, tooltip:tooltip);
            return property.SignedIntegerValue();
        }

        /// <summary>
        /// Adds new signed integer (up to long range) value element.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The created element.</returns>
        public virtual SignedIntegerValueElement SignedIntegerValue(string name)
        {
            var element = SignedIntegerValue(name:name, tooltip:null);
            return element;
        }

        /// <summary>
        /// Adds new signed integer (up to long range) value element.
        /// </summary>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public SignedIntegerValueElement SignedIntegerValue(Action<SignedIntegerValueElement> onAdd)
        {
            var element = SignedIntegerValue();
            onAdd.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new signed integer (up to long range) value element.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public SignedIntegerValueElement SignedIntegerValue(string name, string tooltip, Action<SignedIntegerValueElement> onAdd)
        {
            var element = SignedIntegerValue(name:name, tooltip:tooltip);
            onAdd.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new signed integer (up to long range) value element.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public SignedIntegerValueElement SignedIntegerValue(string name, Action<SignedIntegerValueElement> onAdd)
        {
            var element = SignedIntegerValue(name:name);
            onAdd.Invoke(element);
            return element;
        }


        #endregion

        #region UnsignedIntegerValueElement

        /// <summary>
        /// Adds new unsigned signed integer (up to ulong range) value element.
        /// </summary>
        /// <returns>The created element.</returns>
        public virtual UnsignedIntegerValueElement UnsignedIntegerValue()
        {
            var element = new UnsignedIntegerValueElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new unsigned signed integer (up to ulong range) value element.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual UnsignedIntegerValueElement UnsignedIntegerValue(string name, string tooltip)
        {
            var property = AddPropertyItem(name:name, tooltip:tooltip);
            return property.UnsignedIntegerValue();
        }

        /// <summary>
        /// Adds new unsigned signed integer (up to ulong range) value element.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The created element.</returns>
        public virtual UnsignedIntegerValueElement UnsignedIntegerValue(string name)
        {
            var property = AddPropertyItem(name:name, tooltip:null);
            return property.UnsignedIntegerValue();
        }

        /// <summary>
        /// Adds new unsigned signed integer (up to ulong range) value element.
        /// </summary>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public UnsignedIntegerValueElement UnsignedIntegerValue(Action<UnsignedIntegerValueElement> onAdd)
        {
            var element = UnsignedIntegerValue();
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new unsigned signed integer (up to ulong range) value element.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public UnsignedIntegerValueElement UnsignedIntegerValue(string name, string tooltip, Action<UnsignedIntegerValueElement> onAdd)
        {
            var element = UnsignedIntegerValue(name:name, tooltip:tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new unsigned signed integer (up to ulong range) value element.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public UnsignedIntegerValueElement UnsignedIntegerValue(string name, Action<UnsignedIntegerValueElement> onAdd)
        {
            var element = UnsignedIntegerValue(name: name);
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region IntegerValue

        /// <summary>
        /// Adds new integer value element.
        /// </summary>
        /// <returns>The created element.</returns>
        public virtual IntegerValueElement IntegerValue()
        {
            var element = new IntegerValueElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new integer value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual IntegerValueElement IntegerValue(string name, string tooltip)
        {
            var property = AddPropertyItem(name:name, tooltip:tooltip);
            return property.IntegerValue();
        }

        /// <summary>
        /// Adds new integer value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The created element.</returns>
        public virtual IntegerValueElement IntegerValue(string name)
        {
            var element = IntegerValue(name:name, tooltip:null);
            return element;
        }

        /// <summary>
        /// Adds new integer value element.
        /// </summary>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public IntegerValueElement IntegerValue(Action<IntegerValueElement> onAdd)
        {
            var element = IntegerValue();
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new integer value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public IntegerValueElement IntegerValue(string name, string tooltip, Action<IntegerValueElement> onAdd)
        {
            var element = IntegerValue(name:name, tooltip:tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new integer value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public virtual IntegerValueElement IntegerValue(string name, Action<IntegerValueElement> onAdd)
        {
            var element = IntegerValue(name: name);
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region ComboBox

        /// <summary>
        /// Adds new combobox element.
        /// </summary>
        /// <returns>The created element.</returns>
        public virtual ComboBoxElement ComboBox()
        {
            var element = new ComboBoxElement();
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new combobox element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual ComboBoxElement ComboBox(string name, string tooltip)
        {
            var property = AddPropertyItem(name: name, tooltip:tooltip);
            return property.ComboBox();
        }

        /// <summary>
        /// Adds new combobox element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The created element.</returns>
        public virtual ComboBoxElement ComboBox(string name)
        {
            var element = ComboBox(name:name, tooltip:null);
            return element;
        }

        /// <summary>
        /// Adds new combobox element.
        /// </summary>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public ComboBoxElement ComboBox(Action<ComboBoxElement> onAdd)
        {
            var element = ComboBox();
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new combobox element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public ComboBoxElement ComboBox(string name, string tooltip, Action<ComboBoxElement> onAdd)
        {
            var element = ComboBox(name:name, tooltip:tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new combobox element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public ComboBoxElement ComboBox(string name, Action<ComboBoxElement> onAdd)
        {
            var element = ComboBox(name:name);
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region Enum

        /// <summary>
        /// Adds new enum value element.
        /// </summary>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <param name="formatMode">The formatting mode.</param>
        /// <returns>The created element.</returns>
        public virtual EnumElement Enum(Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate, EnumDisplayAttribute.FormatMode formatMode)
        {
            var element = new EnumElement(type:type, customBuildEntriesDelegate:customBuildEntriesDelegate, formatMode:formatMode);
            OnAddElement(element);
            return element;
        }

        /// <summary>
        /// Adds new enum value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="formatMode">The formatting mode.</param>
        /// <returns>The created element.</returns>
        public virtual EnumElement Enum(string name, Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate, string tooltip, EnumDisplayAttribute.FormatMode formatMode)
        {
            var property = AddPropertyItem(name:name, tooltip:tooltip);
            return property.Enum(type, customBuildEntriesDelegate, formatMode);
        }

        /// <summary>
        /// Adds new enum value element.
        /// </summary>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <returns>The created element.</returns>
        public virtual EnumElement Enum(Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate)
        {
            var element = Enum(type:type, customBuildEntriesDelegate:customBuildEntriesDelegate, formatMode: EnumDisplayAttribute.FormatMode.Default);
            return element;
        }

        /// <summary>
        /// Adds new enum value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual EnumElement Enum(string name, Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate, string tooltip)
        {
            var element = Enum(name:name, type:type, customBuildEntriesDelegate:customBuildEntriesDelegate, tooltip:tooltip, formatMode:EnumDisplayAttribute.FormatMode.Default);
            return element;
        }

        /// <summary>
        /// Adds new enum value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <returns>The created element.</returns>
        public virtual EnumElement Enum(string name, Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate)
        {
            var element = Enum(name:name, type:type, customBuildEntriesDelegate:customBuildEntriesDelegate, tooltip:null, formatMode:EnumDisplayAttribute.FormatMode.Default);
            return element;
        }

        /// <summary>
        /// Adds new enum value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="type">The enum type.</param>
        /// <returns>The created element.</returns>
        public virtual EnumElement Enum(string name, Type type)
        {
            var element = Enum(name, type, customBuildEntriesDelegate: null, tooltip: null);
            return element;
        }

        /// <summary>
        /// Adds new enum value element.
        /// </summary>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <param name="formatMode">The formatting mode.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public EnumElement Enum(Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate, EnumDisplayAttribute.FormatMode formatMode, Action<EnumElement> onAdd)
        {
            var element = Enum(type: type, customBuildEntriesDelegate: customBuildEntriesDelegate, formatMode: formatMode);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new enum value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="formatMode">The formatting mode.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public EnumElement Enum(string name, Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate, string tooltip, EnumDisplayAttribute.FormatMode formatMode, Action<EnumElement> onAdd)
        {
            var element = Enum(name:name, type:type, customBuildEntriesDelegate:customBuildEntriesDelegate, tooltip:tooltip, formatMode:formatMode);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new enum value element.
        /// </summary>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public EnumElement Enum(Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate, Action<EnumElement> onAdd)
        {
            var element = Enum(type: type, customBuildEntriesDelegate: customBuildEntriesDelegate);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new enum value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public EnumElement Enum(string name, Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate, string tooltip, Action<EnumElement> onAdd)
        {
            var element = Enum(name: name, type: type, customBuildEntriesDelegate: customBuildEntriesDelegate, tooltip: tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new enum value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public EnumElement Enum(string name, Type type, EnumComboBox.BuildEntriesDelegate customBuildEntriesDelegate, Action<EnumElement> onAdd)
        {
            var element = Enum(name: name, type: type, customBuildEntriesDelegate: customBuildEntriesDelegate);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds new enum value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="type">The enum type.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public EnumElement Enum(string name, Type type, Action<EnumElement> onAdd)
        {
            var element = Enum(name: name, type: type);
            onAdd?.Invoke(element);
            return element;
        }


        #endregion

        #region Object

        /// <summary>
        /// Adds object(s) editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Object(ValueContainer values, CustomEditor overrideEditor)
        {
            if (values == null)
                throw new ArgumentNullException();

            var editor = CustomEditorsUtil.CreateEditor(values, overrideEditor);

            OnAddEditor(editor);
            editor.Initialize(CustomEditor.CurrentCustomEditor.Presenter, this, values);

            return editor;
        }

        /// <summary>
        /// Adds object(s) editor with name label. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Object(string name, ValueContainer values, CustomEditor overrideEditor, string tooltip)
        {
            var property = AddPropertyItem(name:name, tooltip:tooltip);
            return property.Object(values, overrideEditor);
        }

        /// <summary>
        /// Adds object(s) editor with name label. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Object(PropertyNameLabel label, ValueContainer values, CustomEditor overrideEditor, string tooltip)
        {
            var property = AddPropertyItem(label:label, tooltip:tooltip);
            return property.Object(values:values, overrideEditor:overrideEditor);
        }

        /// <summary>
        /// Adds object(s) editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="values">The values.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Object(ValueContainer values)
        {
            var element = Object(values:values, overrideEditor:null);
            return element;
        }

        /// <summary>
        /// Adds object(s) editor with name label. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Object(string name, ValueContainer values, CustomEditor overrideEditor)
        {
            var element = Object(name:name, values:values, overrideEditor:overrideEditor, tooltip: null);
            return element;
        }

        /// <summary>
        /// Adds object(s) editor with name label. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Object(PropertyNameLabel label, ValueContainer values)
        {
            var element = Object(label:label, values:values, overrideEditor: null, tooltip: null);
            return element;
        }

        /// <summary>
        /// Adds object(s) editor with name label. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Object(PropertyNameLabel label, ValueContainer values, CustomEditor overrideEditor)
        {
            var element = Object(label:label, values:values, overrideEditor: overrideEditor, tooltip: null);
            return element;
        }

        /// <summary>
        /// Adds object(s) editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Object(ValueContainer values, CustomEditor overrideEditor, Action<CustomEditor> onAdd)
        {
            var element = Object(values: values, overrideEditor: overrideEditor);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds object(s) editor with name label. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Object(string name, ValueContainer values, CustomEditor overrideEditor, string tooltip, Action<CustomEditor> onAdd)
        {
            var element = Object(name:name, values: values, overrideEditor: overrideEditor, tooltip: tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds object(s) editor with name label. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Object(PropertyNameLabel label, ValueContainer values, CustomEditor overrideEditor, string tooltip, Action<CustomEditor> onAdd)
        {
            var element = Object(label:label,values:values, overrideEditor:overrideEditor,tooltip:tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds object(s) editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="values">The values.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Object(ValueContainer values, Action<CustomEditor> onAdd)
        {
            var element = Object(values: values);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds object(s) editor with name label. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Object(string name, ValueContainer values, CustomEditor overrideEditor, Action<CustomEditor> onAdd)
        {
            var element = Object(name:name, values: values, overrideEditor: overrideEditor);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds object(s) editor with name label. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Object(PropertyNameLabel label, ValueContainer values, Action<CustomEditor> onAdd)
        {
            var element = Object(label: label, values: values);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds object(s) editor with name label. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Object(PropertyNameLabel label, ValueContainer values, CustomEditor overrideEditor, Action<CustomEditor> onAdd)
        {
            var element = Object(label: label, values: values, overrideEditor: overrideEditor);
            onAdd?.Invoke(element);
            return element;
        }

        #endregion

        #region Property

        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Property(string name, ValueContainer values, CustomEditor overrideEditor, string tooltip)
        {
            var editor = CustomEditorsUtil.CreateEditor(values, overrideEditor);
            var style = editor.Style;

            if (style == DisplayStyle.InlineIntoParent || name == EditorDisplayAttribute.InlineStyle)
            {
                return Object(values, editor);
            }

            if (style == DisplayStyle.Group)
            {
                var group = Group(name, editor, true);
                group.Panel.Close(false);
                group.Panel.TooltipText = tooltip;
                return group.Object(values, editor);
            }

            var property = AddPropertyItem(name, tooltip);
            return property.Object(values, editor);
        }

        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Property(PropertyNameLabel label, ValueContainer values, CustomEditor overrideEditor, string tooltip)
        {
            var editor = CustomEditorsUtil.CreateEditor(values, overrideEditor);
            var style = editor.Style;

            if (style == DisplayStyle.InlineIntoParent)
            {
                return Object(values, editor);
            }

            if (style == DisplayStyle.Group)
            {
                var group = Group(label.Text, editor, true);
                group.Panel.Close(false);
                group.Panel.TooltipText = tooltip;
                return group.Object(values, editor);
            }

            var property = AddPropertyItem(label, tooltip);
            return property.Object(values, editor);
        }

        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Property(string name, ValueContainer values, CustomEditor overrideEditor) 
        {
            var element = Property(name:name, values:values, overrideEditor:overrideEditor, tooltip: null);
            return element;
        }

        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="values">The values.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Property(string name, ValueContainer values)
        {
            var element = Property(name:name, values:values, overrideEditor: null, tooltip: null);
            return element;
        }

        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Property(PropertyNameLabel label, ValueContainer values, CustomEditor overrideEditor)
        {
            var element = Property(label:label, values:values, overrideEditor:overrideEditor, tooltip: null);
            return element;
        }


        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Property(PropertyNameLabel label, ValueContainer values)
        {
            var element = Property(label:label, values:values, overrideEditor: null, tooltip: null);
            return element;
        }

        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Property(string name, ValueContainer values, CustomEditor overrideEditor, string tooltip, Action<CustomEditor> onAdd)
        {
            var element = Property(name: name, values: values, overrideEditor: overrideEditor, tooltip: tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Property(PropertyNameLabel label, ValueContainer values, CustomEditor overrideEditor, string tooltip, Action<CustomEditor> onAdd)
        {
            var element = Property(label:label, values: values, overrideEditor: overrideEditor, tooltip: tooltip);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Property(string name, ValueContainer values, CustomEditor overrideEditor, Action<CustomEditor> onAdd)
        {
            var element = Property(name: name, values: values, overrideEditor: overrideEditor);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <param name="values">The values.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Property(string name, ValueContainer values, Action<CustomEditor> onAdd)
        {
            var element = Property(name: name, values: values);
            onAdd?.Invoke(element);
            return element;
        }

        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <param name="overrideEditor">The custom editor to use. If null will detect it by auto.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Property(PropertyNameLabel label, ValueContainer values, CustomEditor overrideEditor, Action<CustomEditor> onAdd)
        {
            var element = Property(label: label, values: values, overrideEditor: overrideEditor);
            onAdd?.Invoke(element);
            return element;
        }


        /// <summary>
        /// Adds object property editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="values">The values.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public CustomEditor Property(PropertyNameLabel label, ValueContainer values, Action<CustomEditor> onAdd)
        {
            var element = Property(label: label, values: values);
            onAdd?.Invoke(element);
            return element;
        }


        #endregion

        #region Element

        /// <summary>
        /// Adds a new element
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns>The created element.</returns>
        public virtual T Element<T>() 
            where T : LayoutElement
        {
            var element = Activator.CreateInstance(typeof(T), true) as T;
            AddElement(element);
            return element;
        }

        /// <summary>
        /// Adds a new element
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public virtual T Element<T>(Action<T> onAdd)
           where T : LayoutElement
        {
            var element = Activator.CreateInstance(typeof(T), true) as T;
            AddElement(element,onAdd);
            return element;
        }

        #endregion

        #region Split

        /// <summary>
        /// Adds a new split container element
        /// </summary>
        /// <typeparam name="TElement1"></typeparam>
        /// <typeparam name="TElement2"></typeparam>
        /// <param name="orientation">The orientation.</param>  
        /// <returns>The created element.</returns>
        public SplitElement<TElement1,TElement2> SplitContainer<TElement1, TElement2>(Orientation orientation) 
            where TElement1 : LayoutElement
            where TElement2 : LayoutElement
        {
            var element = new SplitElement<TElement1, TElement2>(orientation);
            AddElement(element);
            return element;
        }

        /// <summary>
        /// Adds a new split container element
        /// </summary>
        /// <typeparam name="TElement1"></typeparam>
        /// <typeparam name="TElement2"></typeparam>
        /// <param name="orientation">The orientation.</param>  
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public SplitElement<TElement1, TElement2> SplitContainer<TElement1, TElement2>(Orientation orientation, Action<SplitElement<TElement1, TElement2>> onAdd)
            where TElement1 : LayoutElement
            where TElement2 : LayoutElement
        {
            var element = new SplitElement<TElement1, TElement2>(orientation);
            AddElement(element,onAdd);
            return element;
        }

        /// <summary>
        /// Adds a new split container element
        /// </summary>
        /// <typeparam name="TElement1"></typeparam>
        /// <typeparam name="TElement2"></typeparam>
        /// <returns>The created element.</returns>
        public SplitElement<TElement1, TElement2> SplitContainer
            <TElement1, TElement2>()
           where TElement1 : LayoutElement
           where TElement2 : LayoutElement
        {
            var element = SplitContainer<TElement1, TElement2>(Orientation.Horizontal);         
            return element;
        }

        /// <summary>
        /// Adds a new split container element
        /// </summary>
        /// <typeparam name="TElement1"></typeparam>
        /// <typeparam name="TElement2"></typeparam>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public SplitElement<TElement1, TElement2> SplitContainer<TElement1, TElement2>(Action<SplitElement<TElement1, TElement2>> onAdd)
            where TElement1 : LayoutElement
            where TElement2 : LayoutElement
        {
            var element = SplitContainer<TElement1, TElement2>(Orientation.Horizontal, onAdd);
            return element;
        }

        /// <summary>
        /// Adds a new split container element
        /// </summary>
        /// <typeparam name="TElement"></typeparam>
        /// <param name="orientation">The orientation.</param>  
        /// <returns>The created element.</returns>
        public SplitElement<TElement, TElement> SplitContainer<TElement>(Orientation orientation)
            where TElement : LayoutElement
        {
            var element = SplitContainer<TElement, TElement>(orientation);
            return element;
        }

        /// <summary>
        /// Adds a new split container element
        /// </summary>
        /// <typeparam name="TElement"></typeparam>
        /// <param name="orientation">The orientation.</param>  
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public SplitElement<TElement, TElement> SplitContainer<TElement>(Orientation orientation, Action<SplitElement<TElement, TElement>> onAdd)
            where TElement : LayoutElement
        {
            var element = SplitContainer<TElement, TElement>(orientation, onAdd);
            return element;
        }

        /// <summary>
        /// Adds a new split container element
        /// </summary>
        /// <typeparam name="TElement"></typeparam>
        /// <returns>The created element.</returns>
        public SplitElement<TElement, TElement> SplitContainer<TElement>()
            where TElement : LayoutElement
        {
            var element = SplitContainer<TElement, TElement>(Orientation.Horizontal);
            return element;
        }

        /// <summary>
        /// Adds a new split container element
        /// </summary>
        /// <typeparam name="TElement"></typeparam>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public SplitElement<TElement, TElement> SplitContainer<TElement>(Action<SplitElement<TElement, TElement>> onAdd)
            where TElement : LayoutElement
        {
            var element = SplitContainer<TElement,TElement>(Orientation.Horizontal, onAdd);
            return element;
        }

        #endregion

        #region SplitPanel

        /// <summary>
        /// Adds a new split panel element
        /// </summary>    
        /// <param name="orientation">The orientation.</param>  
        /// <returns>The created element.</returns>
        public SplitPanelElement SplitPanel(Orientation orientation)         
        {
            var element = new SplitPanelElement(orientation);
            AddElement(element);
            return element;
        }

        /// <summary>
        /// Adds a new split panel element
        /// </summary>    
        /// <param name="orientation">The orientation.</param>  
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public SplitPanelElement SplitPanel(Orientation orientation, Action<SplitPanelElement> onAdd)
        {
            var element = new SplitPanelElement(orientation);
            AddElement(element,onAdd);
            return element;
        }

        /// <summary>
        /// Adds a new split panel element
        /// </summary>    
        /// <returns>The created element.</returns>
        public SplitPanelElement SplitPanel()
        {
            var element = SplitPanel(Orientation.Horizontal);
            return element;
        }

        /// <summary>
        /// Adds a new split panel element
        /// </summary>    
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns>The created element.</returns>
        public SplitPanelElement SplitPanel(Action<SplitPanelElement> onAdd)
        {
            var element = SplitPanel(Orientation.Horizontal, onAdd);
            return element;
        }

        #endregion


        private PropertiesListElement AddPropertyItem()
        {
            // Try to reuse previous control
            PropertiesListElement element;
            if (Children.Count > 0 && Children[Children.Count - 1] is PropertiesListElement propertiesListElement)
            {
                element = propertiesListElement;
            }
            else
            {
                element = new PropertiesListElement();
                OnAddElement(element);
            }
            return element;
        }

        /// <summary>
        /// Adds the <see cref="PropertiesListElement"/> to the current layout or reuses the previous one. Used to inject properties.
        /// </summary>
        /// <param name="name">The property label name.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The element.</returns>
        public virtual PropertiesListElement AddPropertyItem(string name, string tooltip = null)
        {
            var element = AddPropertyItem();
            element.OnAddProperty(name, tooltip);
            return element;
        }

        /// <summary>
        /// Adds the <see cref="PropertiesListElement"/> to the current layout or reuses the previous one. Used to inject properties.
        /// </summary>
        /// <param name="label">The property label.</param>
        /// <param name="tooltip">The property label tooltip text.</param>
        /// <returns>The element.</returns>
        public virtual PropertiesListElement AddPropertyItem(PropertyNameLabel label, string tooltip = null)
        {
            if (label == null)
                throw new ArgumentNullException();

            var element = AddPropertyItem();
            element.OnAddProperty(label, tooltip);
            return element;
        }

        /// <summary>
        /// Adds custom element to the layout.
        /// </summary>
        /// <param name="element">The element.</param>
        public void AddElement(LayoutElement element)
        {
            if (element == null)
                throw new ArgumentNullException();
            OnAddElement(element);
        }

        /// <summary>
        /// Adds custom element to the layout.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="element">The element.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        public void AddElement<T>(T element, Action<T> onAdd) where T : LayoutElement
        {
            AddElement(element);
            onAdd?.Invoke(element);
        }

        /// <summary>
        /// Adds custom element to the layout.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="element">The element.</param>
        /// <returns></returns>
        public T Addchild<T>(T element) where T : LayoutElement
        {
            AddElement(element); 
            return element;
        }

        /// <summary>
        /// Adds custom element to the layout.
        /// </summary>
        /// <param name="element">The element.</param>
        /// <param name="onAdd">Invoke after child is added.</param>
        /// <returns></returns>
        public T Addchild<T>(T element, Action<T> onAdd) where T : LayoutElement
        {
            AddElement(element,onAdd);
            return element;
        }

        /// <summary>
        /// Called when element is added to the layout.
        /// </summary>
        /// <param name="element">The element.</param>
        protected virtual void OnAddElement(LayoutElement element)
        {
            element.Control.Parent = ContainerControl;
            Children.Add(element);
        }

        /// <summary>
        /// Called when editor is added.
        /// </summary>
        /// <param name="editor">The editor.</param>
        protected virtual void OnAddEditor(CustomEditor editor)
        {
            // This could be passed by the calling code but it's easier to hide it from the user
            // Note: we need that custom editor to link generated editor into the parent
            var customEditor = CustomEditor.CurrentCustomEditor;
            Assert.IsNotNull(customEditor);
            customEditor.OnChildCreated(editor);
            Editors.Add(editor);
        }

        /// <summary>
        /// Clears the layout.
        /// </summary>
        public virtual void ClearLayout()
        {
            Children.Clear();
            Editors.Clear();
        }

        /// <inheritdoc />
        public override Control Control => ContainerControl;

        /// <summary>
        /// Implicit cast using LayoutElementsContrainer.Control
        /// </summary>
        /// <param name="layoutElement"></param>
        public static implicit operator Control(LayoutElementsContainer layoutElement) => layoutElement?.Control;

        /// <summary>
        /// Implicit cast using LayoutElementsContrainer.ContainerControl
        /// </summary>
        /// <param name="layoutElement"></param>
        public static implicit operator ContainerControl(LayoutElementsContainer layoutElement) => layoutElement?.ContainerControl;

    }
}
