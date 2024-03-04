using FlaxEditor.Windows.Assets;
using FlaxEditor.GUI;
using FlaxEngine.Experimental.UI;
using FlaxEngine;
using System.Linq;
using System;
using FlaxEngine.Json;
using System.Text;
using FlaxEngine.Experimental.UI.Editor;
using FlaxEngine.GUI;
using FlaxEditor.Windows;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;

namespace FlaxEditor.Experimental.UI
{
    internal class UITransformationToolBlueprint : UIBlueprint 
    {
        private UIButton LeftHandle;
        private UIButton RightHandle;
        private UIButton TopHandle;
        private UIButton TopRightHandle;
        private UIButton TopLeftHandle;
        private UIButton BottomHandle;
        private UIButton BottomLeftHandle;
        private UIButton BottomRightHandle;
        private UIInputEventHook CenterHandle;
        internal enum Selection
        {
            None = 0,
            Center = 1,
            Top = 2,
            Bottom = 4,
            Left = 8,
            Right = 16
        }
        internal Selection SelectedHandle = Selection.Center;
        internal Vector2 PointerLocation;
        private UIComponent Selected = null;
        private Vector2 Offset;
        internal UIComponent SelectedComponent;
        internal UIComponent EditedComponent;
        internal CustomEditorPresenter Presenter;
        internal CustomEditorPresenter SlotPresenter;
        protected override void OnInitialized()
        {
            LeftHandle = GetVariable<UIButton>("LeftHandle");
            RightHandle = GetVariable<UIButton>("RightHandle");
            TopHandle = GetVariable<UIButton>("TopHandle");
            TopRightHandle = GetVariable<UIButton>("TopRightHandle");
            TopLeftHandle = GetVariable<UIButton>("TopLeftHandle");
            BottomHandle = GetVariable<UIButton>("BottomHandle");
            BottomLeftHandle = GetVariable<UIButton>("BottomLeftHandle");
            BottomRightHandle = GetVariable<UIButton>("BottomRightHandle");
            CenterHandle = GetVariable<UIInputEventHook>("CenterHandle");

            LeftHandle.StateChanged += HandlesStateChanged;
            RightHandle.StateChanged += HandlesStateChanged;
            TopHandle.StateChanged += HandlesStateChanged;
            TopRightHandle.StateChanged += HandlesStateChanged;
            TopLeftHandle.StateChanged += HandlesStateChanged;
            BottomHandle.StateChanged += HandlesStateChanged;
            BottomLeftHandle.StateChanged += HandlesStateChanged;
            BottomRightHandle.StateChanged += HandlesStateChanged;

            CenterHandle.PointerInput = CenterHandlePointerInputHook;


            Component.Visibility |= UIComponentVisibility.Hiden;
        }
        UIButton.State State;
        private UIEventResponse CenterHandlePointerInputHook(UIPointerEvent InEvent)
        {
            if (InEvent.Locations.Length == 0)
                return UIEventResponse.None;
            PointerLocation = InEvent.Locations[0];
            var newState = (UIButton.State)InEvent.State;
            if (newState == UIButton.State.Pressing)
            {
                Debug.Log("UpdateDragedObject !!");
                UpdateDragedObject();
                return UIEventResponse.None;
            }

            bool IsInside = false;
            foreach (var Location in InEvent.Locations)
            {
                if (CenterHandle.Contains(Location))
                {
                    IsInside = true;
                    PointerLocation = Location;
                }
            }
            if (State != newState)
            {
                HandlesStateChanged(CenterHandle, newState);
                State = newState;
                if (IsInside)
                {
                    return UIEventResponse.Focus;
                }
            }
            
            return UIEventResponse.None;
        }
        private void HandlesStateChanged(UIComponent button, UIButton.State state)
        {
            if (state == UIButton.State.Press && Selected == null)
            {
                if (SelectedHandle == Selection.None)
                {
                    switch (button.Label)
                    {
                        case "CenterHandle":
                            Selected = CenterHandle;
                            SelectedHandle = Selection.Center;
                            SelectElement(EditedComponent, PointerLocation);
                            break;
                        case "LeftHandle": Selected = LeftHandle; SelectedHandle = Selection.Left; break;
                        case "RightHandle": Selected = RightHandle; SelectedHandle = Selection.Right; break;
                        case "TopHandle": Selected = TopHandle; SelectedHandle = Selection.Top; break;
                        case "TopRightHandle": Selected = TopRightHandle; SelectedHandle = Selection.Top | Selection.Right; break;
                        case "TopLeftHandle": Selected = TopLeftHandle; SelectedHandle = Selection.Top | Selection.Left; break;
                        case "BottomHandle": Selected = BottomHandle; SelectedHandle = Selection.Bottom; break;
                        case "BottomLeftHandle": Selected = BottomLeftHandle; SelectedHandle = Selection.Bottom | Selection.Left; break;
                        case "BottomRightHandle": Selected = BottomRightHandle; SelectedHandle = Selection.Bottom | Selection.Right; break;
                        default:
                            Debug.Log("Emm the UITransformationTool cant select handle");
                            break;
                    }
                    if (SelectedComponent != null)
                    {
                        if (SelectedHandle == Selection.Center)
                        {
                            Offset = PointerLocation - SelectedComponent.Translation;
                            return;
                        }
                        Offset = Vector2.Zero;

                        if (SelectedHandle.HasFlag(Selection.Top))
                        {
                            Offset.Y = SelectedComponent.Top - PointerLocation.Y;
                        }
                        if (SelectedHandle.HasFlag(Selection.Bottom))
                        {
                            Offset.Y = PointerLocation.Y - SelectedComponent.Bottom;
                        }
                        if (SelectedHandle.HasFlag(Selection.Left))
                        {
                            Offset.X = SelectedComponent.Left - PointerLocation.X;
                        }
                        if (SelectedHandle.HasFlag(Selection.Right))
                        {
                            Offset.X = PointerLocation.X - SelectedComponent.Right;
                        }
                    }
                }
            }
            else
            {
                if (state == UIButton.State.Release)
                {
                    EndDrag();
                }
            }
        }

