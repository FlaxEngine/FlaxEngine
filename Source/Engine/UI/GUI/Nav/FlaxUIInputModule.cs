using System;
using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The Basic Navigation module with norma Flax Input
    /// </summary>
    public class FlaxUIInputModule : UIInputModule
    {
        /// <summary>
        /// The input axis to use for horizontal
        /// </summary>
        [Tooltip("The input axis to use for horizontal")]
        public string HorizontalInput = "Horizontal";
        /// <summary>
        /// The input axis to use for vertical
        /// </summary>
        [Tooltip("The input axis to use for vertical")]
        public string VerticalInput = "Vertical";
        /// <summary>
        /// The input action to use for submit
        /// </summary>
        [Tooltip("The input action to use for submit")]
        public string SubmitInput = "Submit";
        /// <summary>
        /// The point from which an axis is being threated as pushed enough
        /// </summary>
        [Tooltip("The point from which an axis is being threated as pushed enough")]
        public float InputZone = 0.5f;

        /// <summary>
        /// Holds the input state
        /// </summary>
        struct FlaxInput
        {
            public float horizontal;
            public float vertical;
            public bool submit;
        }
        FlaxInput lastFrameState = new FlaxInput { horizontal = 0, vertical = 0, submit = false };
        FlaxInput actualState;

        /// <summary>
        /// Gets the current Input
        /// </summary>
        /// <returns>Current Input in FlaxInput struct</returns>
        FlaxInput GatherInput()
        {
            FlaxInput flInput = new FlaxInput();
            flInput.horizontal = Input.GetAxis(HorizontalInput);
            flInput.vertical = Input.GetAxis(VerticalInput);
            flInput.submit = Input.GetAction(SubmitInput);
            return flInput;
        }

        float horizontalHeldTime = 0;
        float verticalHeldTime = 0;

        float horizontalRateTime = 0;
        float verticalRateTime = 0;

        /// <inheritdoc/>
        public override void OnUpdate()
        {
            actualState = GatherInput();

            if(actualState.horizontal > InputZone || actualState.horizontal < -InputZone)
            {
                if (horizontalHeldTime == 0)
                {
                    InputSystem.Navigate(actualState.horizontal > 0 ? NavDir.Right : NavDir.Left);
                }

                if(horizontalHeldTime > MoveRepeatDelay)
                {
                    horizontalRateTime += Time.DeltaTime;
                }
                if(horizontalRateTime > MoveRepeatRate)
                {
                    InputSystem.Navigate(actualState.horizontal > 0 ? NavDir.Right : NavDir.Left);
                    horizontalRateTime = 0;
                }
                horizontalHeldTime += Time.DeltaTime;
            }
            else
            {
                horizontalHeldTime = 0;
                horizontalRateTime = 0;
            }

            if (actualState.vertical > InputZone || actualState.vertical < -InputZone)
            {
                if(verticalHeldTime == 0)
                {
                    InputSystem.Navigate(actualState.vertical > 0 ? NavDir.Up : NavDir.Down);
                }

                if (verticalHeldTime > MoveRepeatDelay)
                {
                    verticalRateTime += Time.DeltaTime;
                }
                if (verticalRateTime > MoveRepeatRate)
                {
                    InputSystem.Navigate(actualState.vertical > 0 ? NavDir.Up : NavDir.Down);
                    verticalRateTime = 0;
                }
                verticalHeldTime += Time.DeltaTime;
            }
            else
            {
                verticalHeldTime = 0;
                verticalRateTime = 0;
            }

            if (actualState.submit && !lastFrameState.submit)
            {
                InputSystem.Submit();
            }

            lastFrameState = actualState;
        }
    }
}
