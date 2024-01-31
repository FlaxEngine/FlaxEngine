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

namespace FlaxEditor.Experimental.UI
{
    internal class UITransformationToolBlueprint: UIBlueprint 
    {
        internal enum Selection
        {
            Center = 0,
            Top = 1,
            Bottom = 2,
            Left = 4,
            Right = 8
        }
        internal Selection SelectedHandle = Selection.Center;
        internal Vector2 PointerLocation;
        private UIComponent Selected = null;
        private Vector2 Offset;
        private UIButton LeftHandle;
        private UIButton RightHandle;
        private UIButton TopHandle;
        private UIButton TopRightHandle;
        private UIButton TopLeftHandle;
        private UIButton BottomHandle;
        private UIButton BottomLeftHandle;
        private UIButton BottomRightHandle;
        private InputEventHook CenterHandle;

        private UIComponent SelectedComponent;

        internal UIComponent EditedComponent;
        internal CustomEditorPresenter Presenter;
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
            CenterHandle = GetVariable<InputEventHook>("CenterHandle");

            LeftHandle.StateChanged += HandlesStateChanged;
            RightHandle.StateChanged += HandlesStateChanged;
            TopHandle.StateChanged += HandlesStateChanged;
            TopRightHandle.StateChanged += HandlesStateChanged;
            TopLeftHandle.StateChanged += HandlesStateChanged;
            BottomHandle.StateChanged += HandlesStateChanged;
            BottomLeftHandle.StateChanged += HandlesStateChanged;
            BottomRightHandle.StateChanged += HandlesStateChanged;

            CenterHandle.PointerInput = CenterHandlePointerInputHook;
        }
        UIButton.State State;
        private UIEventResponse CenterHandlePointerInputHook(UIPointerEvent InEvent)
        {
            if (InEvent.Locations.Length == 0)
                return UIEventResponse.None;
            bool IsInside = false;
            foreach (var Location in InEvent.Locations)
            {
                if (CenterHandle.Contains(Location))
                {
                    IsInside = true;
                    PointerLocation = Location;
                }
            }
            if (!IsInside)
            {
                PointerLocation = InEvent.Locations[0];
                State = UIButton.State.None;
            }
            else
            {
                var newState = (UIButton.State)InEvent.State;
                if (State != newState)
                {
                    HandlesStateChanged(CenterHandle, newState);
                    return UIEventResponse.Focus;
                }
            }
            return UIEventResponse.None;
        }
        private void HandlesStateChanged(UIComponent button, UIButton.State state)
        {
            if (state == UIButton.State.Press)
            {
                switch (button.Label)
                {
                    case "CenterHandle": Selected = CenterHandle; SelectedHandle = Selection.Center; break;
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
                if (Selected)
                {
                    SelectElement(EditedComponent, PointerLocation);
                }
            }
            else
            {
                Debug.Log(state.ToString());
                Selected = null;
            }
        }

        internal void SelectForDrag(UIComponent Element)
        {
            Offset = PointerLocation - Element.Translation;
            SelectedComponent = Element;

            //coppy the simple transform
            Component.Rect = Element.Rect;
        }
        internal void UpdateDragedObject() 
        {
            if (SelectedComponent == null)
                return;
            if (SelectedHandle == Selection.Center)
            {
                SelectedComponent.Translation = PointerLocation - Offset;
                Component.Rect = SelectedComponent.Rect;
            }
            else
            {
                //resize the component

            }
        }
        internal void EndDrag()
        {
            SelectedComponent = null;
            SelectedHandle = Selection.Center;
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
            TransformationTool.Presenter.Panel.Parent = this;
            

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save");
            _addComponentButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Folder32, AddComponent).LinkTooltip("Add Component");
            _toolstrip.AddSeparator();
            AddChild<Label>();
            NativeUIHost host = AddChild<NativeUIHost>();
            host.AnchorPreset = FlaxEngine.GUI.AnchorPresets.StretchAll;
            host.Blueprint = EditedBluprint;
            host.Height -= _toolstrip.Size.Y;
            host.Location = new Float2(Location.X, _toolstrip.Size.Y);

            UITransformationTool = AddChild<NativeUIHost>();
            UITransformationTool.AnchorPreset = FlaxEngine.GUI.AnchorPresets.StretchAll;
            UITransformationTool.Blueprint = TransformationTool;
            UITransformationTool.Height -= _toolstrip.Size.Y;
            UITransformationTool.Location = new Float2(Location.X, _toolstrip.Size.Y);

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
