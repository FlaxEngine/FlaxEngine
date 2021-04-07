using System;
using System.Collections.Generic;
using FlaxEngine;
using FlaxEngine.GUI;
using System.Linq;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Possible navigation directiosn
    /// </summary>
    public enum NavDir
    {
        /// <summary>
        /// Left
        /// </summary>
        Left,
        /// <summary>
        /// Right
        /// </summary>
        Right,
        /// <summary>
        /// Up
        /// </summary>
        Up,
        /// <summary>
        /// Down
        /// </summary>
        Down
    }
    /// <summary>
    /// The main system for navifating the UI with like controller, keyboard etc.
    /// </summary>
    public class UIInputSystem : Script
    {
        ISelectable currentlySelected;

        [Serialize]
        UIControl defaultSelectedControl;

        /// <summary>
        /// Control which is selected when the game starts
        /// </summary>
        [NoSerialize]
        [EditorOrder(10), Tooltip("Control which is selected when the game starts.")]
        public UIControl DefaultSelectedControl
        {
            get => defaultSelectedControl;
            set
            {
                if(value != null && !(value.Control is ISelectable))
                {
                    Debug.LogError("Default Selected does not implement ISelectable");
                    return;
                }
                defaultSelectedControl = value;
            }
        }

        /// <summary>
        /// Static array of all selectables by their root, this is for grouping selectables
        /// </summary>
        static Dictionary<RootControl, List<ISelectable>> selectablesByRoot = new Dictionary<RootControl, List<ISelectable>>();
        /// <summary>
        /// These are all Input Systems by their root
        /// </summary>
        static Dictionary<RootControl, UIInputSystem> inputSystemsByRoot = new Dictionary<RootControl, UIInputSystem>();
        /// <summary>
        /// This is the single instance
        /// </summary>
        static UIInputSystem instance;

        /// <summary>
        /// Get the input system or the general one
        /// </summary>
        /// <param name="rctrl">The Root Control to test with</param>
        /// <returns></returns>
        public static UIInputSystem GetInputSystemForRctrl(RootControl rctrl)
        {
            if (inputSystemsByRoot.ContainsKey(rctrl))
                return inputSystemsByRoot[rctrl];
            else
                return instance;
        }

        /// <summary>
        /// Gets List of selectables by their RootControl
        /// </summary>
        /// <param name="rctrl">The Root Control</param>
        /// <returns></returns>
        public static List<ISelectable> GetSelListForRctrl(RootControl rctrl)
        {
            if (!selectablesByRoot.ContainsKey(rctrl))
            {
                selectablesByRoot.Add(rctrl, new List<ISelectable>());
            }
            return selectablesByRoot[rctrl];
        }

        /// <summary>
        /// Adds selectable, should be used if you are implementing the interface yoursef
        /// </summary>
        /// <param name="sel">Yourself</param>
        public static void AddSelectable(ISelectable sel)
        {
            GetSelListForRctrl(sel.GetRootControl()).Add(sel);
        }
        
        /// <summary>
        /// Removes the selecatble(At the end of the game etc..)..cleanup, should be used if you are implementing the interface yourself
        /// </summary>
        /// <param name="selec">Yourself</param>
        public static void RemoveSelectable(ISelectable selec)
        {
            foreach (List<ISelectable> sels in selectablesByRoot.Values)
            {
                if(sels.Contains(selec))
                    sels.Remove(selec);
            }
        }

        /// <inheritdoc/>
        public override void OnAwake()
        {
            base.OnAwake();

            if (instance == null)
                instance = this;

            RootControl rctrl = (Actor as UICanvas).GUI;
            if (rctrl != null)
                inputSystemsByRoot.Add(rctrl, this);
        }

        /// <inheritdoc/>
        public override void OnDestroy()
        {
            base.OnDestroy();
            if (instance == this)
                instance = null;
        }

        /// <inheritdoc/>
        public override void OnStart()
        {
            base.OnStart();
            if(DefaultSelectedControl != null)
                NavigateTo(DefaultSelectedControl.Control as ISelectable);
        }

        /// <summary>
        /// Performs stuff to navigate to the new ISelectable
        /// </summary>
        /// <param name="newSelectable">The selectable to navigate to</param>
        public void NavigateTo(ISelectable newSelectable)
        {
            currentlySelected?.OnDeSelect();
            currentlySelected = newSelectable;
            currentlySelected?.OnSelect();
        }

        /// <summary>
        /// Navigates in that direction, used by InputModules etc.
        /// </summary>
        /// <param name="navDir">The direction</param>
        public void Navigate(NavDir navDir)
        {
            if (currentlySelected == null)
                return;
            if (!currentlySelected.OnNavigate(navDir, this))
            {
                ISelectable newSel = AutoNaviagate(currentlySelected, navDir);
                //Debug.LogWarning("We tried to auto navigate from " + currentlySelected + " in direction of " + navDir + " and found " + newSel);
                if(newSel != null)
                {
                    NavigateTo(newSel);
                }
            }            
        }

        /// <summary>
        /// Creates UI ScreenSpace like direction(Y inverted) from the navdir, used internally for calculating how likely we are autonavigating towards new Control
        /// </summary>
        /// <param name="navDir">The direction to use</param>
        /// <returns>Screen Space like direction </returns>
        public Vector2 GetUIVectorDirFromNavDir(NavDir navDir)
        {
            switch (navDir)
            {
                case NavDir.Down:
                    return new Vector2(0, 1);//Swapped dirs because UI is upside down
                case NavDir.Up:
                    return new Vector2(0, -1);
                case NavDir.Right:
                    return new Vector2(1, 0);
                case NavDir.Left:
                    return new Vector2(-1, 0);
                default:
                    return Vector2.Zero;
            }
        }

        /// <summary>
        /// Performs auto navigation
        /// </summary>
        /// <param name="from">The selectable from which to search</param>
        /// <param name="navDir">The direction</param>
        /// <returns>The new selectable which was found</returns>
        private ISelectable AutoNaviagate(ISelectable from, NavDir navDir)
        {
            Vector2 directionOfInput = GetUIVectorDirFromNavDir(navDir);

            float closetsDistance = float.PositiveInfinity;
            ISelectable closestSelectable = null;
            foreach(ISelectable potentialSelectable in GetSelListForRctrl(from.GetRootControl()))
            {
                if (potentialSelectable != this && potentialSelectable.EvaluateInAutoNav())
                {
                    Vector2 directionOfDifferences = (potentialSelectable.GetPosition() - from.GetPosition());
                    directionOfDifferences.Normalize();
                    float likelinessOfDirectionCohersion = Vector2.Dot(directionOfInput, directionOfDifferences);
                    if (likelinessOfDirectionCohersion > 0f)
                    {
                        float distance = Vector2.Distance(potentialSelectable.GetPosition(), from.GetPosition());
                        if (distance < closetsDistance)
                        {
                            closetsDistance = distance;
                            closestSelectable = potentialSelectable;
                        }
                    }
                }
            }
            return closestSelectable;
        }

        /// <summary>
        /// Performs submit action, useful for Custom UIInputModules
        /// </summary>
        public void Submit()
        {
            currentlySelected?.OnSubmit();
        }
    }
}
