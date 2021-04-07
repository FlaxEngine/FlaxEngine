using System;
using System.Collections.Generic;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Dynamic GameButton, itself is useless, but paired with something like Image does many stuff
    /// </summary>
    public class GameButton : Selectable
    {
        /// <summary>
        /// The types of Button Actions
        /// </summary>
        public enum ButtonVerke
        {
            /// <summary>
            /// Swaps brushes
            /// </summary>
            BrushSwap,
            /// <summary>
            /// Tints Colorables or Panels
            /// </summary>
            ColorTint,
        }

        /// <summary>
        /// The type to use
        /// </summary>
        [EditorOrder(10), Tooltip("The type to use.")]
        public ButtonVerke buttonType;
        bool isColorTint { get => buttonType == ButtonVerke.ColorTint; }
        bool isBrushSwap { get => buttonType == ButtonVerke.BrushSwap; }
        
        /// <summary>
        /// The Normal Color
        /// </summary>
        [VisibleIf("isColorTint"), EditorOrder(20)]
        public Color NormalColor;
        /// <summary>
        /// The Color when Hovered
        /// </summary>
        [VisibleIf("isColorTint"), EditorOrder(30)]
        public Color HoverColor;
        /// <summary>
        /// The Pressed Color
        /// </summary>
        [VisibleIf("isColorTint"), EditorOrder(40)]
        public Color PressedColor;
        /// <summary>
        /// The Disbaled Color
        /// </summary>
        [VisibleIf("isColorTint"), EditorOrder(50)]
        public Color DisabledColor;

        /// <summary>
        /// The Disbaled Color
        /// </summary>
        [VisibleIf("isBrushSwap"), EditorOrder(20)]
        public IBrush NormalBrush;
        /// <summary>
        /// The Disbaled Brush
        /// </summary>
        [VisibleIf("isBrushSwap"), EditorOrder(30)]
        public IBrush HoverBrush;
        /// <summary>
        /// The Disbaled Brush
        /// </summary>
        [VisibleIf("isBrushSwap"), EditorOrder(40)]
        public IBrush PressedBrush;
        /// <summary>
        /// The Disbaled Brush
        /// </summary>
        [VisibleIf("isBrushSwap"), EditorOrder(50)]
        public IBrush DisabledBrush;

        /// <summary>
        /// The Target Control, stored for future use
        /// </summary>
        [Serialize]
        protected UIControl targetControl;
        Image targetControlAsImage => targetControl?.Control as Image;
        Colorable targetControlAsColorable => targetControl?.Control as Colorable;

        /// <summary>
        /// The TargetControl to modify, supported Image, Colorable(Image, Label)
        /// </summary>
        [NoSerialize]
        [EditorOrder(60), Tooltip("The TargetControl to modify, supported Image, Colorable(Image, Label).")]
        public UIControl TargetControl 
        {
            get => targetControl;
            set { 
                if (value != null && !(value.Control is Image) && !(value.Control is Colorable))
                {
                    Debug.LogError("Thats not an Image nor Colorable"); 
                    return; 
                }
                targetControl = value; 
            }
        }

        /// <summary>
        /// What Happens after the Click
        /// </summary>
        [EditorOrder(70), Tooltip("What Happens after the Click.")]
        public GameEvent OnClick = new GameEvent();

        /// <inheritdoc/>
        public override void Draw()
        {
            if(TargetControl != null)
            {
                if (isColorTint)
                {
                    SetColors(!Enabled ? DisabledColor : (IsPressed ? PressedColor : (IsSelected || IsMouseOver ? HoverColor : NormalColor)));
                }
                if (isBrushSwap)
                {
                    if(targetControlAsImage != null)
                        targetControlAsImage.Brush = !Enabled ? DisabledBrush : (IsPressed ? PressedBrush : (IsSelected || IsMouseOver ? HoverBrush : NormalBrush));
                }
            }            
            base.Draw();
        }

        /// <summary>
        /// Shortcut for setting all the various Colors
        /// </summary>
        /// <param name="color"></param>
        private void SetColors(Color color)
        {
            if (targetControlAsColorable != null)
            {
                targetControlAsColorable.Color = color;
            }
        }

        /// <inheritdoc/>
        public override void OnDeSelect()
        {
            base.OnDeSelect();
            IsPressed = false;
        }

        /// <inheritdoc/>
        public override void OnSubmit()
        {
            base.OnSubmit();
            IsPressed = true;
            OnClick?.Invoke();
        }

        /// <summary>
        /// Whether or not we are pressed
        /// </summary>
        [HideInEditor]
        public bool IsPressed;

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                IsPressed = false;
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                UIInputSystem.GetInputSystemForRctrl(Root).NavigateTo(this);
                OnSubmit();
                return true;
            }
            return base.OnMouseDown(location, button);
        }
    }
}
