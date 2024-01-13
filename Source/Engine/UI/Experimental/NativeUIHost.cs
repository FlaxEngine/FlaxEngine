

using FlaxEngine.GUI;
namespace FlaxEngine.Experimental.UI
{
    //note this is temporary solution

    /// <summary>
    /// host for Native UI
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Panel" />
    public class NativeUIHost : Panel
    {
        public UIPanelComponent PanelComponent = new UIPanelComponent();
        UIActionEvent LastAction = new UIActionEvent();
        UIActionEvent Action = new UIActionEvent();
        /// <inheritdoc />
        public override void Draw()
        {
            PanelComponent.Render();
            base.Draw();
        }
        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            if( PanelComponent == null)
                PanelComponent = new UIPanelComponent();
            PanelComponent.Layout();
            base.PerformLayout(force);
        }
        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            PanelComponent.Transform = new UIComponentTransform()
            {
                Translation = Bounds.Location,
                Size = Bounds.Size,
                Shear = Shear,
                Angle = Rotation,
                Pivot = Pivot,
            };
            Action = new UIActionEvent()
            {
                Button = GamepadButton.None,
                State = InputActionState.None,
                Chars = Input.InputText,
                Key = KeyboardKeys.None,
                Value = 0,
            };
            PanelComponent.OnActionInputChaneged(Action);
            base.Update(deltaTime);
            
        }
        public override bool OnKeyDown(KeyboardKeys key)
        {
            return base.OnKeyDown(key);
        }
        public override void OnKeyUp(KeyboardKeys key)
        {
            base.OnKeyUp(key);
        }
    }
}