        internal void SelectForDrag(UIComponent Element)
        {
            SelectedComponent = Element;

            if (SelectedComponent == null)
            {
                Component.Visibility |= UIComponentVisibility.Hiden;
            }
            else 
            {
                Component.Visibility &= ~UIComponentVisibility.Hiden;
            }
            //coppy the simple transform
            Component.Rect = Element.Rect;
        }
        internal void UpdateDragedObject() 
        {
            if (SelectedComponent == null)
                return;

            var Point = PointerLocation - Offset;
            if (SelectedHandle == Selection.Center)
            {
                SelectedComponent.Translation = Point;
            }
            else
            {
                if (SelectedHandle.HasFlag(Selection.Top))
                {
                    SelectedComponent.Top = Point.Y;
                }
                if (SelectedHandle.HasFlag(Selection.Bottom))
                {
                    SelectedComponent.Bottom = Point.Y;
                }
                if (SelectedHandle.HasFlag(Selection.Left))
                {
                    SelectedComponent.Left = Point.X;
                }
                if (SelectedHandle.HasFlag(Selection.Right))
                {
                    SelectedComponent.Right = Point.X;
                }
            }
            Component.Rect = SelectedComponent.Rect;
        }
        internal void EndDrag()
        {
            Selected = null;
            SelectedHandle = Selection.None;
        }

        private void SetSelection(UIComponent NewSelection)
        {
            if (SelectedComponent != null) SelectedComponent.Deselect();
            SelectedComponent = NewSelection;
            if (SelectedComponent != null)
            {
                SelectedComponent.Select();
                SelectForDrag(SelectedComponent);
                Presenter.Select(SelectedComponent);
                SlotPresenter.Select(SelectedComponent.GetSlot());
            }
        }
        private bool SelectElement(UIComponent comp, Float2 location)
        {
            bool GotSelection = false;
            if (comp == null)
                return false;

            if (comp.Contains(location) && SelectedComponent != comp && !comp.IsLockedInDesigner())
            {
                SetSelection(comp);
                GotSelection = true;
            }
            if (comp is UIPanelComponent panelComponent)
            {
                var children = panelComponent.GetAllChildren();
                for (int i = 0; i < children.Length; i++)
                {
                    if (children[i].Contains(location) && SelectedComponent != children[i] && !children[i].IsLockedInDesigner())
                    {
                        if (SelectElement(children[i], location))
                        {
                            GotSelection = true;
                            break;
                        }
                    }
                }
            }
            if (!GotSelection)
            {
                SetSelection(null);
            }
            return GotSelection;
        }

    }

    internal class UIDesignerEditor : AssetEditorWindow
    {
        (Type,UIDesignerAttribute)[] Attributes;
        

        private UIBlueprint EditedBluprint;
        private NativeUIHost UITransformationTool;
        ToolStripButton _saveButton;
        ToolStripButton _addComponentButton;

