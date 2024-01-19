
using System;
using FlaxEditor.Content;
using FlaxEngine.GUI;

namespace FlaxEngine.Experimental.UI
{
    //note this is temporary solution

    /// <summary>
    /// host for Native UI
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    public class NativeUIHost : Control
    {
        /// <summary>
        /// The panel component
        /// </summary>
        [EditorOrder(0)]
        public UIBlueprintAsset Asset;

        /// <summary>
        /// The panel component
        /// </summary>
        //[HideInEditor]
        public UIPanelComponent PanelComponent;
        UIActionEvent LastAction = new UIActionEvent();
        UIActionEvent Action = new UIActionEvent();
        /// <inheritdoc />
        public override void Draw()
        {
            PanelComponent?.Render();
            base.Draw();
        }
        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            if (PanelComponent != null)
            {
                PanelComponent.Layout();
            }
            base.PerformLayout(force);
        }
        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            if (PanelComponent != null)
            {
                PanelComponent.Transform = new UIComponentTransform()
                {
                    Rect = Bounds,
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
                PanelComponent?.OnActionInputChaneged(Action);
            }
            else if (Asset != null)
            {
                if (Asset.IsLoaded)
                {
                    if (Asset.Component != null)
                    {
                        PanelComponent = (UIPanelComponent)Asset.Component;
                    }
                    else
                    {
                        Asset.Reload();
                        Asset.WaitForLoaded();
                    }
                }
            }
            base.Update(deltaTime);
            
        }
        /// <inheritdoc/>
        public override bool OnKeyDown(KeyboardKeys key)
        {
            return base.OnKeyDown(key);
        }
        /// <inheritdoc/>
        public override void OnKeyUp(KeyboardKeys key)
        {
            base.OnKeyUp(key);
        }
    }
}
