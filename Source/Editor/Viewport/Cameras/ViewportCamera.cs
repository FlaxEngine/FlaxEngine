using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport.Cameras
{
    /// <summary>
    /// Camera class for <see cref="T:FlaxEditor.Viewport.EditorViewport" />'s
    /// </summary>
    [HideInEditor]
    public class ViewportCamera
    {
        private EditorViewport _viewport;
        private Vector3 _translation;
        private Quaternion _orientation;
        private Vector3 _up;
        private Vector3 _right;
        private Vector3 _forward;
        private float _orbitRadius;
        private Vector3 _orbitCenter;
        private Matrix _projectionMatrix;
        private Matrix _viewMatrix;
        private bool _useOrthographicProjection;

        /// <summary>
        /// queue of AnimatedActions 
        /// <br>first AnimatedActions is curent Action can be overiten by <br>Calling</br><br>AnimatedActions[0] = new <see cref="AnimatedAction"/>(...);</br></br>
        /// <br> AnimatedActions.Add(new <see cref="AnimatedAction"/>(...)) will add it in to the queue and will be executend when curent <see cref="AnimatedAction"/> is complited</br>
        /// <br> AnimatedActions.Clear(); cansels all actions<br>  </br></br>
        /// <br>List can have 0 elements</br>
        /// </summary>
        public List<AnimatedAction> AnimatedActions = new List<AnimatedAction>(1);
        /// <summary>
        /// allows to move camera with smoothing
        /// </summary>
        public struct AnimatedAction
        {
            internal readonly float      _orbitRadius;
            internal readonly Vector3    _orbitCenter;
            internal readonly Quaternion _orientation;
            internal float curentSpeed;
            /// <summary>
            /// creates new AnimatedAction
            /// </summary>
            /// <param name="orbitRadius">Target orbitRadius</param>
            /// <param name="orbitCenter">Target orbitCenter</param>
            /// <param name="orientation">Target orientation</param>
            public AnimatedAction(float orbitRadius, Vector3 orbitCenter, Quaternion orientation)
            {
                _orbitRadius = orbitRadius;
                _orbitCenter = orbitCenter;
                _orientation = orientation;
            }
        }

        /// <summary>true when camera moves or rotates </summary>
        private bool _recalculateViewMatrix = true;

        /// <summary>
        /// true when zomming in and out in Orthographic mode 
        /// or
        /// swaping betwine Orthographic view and perspective
        /// </summary>
        private bool _recalculateprojectionMatrix = true;

        /// <summary> Gets or sets the camera near clipping plane.                                                  </summary>
        public float NearPlane = 8f;

        /// <summary> Gets or sets the camera far clipping plane.                                                   </summary>
        public float FarPlane = 20000f;

        /// <summary> Gets or sets the camera field of view (in degrees).                                           </summary>
        public float FieldOfView = 90f;

        /// <summary> Gets or sets if the panning direction is inverted.                                            </summary>
        public bool InvertPanning;

        /// <summary> Gets or sets the camera movement speed.                                                       </summary>
        public float MovementSpeed;

        /// <summary> Gets or sets the mouse sensitivity for camera movement                                        </summary>
        public float MouseSensitivity;

        /// <summary> flag for if camera can move araund using controls                                             </summary>
        public bool IsMovable = false;

        /// <summary> Gets a value indicating whether the viewport camera uses movement speed.                      </summary>
        public bool UseMovementSpeed = true;

        /// <summary> flag for if camera can move roll using controls                                               </summary>
        public bool LockRoll = true;

        /// <summary>
        /// crates new viewport camera
        /// </summary>
        /// <param name="isMovable">flag for if camera can move araund using controls</param>
        public ViewportCamera(bool isMovable = false)
        {
            IsMovable = isMovable;

            _recalculateViewMatrix = true;
            _recalculateprojectionMatrix = true;
        }

        /// <summary> Gets or sets the camera orthographic size scale (if camera uses orthographic mode).           </summary>
        public float OrthographicScale { get; private set; } = 1f;

        /// <summary> Gets or sets the camera orthographic mode (otherwise uses perspective projection).            </summary>
        public bool UseOrthographicProjection
        {
            get
            {
                return _useOrthographicProjection;
            }
            set
            {
                if (_useOrthographicProjection != value)
                {
                    _useOrthographicProjection = value;
                    _recalculateprojectionMatrix = true;
                }
            }
        }

        /// <summary>Gets or sets the orbit center.</summary>
        public Vector3 OrbitCenter
        {
            get
            {
                return _orbitCenter;
            }
            set
            {
                _orbitCenter = value;
                Vector3 position = Forward * -_orbitRadius;
                position = _orbitCenter + position;
                SetView(position);
            }
        }

        /// <summary>Gets or sets the orbit radius.<br>The distance form the OrbitCenter to camera Translation</br></summary>
        public float OrbitRadius
        {
            get
            {
                return _orbitRadius;
            }
            set
            {
                if (_orbitRadius != value)
                {
                    _orbitRadius = value;
                    Vector3 position = Forward * -_orbitRadius;
                    position = OrbitCenter + position;
                    SetView(position);
                    if (_useOrthographicProjection)
                    {
                        _recalculateprojectionMatrix = true;
                    }
                }
            }
        }

        /// <summary>Camera Translation</summary>
        public Vector3 Translation
        {
            get
            {
                return _translation;
            }
            private set
            {
                _translation = value;
                _recalculateViewMatrix = true;
                if (_viewport != null)
                {
                    _viewport.OnCameraMoved?.Invoke();
                }
                
            }
        }

        /// <summary>[Todo] add comment here</summary>
        public Quaternion Orientation
        {
            get
            {
                return _orientation;
            }
            private set
            {
                _orientation = value;
                _forward = Float3.Transform(Float3.Forward, Orientation);
                _up = Float3.Transform(Float3.Up, Orientation);
                _right = Float3.Transform(Float3.Right, Orientation);
                _recalculateViewMatrix = true;
            }
        }

        /// <summary>[Todo] add comment here</summary>
        public Float3 Up
        {
            get
            {
                return _up;
            }
        }
        /// <summary>[Todo] add comment here</summary>
        public Float3 Right
        {
            get
            {
                return _right;
            }
        }

        /// <summary>[Todo] add comment here</summary>
        public Float3 Forward
        {
            get
            {
                return _forward;
            }
        }

        /// <summary>[Todo] add comment here</summary>
        public Matrix ProjectionMatrix
        {
            get
            {
                return _projectionMatrix;
            }
        }

        /// <summary>
        /// Gets the cached of projection matrix.
        /// </summary>
        public Matrix ViewMatrix
        {
            get
            {
                return _viewMatrix;
            }
        }

        /// <summary>
        /// Gets the parent viewport.
        /// </summary>
        public EditorViewport Viewport
        {
            get
            {
                return _viewport;
            }
            internal set
            {
                if (value == null)
                {
                    value.SizeChanged -= ViewportSizeChanged;
                }
                else
                {
                    value.SizeChanged += ViewportSizeChanged;
                }
                _viewport = value;
            }
        }
        /// <summary>
        /// caled when viewport resized
        /// </summary>
        /// <param name="c"></param>
        private void ViewportSizeChanged(Control c)
        {
            _recalculateViewMatrix = true;
            _recalculateprojectionMatrix = true;
        }


        /// <inheritdoc />
        public virtual void Update(float deltaTime)
        {
            EditorViewport.ViewportInput input = _viewport.Input;
            if (input.IsZooming)
            {
                OrbitRadius = Mathf.Max(OrbitRadius - input.MouseWheelDelta * (OrbitRadius * 0.25f), 0.001f);
            }
            if (IsMovable)
            {
                var movementSpeed = MovementSpeed;
                if (input.IsShiftDown)
                {
                    movementSpeed *= 2f;
                }
                if (input.IsMoving)
                {
                    Vector3 axis = input.Axis * 100f * movementSpeed;
                    //if (_useOrthographicProjection)
                    //{
                    //    OrbitRadius = Mathf.Max(OrbitRadius - axis.Z * deltaTime, 0.001f);
                    //    axis *= new Float3(1f, 1f, 0f);
                    //}
                    OrbitCenter += axis * Orientation * deltaTime;
                }
                if (input.IsRotating)
                {
                    if (LockRoll)
                    {
                        Orientation = Quaternion.RotationAxis(Float3.Up, input.MouseDelta.X * 0.1f * deltaTime) 
                            * Orientation 
                            * Quaternion.RotationAxis(Float3.Right, input.MouseDelta.Y * 0.1f * MouseSensitivity * deltaTime);
                    }
                    else
                    {
                        Orientation *=
                        Quaternion.RotationYawPitchRoll
                        (
                            input.MouseDelta.X * 0.1f * MouseSensitivity * deltaTime,
                            input.MouseDelta.Y * 0.1f * MouseSensitivity * deltaTime,
                            input.Roll * Mathf.PiOverTwo * MouseSensitivity * deltaTime
                        );
                    }
                    _orbitCenter = Translation + Forward * _orbitRadius;
                }
                if (input.IsPanning)
                {
                    OrbitCenter += new Vector3(input.MouseDelta.X * MouseSensitivity, (InvertPanning ? input.MouseDelta.Y : -input.MouseDelta.Y) * MouseSensitivity, 0) * Orientation;
                }
            }

            if (input.IsOrbiting)
            {
                if (LockRoll)
                {
                    Orientation = Quaternion.RotationAxis(Float3.Up, input.MouseDelta.X * 0.1f * MouseSensitivity * deltaTime) * Orientation * Quaternion.RotationAxis(Float3.Right, input.MouseDelta.Y * 0.1f * MouseSensitivity * deltaTime);
                }
                else
                {
                    Orientation *=
                    Quaternion.RotationYawPitchRoll
                    (
                        input.MouseDelta.X * 0.1f * MouseSensitivity * deltaTime,
                        input.MouseDelta.Y * 0.1f * MouseSensitivity * deltaTime,
                        input.Roll * Mathf.PiOverTwo * MouseSensitivity * deltaTime
                    );
                }
                Vector3 position = _orbitCenter + Forward * -_orbitRadius;
                SetView(position);
            }
            if (_recalculateViewMatrix)
            {
                CalculateViewMatrix();
            }
            if (_recalculateprojectionMatrix)
            {
                CalculateProjectionMatrix();
            }
            if (AnimatedActions.Count != 0)
            {
                if (ProccesAction(AnimatedActions[0],deltaTime))
                {
                    AnimatedActions.RemoveAt(0);
                }
            }
        }
        /// <summary>
        /// add or sets the <see cref="AnimatedAction"/>
        /// </summary>
        /// <param name="orientation"></param>
        public void AddOrSetAnimatedAction(Quaternion orientation)
        {
            if (AnimatedActions.Count == 0)
            {
                AnimatedActions.Add(new AnimatedAction(_orbitRadius,_orbitCenter, orientation));
            }
            else
            {
                AnimatedActions[0] = new AnimatedAction(_orbitRadius, _orbitCenter, orientation);
            }
        }
        bool ProccesAction(AnimatedAction action, float delta)
        {
            
            const float e = 0.001f;
            action.curentSpeed += Mathf.Pi * delta;
            delta = action.curentSpeed;

            Translation = Vector3.Lerp(_orbitCenter, action._orbitCenter, delta);
            OrbitRadius = Mathf.Lerp(_orbitRadius, action._orbitRadius, delta);
            Orientation = Quaternion.Slerp(_orientation, action._orientation, delta);

            bool a = Vector3.NearEqual(_orbitCenter, action._orbitCenter, e);
            bool b = Mathf.WithinEpsilon(_orbitRadius, action._orbitRadius, e);
            bool c = Quaternion.NearEqual(_orientation, action._orientation, e);

            return a && b && c;
        }

        /// <summary>
        /// Creates the view matrix.
        /// </summary>
        internal void CalculateViewMatrix()
        {
            if (_viewport != null)
            {
                Float3 position = Translation;
                Float3 target = position + Forward.Normalized;
                Float3 up = Up;
                Matrix.LookAt(ref position, ref target, ref up, out _viewMatrix);
                _recalculateViewMatrix = false;
            }
        }

        internal void CalculateProjectionMatrix()
        {
            if (_viewport != null)
            {
                if (_useOrthographicProjection)
                {
                    Matrix.Ortho(_viewport.Width * OrthographicScale, _viewport.Height * OrthographicScale, NearPlane, FarPlane, out _projectionMatrix);
                }
                else
                {
                    float aspect = _viewport.Width / _viewport.Height;
                    Matrix.PerspectiveFov(FieldOfView * Mathf.DegreesToRadians, aspect, NearPlane, FarPlane, out _projectionMatrix);
                }
                _recalculateprojectionMatrix = false;
            }
        }

        /// <summary>[Todo] add comment here</summary>
        public void SetOrthographicScale(float NewScale)
        {
            OrbitRadius = NewScale * _viewport.Height;
            OrthographicScale = NewScale;
        }

        /// <summary>
        /// Gets the View Ray
        /// </summary>
        /// <returns>View Ray</returns>
        public Ray GetViewRay()
        {
            return new Ray(Translation, Forward * OrbitRadius);
        }

        /// <summary>
        /// Sets view orientation and position to match the arc ball camera style view.
        /// </summary>
        /// <param name="orientation">The view rotation.</param>
        /// <param name="orbitCenter">The orbit center location.</param>
        /// <param name="orbitRadius">The orbit radius.</param>
        public void SetArcBallView(Quaternion orientation, Vector3 orbitCenter, float orbitRadius)
        {
            Orientation = orientation;
            OrbitRadius = orbitRadius;
            OrbitCenter = orbitCenter;
        }

        /// <summary>
        /// Sets view.
        /// </summary>
        /// <param name="transform">The view transform.</param>
        public void SetView(Transform transform)
        {
            Orientation = transform.Orientation;
            Translation = transform.Translation;
        }

        /// <summary>
        /// Sets view.
        /// </summary>
        /// <param name="translation">The view position.</param>
        /// <param name="orientation">The view rotation.</param>
        public void SetView(Vector3 translation, Quaternion orientation)
        {
            Orientation = orientation;
            Translation = translation;
        }

        /// <summary>
        /// Sets view.
        /// </summary>
        /// <param name="orientation">The view rotation.</param>
        public void SetView(Quaternion orientation)
        {
            Orientation = orientation;
        }

        /// <summary>
        /// Sets view.
        /// </summary>
        /// <param name="translation">The view position.</param>
        public void SetView(Vector3 translation)
        {
            Translation = translation;
        }

        /// <summary>
        /// Sets view.
        /// </summary>
        /// <param name="ray">The view ray.</param>
        public void SetView(Ray ray)
        {
            Float3 right = Float3.Cross(ray.Direction, Float3.Up);
            Float3 up = Float3.Cross(right, ray.Direction);
            _orbitRadius = ray.Direction.Length;
            SetView(ray.Position, Quaternion.LookRotation(ray.Direction, up));
        }

        /// <summary>
        /// Sets view orientation and position to match the arc ball camera style view for the given target object bounds.
        /// </summary>
        /// <param name="objectBounds">The target object bounds.</param>
        /// <param name="marginDistanceScale">The margin distance scale of the orbit radius.</param>
        public void SetArcBallView(BoundingBox objectBounds, float marginDistanceScale = 2f)
        {
            SetArcBallView(BoundingSphere.FromBox(objectBounds), marginDistanceScale);
        }

        /// <summary>
        /// Sets view orientation and position to match the arc ball camera style view for the given target object bounds.
        /// </summary>
        /// <param name="objectBounds">The target object bounds.</param>
        /// <param name="marginDistanceScale">The margin distance scale of the orbit radius.</param>
        public void SetArcBallView(BoundingSphere objectBounds, float marginDistanceScale = 2f)
        {
            SetArcBallView(new Quaternion(-0.08f, -0.92f, 0.31f, -0.23f), objectBounds.Center, objectBounds.Radius * marginDistanceScale);
        }

        /// <summary>
        /// Sets view orientation and position to match the arc ball camera style view for the given orbit radius.
        /// </summary>
        /// <param name="orbitRadius">The orbit radius.</param>
        public void SetArcBallView(float orbitRadius)
        {
            SetArcBallView(new Quaternion(-0.08f, -0.92f, 0.31f, -0.23f), Vector3.Zero, orbitRadius);
        }

        /// <summary>
        /// Moves the viewport to visualize the actor.
        /// </summary>
        /// <param name="actor">The actor to preview.</param>
        public void ShowActor(Actor actor)
        {
            BoundingSphere sphere;
            Editor.GetActorEditorSphere(actor, out sphere);
            SetArcBallView(sphere, 2f);
        }

        /// <summary>
        /// Moves the viewport to visualize selected actors.
        /// </summary>
        /// <param name="actor">The actors to show.</param>
        /// <param name="orientation">The used orientation.</param>
        public void ShowActor(Actor actor, ref Quaternion orientation)
        {
            BoundingSphere sphere;
            Editor.GetActorEditorSphere(actor, out sphere);
            ShowSphere(ref sphere, ref orientation);
        }

        /// <summary>
        /// Moves the viewport to visualize selected actors.
        /// </summary>
        /// <param name="selection">The actors to show.</param>
        public void ShowActors(List<SceneGraphNode> selection)
        {
            BoundingSphere mergesSphere = BoundingSphere.Empty;
            for (int i = 0; i < selection.Count; i++)
            {
                BoundingSphere sphere;
                selection[i].GetEditorSphere(out sphere);
                BoundingSphere.Merge(ref mergesSphere, ref sphere, out mergesSphere);
            }
            if (mergesSphere != BoundingSphere.Empty)
            {
                SetArcBallView(mergesSphere, 2f);
            }
        }

        /// <summary>
        /// Moves the viewport to visualize selected actors.
        /// </summary>
        /// <param name="selection">The actors to show.</param>
        /// <param name="orientation">The used orientation.</param>
        public void ShowActors(List<SceneGraphNode> selection, ref Quaternion orientation)
        {
            BoundingSphere mergesSphere = BoundingSphere.Empty;
            for (int i = 0; i < selection.Count; i++)
            {
                BoundingSphere sphere;
                selection[i].GetEditorSphere(out sphere);
                BoundingSphere.Merge(ref mergesSphere, ref sphere, out mergesSphere);
            }
            SetArcBallView(orientation, mergesSphere.Center, mergesSphere.Radius);
        }

        /// <summary>
        /// Moves the camera to visualize given world area defined by the sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <param name="orientation">The camera orientation.</param>
        public void ShowSphere(ref BoundingSphere sphere, ref Quaternion orientation)
        {
            SetArcBallView(orientation, sphere.Center, sphere.Radius);
        }

        /// <summary>
        /// saves the camera setting in to Xml
        /// </summary>
        /// <param name="writer">The Xml writer.</param>
        public void Serialize(ref System.Xml.XmlWriter writer)
        {
            writer.WriteAttributeString("NearPlane", Viewport.Camera.NearPlane.ToString());
            writer.WriteAttributeString("FarPlane", Viewport.Camera.FarPlane.ToString());
            writer.WriteAttributeString("FieldOfView", Viewport.Camera.FieldOfView.ToString());
            writer.WriteAttributeString("MovementSpeed", Viewport.Camera.MovementSpeed.ToString());
            writer.WriteAttributeString("OrthographicScale", Viewport.Camera.OrthographicScale.ToString());
            writer.WriteAttributeString("UseOrthographicProjection", Viewport.Camera.UseOrthographicProjection.ToString());

            writer.WriteAttributeString("OrbitCenter.X", ((double)_orbitCenter.X).ToString());
            writer.WriteAttributeString("OrbitCenter.Y", ((double)_orbitCenter.Y).ToString());
            writer.WriteAttributeString("OrbitCenter.Z", ((double)_orbitCenter.Z).ToString());

            writer.WriteAttributeString("OrbitRadius", ((double)_orbitRadius).ToString());

            writer.WriteAttributeString("Translation.X", ((double)_translation.X).ToString());
            writer.WriteAttributeString("Translation.Y", ((double)_translation.Y).ToString());
            writer.WriteAttributeString("Translation.Z", ((double)_translation.Z).ToString());

            writer.WriteAttributeString("Orientation.X", ((double)_orientation.X).ToString());
            writer.WriteAttributeString("Orientation.Y", ((double)_orientation.Y).ToString());
            writer.WriteAttributeString("Orientation.Z", ((double)_orientation.Z).ToString());
            writer.WriteAttributeString("Orientation.W", ((double)_orientation.W).ToString());
        }
        /// <summary>
        /// try's to load the camera setting from Xml
        /// </summary>
        /// <param name="node">The Xml document node.</param>
        public void Deserialize(ref System.Xml.XmlElement node)
        {
            if (float.TryParse(node.GetAttribute("NearPlane"), out float value2))
                NearPlane = value2;

            if (float.TryParse(node.GetAttribute("FarPlane"), out value2))
                FarPlane = value2;

            if (float.TryParse(node.GetAttribute("FieldOfView"), out value2))
                FieldOfView = value2;

            if (float.TryParse(node.GetAttribute("MovementSpeed"), out value2))
                MovementSpeed = value2;

            if (float.TryParse(node.GetAttribute("OrthographicScale"), out value2))
                OrthographicScale = value2;

            if (bool.TryParse(node.GetAttribute("UseOrthographicProjection"), out var value1))
                UseOrthographicProjection = value1;

            double value4;
            
            if (double.TryParse(node.GetAttribute("OrbitCenter.X"), out value4))
            {
                _orbitCenter.X = (float)value4;
            }
            if (double.TryParse(node.GetAttribute("OrbitCenter.Y"), out value4))
            {
                _orbitCenter.Y = (float)value4;
            }
            if (double.TryParse(node.GetAttribute("OrbitCenter.Z"), out value4))
            {
                _orbitCenter.Z = (float)value4;
            }
            if (double.TryParse(node.GetAttribute("OrbitRadius"), out value4))
            {
                _orbitRadius = (float)value4;
            }
            ;
            if (double.TryParse(node.GetAttribute("Translation.X"), out value4))
            {
                _translation.X = (float)value4;
            }
            ;
            if (double.TryParse(node.GetAttribute("Translation.Y"), out value4))
            {
                _translation.Y = (float)value4;
            }
            if (double.TryParse(node.GetAttribute("Translation.Z"), out value4))
            {
                _translation.Z = (float)value4;
            }
            if (double.TryParse(node.GetAttribute("Orientation.X"), out value4))
            {
                _orientation.X = (float)value4;
            }
            if (double.TryParse(node.GetAttribute("Orientation.Y"), out value4))
            {
                _orientation.Y = (float)value4;
            }
            if (double.TryParse(node.GetAttribute("Orientation.Z"), out value4))
            {
                _orientation.Z = (float)value4;
            }
            if (double.TryParse(node.GetAttribute("Orientation.W"), out value4))
            {
                _orientation.W = (float)value4;
            }
            if (Quaternion.NearEqual(_orientation, Quaternion.Zero))
            {
                _orientation = new Quaternion(0,0,0,1);
            }

            var e = _useOrthographicProjection;

            _orientation.Normalize();
            _forward = Float3.Transform(Float3.Forward, Orientation);
            _up = Float3.Transform(Float3.Up, Orientation);
            _right = Float3.Transform(Float3.Right, Orientation);
            _recalculateViewMatrix = true;
            _recalculateprojectionMatrix = true;
        }
    }
}