        UITransformationToolBlueprint TransformationTool;
        public UIDesignerEditor(Editor editor, UIBlueprintAssetItem item) : base(editor, item)
        {
            TransformationTool = UISystem.LoadEditorBlueprintAsset("Editor\\UI\\UITransformationTool.json") as UITransformationToolBlueprint;
            EditedBluprint = UISystem.CreateFromBlueprintAsset(FlaxEngine.Content.Load<UIBlueprintAsset>(item.Path));
            TransformationTool.Presenter = new CustomEditorPresenter(editor.Undo, null, null);
            TransformationTool.SlotPresenter = new CustomEditorPresenter(editor.Undo, null, null);
            var p = new SplitPanel();
            var p2 = new SplitPanel(Orientation.Vertical);

            AddChild(p);
            p.AnchorPreset = FlaxEngine.GUI.AnchorPresets.StretchAll;
            p.Height -= _toolstrip.Size.Y;
            p.Location = new Float2(Location.X, _toolstrip.Size.Y);
            TransformationTool.SlotPresenter.Panel.SizeChanged += (Control _) => { TransformationTool.Presenter.Panel.Proxy_Offset_Top = TransformationTool.SlotPresenter.Panel.Bottom; };
            p.Panel2.AddChild(p2);

            var UIComponentPaletePanel = p2.Panel1.AddChild<TilesPanel>();
            UIComponentPaletePanel.AnchorPreset = AnchorPresets.StretchAll;

            TransformationTool.Presenter.Panel.Parent = p2.Panel2;
            TransformationTool.SlotPresenter.Panel.Parent = p2.Panel2;
            p2.AnchorPreset = FlaxEngine.GUI.AnchorPresets.StretchAll;

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save");
            _addComponentButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Folder32, AddComponent).LinkTooltip("Add Component");
            _toolstrip.AddSeparator();
            NativeUIHost host = p.Panel1.AddChild<NativeUIHost>();
            host.AnchorPreset = FlaxEngine.GUI.AnchorPresets.StretchAll;
            host.Blueprint = EditedBluprint;
            //host.Height -= _toolstrip.Size.Y;
            //host.Location = new Float2(Location.X, _toolstrip.Size.Y);

            UITransformationTool = p.Panel1.AddChild<NativeUIHost>();
            UITransformationTool.AnchorPreset = FlaxEngine.GUI.AnchorPresets.StretchAll;
            UITransformationTool.Blueprint = TransformationTool;
            //UITransformationTool.Height -= _toolstrip.Size.Y;
            //UITransformationTool.Location = new Float2(Location.X, _toolstrip.Size.Y);

            TransformationTool.SetDesinerFlags(UIComponentDesignFlags.None);

            EditedBluprint.SetDesinerFlags(UIComponentDesignFlags.Designing);
            TransformationTool.EditedComponent = EditedBluprint.Component;

            //System.Linq madnes :D
            Attributes = typeof(UIComponent).Assembly.GetTypes().
                Where(t => t.IsSubclassOf(typeof(UIComponent)) && !t.IsAbstract).
                Select(t =>
                {
                    var o = t.GetCustomAttributes(typeof(UIDesignerAttribute), false);
                    if (o.Length != 0)
                        return (t,(UIDesignerAttribute)o[0]);
                    return (t, null);
                }).Where(t => t.Item2 != null).ToArray();

            UIComponentPaletePanel.TileSize = Float2.One * 75;
            for (int i = 0; i < Attributes.Length; i++)
            {
                var button = UIComponentPaletePanel.AddChild<Button>();
                button.Tag = i;
                button.ButtonClicked += AddComponentToSelected;
                var lable = button.AddChild<Label>();
                lable.AnchorPreset = AnchorPresets.HorizontalStretchBottom;
                lable.AutoHeight = true;
                lable.AutoFitText = true;
                lable.Text = Attributes[i].Item2.DisplayLabel;
            }
            InputActions.Add(options => options.Delete, DeleteComponentFromSelected);
        }

        private void DeleteComponentFromSelected()
        {
            if (TransformationTool.SelectedComponent)
            {
                TransformationTool.SelectedComponent.RemoveFromParent();
            }
        }

        private void AddComponentToSelected(Button obj)
        {
            var ID = (int)obj.Tag;
            Debug.Log(ID);
            if (TransformationTool.SelectedComponent)
            {
                if(TransformationTool.SelectedComponent is UIPanelComponent panel)
                {
                    if (panel.CanAddMoreChildren())
                    {
                        var comp = (UIComponent)Activator.CreateInstance(Attributes[ID].Item1);
                        panel.AddChild(comp);
                        comp.Translation = panel.Rect.Center;
                        UISystem.SetDesinerFlags(comp, UIComponentDesignFlags.Designing);
                        comp.CreatedByUIBlueprint = true;
                    }
                }
            }
        }

        public void AddComponent()
        {
            if (EditedBluprint.Component == null)
            {
                var rootcomp = new UIPanelComponent();
                UISystem.SetDesinerFlags(rootcomp, UIComponentDesignFlags.Designing);
                EditedBluprint.Component = rootcomp;
                rootcomp.CreatedByUIBlueprint = true;
                return;
            }
            var comp = new UIComponent();
            UISystem.SetDesinerFlags(comp, UIComponentDesignFlags.Designing);
            (EditedBluprint.Component as UIPanelComponent)?.AddChild(comp);
            comp.CreatedByUIBlueprint = true;
        }
        public override void Save()
        {
            UISystem.SaveBlueprint(EditedBluprint);
            ClearEditedFlag();
        }
    }
}
