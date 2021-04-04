using System;
using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Base class for all possible UIInputModules
    /// </summary>
    public abstract class UIInputModule : Script
    {
        /// <summary>
        /// The delay to start before holding the button start repeating
        /// </summary>
        [EditorOrder(10), Limit(1, 10000, 0.1f), Tooltip("The delay to start before holding the button start repeating.")]
        public float MoveRepeatDelay = 0.5f;
        /// <summary>
        /// The repeat rate after the initial delay
        /// </summary>
        [EditorOrder(20), Limit(1, 10000, 0.1f), Tooltip("The repeat rate after the initial delay.")]
        public float MoveRepeatRate = 0.2f;

        /// <summary>
        /// The internal UIInputSystem
        /// </summary>
        [Serialize]
        protected UIInputSystem cachedInputSystem;
        /// <summary>
        /// Actual UIInputSystem available to use internally
        /// </summary>
        public UIInputSystem InputSystem
        {
            get
            {
                if(cachedInputSystem == null)
                {
                    cachedInputSystem = Actor.GetScript<UIInputSystem>();
                }
                return cachedInputSystem;
            }
        }
    }
}
