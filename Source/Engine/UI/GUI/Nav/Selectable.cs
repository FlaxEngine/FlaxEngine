using System;
using System.Collections.Generic;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The Selctable, primary Iselectable implementation, can be just inherited to make functionality like Buttons, Sliders etc...
    /// </summary>
    public class Selectable : ContainerControl, ISelectable
    {
        /// <summary>
        /// Whether or not we have registerd
        /// </summary>
        bool inited = false;

        /// <inheritdoc/>
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);
            if (!inited)
            {
                inited = true;
                //Debug.Log("Adding " + this + " for " + Root + " btw my parent is " + Parent);
                UIInputSystem.AddSelectable( this);
            }
        }
        /// <inheritdoc/>
        public override void OnDestroy()
        {
            UIInputSystem.RemoveSelectable(this);
            base.OnDestroy();
        }

        /// <summary>
        /// Just like navdir but Flags
        /// </summary>
        [Flags]
        public enum PossibleDirs
        {
            /// <summary>
            /// Left
            /// </summary>
            Left = 1,
            /// <summary>
            /// Right
            /// </summary>
            Right = 2,
            /// <summary>
            /// Up
            /// </summary>
            Up = 4,
            /// <summary>
            /// Down
            /// </summary>
            Down = 8
        }

        /// <summary>
        /// Whether or not Up nav is Allowed
        /// </summary>
        public bool upAllowed => (GetAllowedDirs() & PossibleDirs.Up) != 0;
        /// <summary>
        /// Whether or not right nav is Allowed
        /// </summary>
        public bool rightAllowed => (GetAllowedDirs() & PossibleDirs.Right) != 0;
        /// <summary>
        /// Whether or not left nav is Allowed
        /// </summary>
        public bool leftAllowed => (GetAllowedDirs() & PossibleDirs.Left) != 0;
        /// <summary>
        /// Whether or not Up downv is Allowed
        /// </summary>
        public bool downAllowed => (GetAllowedDirs() & PossibleDirs.Down) != 0;

        /// <summary>
        /// Control to navigate to when going Up
        /// </summary>
        [Serialize]
        protected UIControl onNavigateUp;
        /// <summary>
        /// Control to navigate to when going Up
        /// </summary>
        [VisibleIf("upAllowed")]
        [NoSerialize, EditorOrder(2010), Tooltip("Control to navigate to when going Up"), EditorDisplay("OnNavigate"), ExpandGroups]
        public UIControl OnNavigateUp{
            get => onNavigateUp;
            set{
                if (value != null && !(value.Control is ISelectable)) {
                    Debug.LogError("That UIControl does not implement ISelectable");
                    return;
                }
                onNavigateUp = value;
            }
        }

        /// <summary>
        /// Control to navigate to when going Down
        /// </summary>
        [Serialize]
        protected UIControl onNavigateDown;
        /// <summary>
        /// Control to navigate to when going Down
        /// </summary>
        [VisibleIf("downAllowed")]
        [NoSerialize, EditorOrder(2020), Tooltip("Control to navigate to when going Down"), EditorDisplay("OnNavigate"), ExpandGroups]
        public UIControl OnNavigateDown
        {
            get => onNavigateDown;
            set {
                if (value != null && !(value.Control is ISelectable)) { 
                    Debug.LogError("That UIControl does not implement ISelectable"); 
                    return;
                }
                onNavigateDown = value;
            }
        }

        /// <summary>
        /// Control to navigate to when going Left
        /// </summary>
        [Serialize]
        protected UIControl onNavigateLeft;
        /// <summary>
        /// Control to navigate to when going Left
        /// </summary>
        [VisibleIf("leftAllowed")]
        [NoSerialize, EditorOrder(2030), Tooltip("Control to navigate to when going Left"), EditorDisplay("OnNavigate"), ExpandGroups]
        public UIControl OnNavigateLeft
        {
            get => onNavigateLeft;
            set {
                if (value != null && !(value.Control is ISelectable)) {
                    Debug.LogError("That UIControl does not implement ISelectable"); 
                    return; 
                } 
                onNavigateLeft = value;
            }
        }

        /// <summary>
        /// Control to navigate to when going Right
        /// </summary>
        [Serialize]
        protected UIControl onNavigateRight;
        /// <summary>
        /// Control to navigate to when going Right
        /// </summary>
        [VisibleIf("rightAllowed")]
        [NoSerialize, EditorOrder(2040), Tooltip("Control to navigate to when going Right"), EditorDisplay("OnNavigate"), ExpandGroups]
        public UIControl OnNavigateRight
        {
            get => onNavigateRight;
            set { 
                if (value != null && !(value.Control is ISelectable)) {
                    Debug.LogError("That UIControl does not implement ISelectable"); 
                    return; 
                }
                onNavigateRight = value;
            }
        }

        /// <summary>
        /// All the navigation dirs, override this for like sliders etc...
        /// </summary>
        /// <returns>All the possible dirs where its allowed to navigate</returns>
        public virtual PossibleDirs GetAllowedDirs()
        {
            return PossibleDirs.Down | PossibleDirs.Left | PossibleDirs.Right | PossibleDirs.Up;
        }

        /// <summary>
        /// Whether or not we are currently Selected by the Navigation System
        /// </summary>
        [HideInEditor]
        public bool IsSelected;

        /// <inheritdoc/>
        public virtual void OnSelect() {
            IsSelected = true;
        }

        /// <inheritdoc/>
        public virtual void OnDeSelect(){
            IsSelected = false;
        }

        /// <inheritdoc/>
        public virtual bool OnNavigate(NavDir navDir, UIInputSystem system)
        {
            switch (navDir)
            {
                case NavDir.Up:
                    {
                        if (upAllowed && OnNavigateUp)
                        {
                            system.NavigateTo(onNavigateUp.Control as ISelectable);
                            return true;
                        }
                        return false;
                    }
                case NavDir.Down:
                    {
                        if (downAllowed && OnNavigateDown)
                        {
                            system.NavigateTo(OnNavigateDown.Control as ISelectable);
                            return true;
                        }
                        return false;
                    }
                case NavDir.Left:
                    {
                        if (leftAllowed && OnNavigateLeft)
                        {
                            system.NavigateTo(OnNavigateLeft.Control as ISelectable);
                            return true;
                        }
                        return false;
                    }
                case NavDir.Right:
                    {
                        if (rightAllowed && OnNavigateRight)
                        {
                            system.NavigateTo(OnNavigateRight.Control as ISelectable);
                            return true;
                        }
                        return false;
                    }
                default:
                    {
                        return false;
                    }
            }
        }

        /// <inheritdoc/>
        public virtual void OnSubmit(){}

        /// <inheritdoc/>
        public bool EvaluateInAutoNav()
        {
            return EnabledInHierarchy && VisibleInHierarchy && Root != null && Parent != null;
        }

        /// <inheritdoc/>
        public RootControl GetRootControl()
        {
            return Root;
        }

        /// <inheritdoc/>
        public Vector2 GetPosition()
        {
            return PointToScreen(Vector2.Zero);
        }
    }
}
