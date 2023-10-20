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


        #region Group

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

        #endregion

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
            var property = AddPropertyItem(name, tooltip);
            return property.Checkbox();
        }

        /// <summary>
        /// Adds new check box element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The created element.</returns>
        public virtual CheckBoxElement Checkbox(string name)
        {
            var element = Checkbox(name, null);
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
            var property = AddPropertyItem(name, tooltip);
            return property.Label(text);
        }

        /// <summary>
        /// Adds new label element.
        /// </summary>
        /// <param name="text">The label text.</param>
        /// <returns>The created element.</returns>
        public virtual LabelElement Label(string text)
        {
            var element = Label(text, TextAlignment.Near);      
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
            var element = Label(text, text, null);
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
            var property = AddPropertyItem(name, tooltip);
            return property.ClickableLabel(text);
        }

        /// <summary>
        /// Adds new label element.
        /// </summary>
        /// <param name="text">The label text.</param>
        /// <returns>The created element.</returns>
        public virtual CustomElement<ClickableLabel> ClickableLabel(string text)
        {
            var element = ClickableLabel(text, horizontalAlignment: TextAlignment.Near);        
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
            var element = ClickableLabel(name, text, null);
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
            var property = AddPropertyItem(name, tooltip);
            return property.Slider();
        }

        /// <summary>
        /// Adds new slider element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>

        /// <returns>The created element.</returns>
        public virtual SliderElement Slider(string name)
        {
            var element = Slider(name,null);
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
            var property = AddPropertyItem(name, tooltip);
            return property.SignedIntegerValue();
        }

        /// <summary>
        /// Adds new signed integer (up to long range) value element.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The created element.</returns>
        public virtual SignedIntegerValueElement SignedIntegerValue(string name)
        {
            var element = SignedIntegerValue(name, null);
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
            var property = AddPropertyItem(name, tooltip);
            return property.UnsignedIntegerValue();
        }

        /// <summary>
        /// Adds new unsigned signed integer (up to ulong range) value element.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The created element.</returns>
        public virtual UnsignedIntegerValueElement UnsignedIntegerValue(string name)
        {
            var property = AddPropertyItem(name, null);
            return property.UnsignedIntegerValue();
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
            var property = AddPropertyItem(name, tooltip);
            return property.IntegerValue();
        }

        /// <summary>
        /// Adds new integer value element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The created element.</returns>
        public virtual IntegerValueElement IntegerValue(string name)
        {
            var element = IntegerValue(name, null);
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
            var property = AddPropertyItem(name, tooltip);
            return property.ComboBox();
        }

        /// <summary>
        /// Adds new combobox element with name label.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The created element.</returns>
        public virtual ComboBoxElement ComboBox(string name)
        {
            var element = ComboBox(name, null);
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
            var element = new EnumElement(type, customBuildEntriesDelegate, formatMode);
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
            var property = AddPropertyItem(name, tooltip);
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
            var element = Enum(type, customBuildEntriesDelegate, formatMode: EnumDisplayAttribute.FormatMode.Default);
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
            var element = Enum(name,type, customBuildEntriesDelegate, tooltip, formatMode:EnumDisplayAttribute.FormatMode.Default);
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
            var element = Enum(name, type, customBuildEntriesDelegate, tooltip:null, formatMode:EnumDisplayAttribute.FormatMode.Default);
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
            var property = AddPropertyItem(name, tooltip);
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
            var property = AddPropertyItem(label, tooltip);
            return property.Object(values, overrideEditor);
        }

        /// <summary>
        /// Adds object(s) editor. Selects proper <see cref="CustomEditor"/> based on overrides.
        /// </summary>
        /// <param name="values">The values.</param>
        /// <returns>The created element.</returns>
        public virtual CustomEditor Object(ValueContainer values)
        {
            var element = Object(values, null);
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
            var element = Object(name, values, overrideEditor, tooltip: null);
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
            var element = Object(label, values, overrideEditor: null, tooltip: null);
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
            var element = Object(label, values, overrideEditor, tooltip: null);
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
            var element = Property(name,values, overrideEditor, tooltip: null);
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
            var element = Property(name, values, overrideEditor: null, tooltip: null);
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
            var element = Property(label, values, overrideEditor, tooltip: null);
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
            var element = Property(label, values, overrideEditor: null, tooltip: null);
            return element;
        }

        #endregion

        protected PropertiesListElement AddPropertyItem()
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

    }
}
