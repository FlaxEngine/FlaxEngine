using System;
using FlaxEngine;

/// <summary>
/// Camera actor with touch orbit / pinch-zoom / mouse drag / keyboard controls.
/// Single-finger drag orbits around the target, two-finger pinch zooms,
/// mouse drag orbits, mouse wheel zooms, WASD flies (desktop parity).
/// </summary>
public class TouchCameraRig : Script
{
    private Float3 _target = new Float3(0, 100, 0);
    private float _distance = 900f;
    private float _yaw = 45f;
    private float _pitch = 25f;

    private bool _orbiting;
    private Float2 _lastPointer;
    private bool _pinching;
    private float _lastPinchDistance;
    private Float2 _lastMousePos;
    private bool _hasLastMousePos;

    /// <inheritdoc />
    public override void OnStart()
    {
        if (Actor is Camera camera)
        {
            camera.UsePerspective = true;
            camera.FieldOfView = 60f;
        }
        UpdateTransform();
    }

    /// <inheritdoc />
    public override void OnUpdate()
    {
        // --- Mouse input (desktop parity) ---
        Float2 mousePos = Input.MousePosition;
        if (Input.GetMouseButton(MouseButton.Right) || Input.GetMouseButton(MouseButton.Left))
        {
            if (_hasLastMousePos)
            {
                Float2 delta = mousePos - _lastMousePos;
                _yaw += delta.X * 0.3f;
                _pitch = Mathf.Clamp(_pitch + delta.Y * 0.2f, 5f, 85f);
            }
        }
        _lastMousePos = mousePos;
        _hasLastMousePos = true;

        float wheel = Input.MouseScrollDelta;
        if (Math.Abs(wheel) > 0.001f)
            _distance = Mathf.Clamp(_distance * (1f - wheel * 0.1f), 200f, 2500f);

        // Keyboard fly support
        float speed = _distance * 0.5f * Time.DeltaTime;
        Float3 forward = Actor.Forward;
        Float3 right = Float3.Normalize(Float3.Cross(forward, Float3.Up));
        if (Input.GetKey(KeyboardKeys.W))
            _target += forward * speed;
        if (Input.GetKey(KeyboardKeys.S))
            _target -= forward * speed;
        if (Input.GetKey(KeyboardKeys.A))
            _target -= right * speed;
        if (Input.GetKey(KeyboardKeys.D))
            _target += right * speed;

        UpdateTransform();
    }

    private void UpdateTransform()
    {
        _pitch = Mathf.Clamp(_pitch, 5f, 85f);
        float yawRad = _yaw * Mathf.DegreesToRadians;
        float pitchRad = _pitch * Mathf.DegreesToRadians;
        float cosPitch = Mathf.Cos(pitchRad);
        Float3 offset = new Float3(
            Mathf.Sin(yawRad) * cosPitch,
            Mathf.Sin(pitchRad),
            Mathf.Cos(yawRad) * cosPitch) * _distance;
        Actor.Position = _target + offset;
        Actor.LookAt(_target);
    }

    /// <inheritdoc />
    public override void OnEnable()
    {
        Input.TouchDown += OnTouchDown;
        Input.TouchMove += OnTouchMove;
        Input.TouchUp += OnTouchUp;
    }

    /// <inheritdoc />
    public override void OnDisable()
    {
        Input.TouchDown -= OnTouchDown;
        Input.TouchMove -= OnTouchMove;
        Input.TouchUp -= OnTouchUp;
    }

    private void OnTouchDown(Float2 position, int index)
    {
        if (index == 0)
        {
            _orbiting = true;
            _lastPointer = position;
        }
        if (index == 1)
        {
            // Second finger lands: start pinch, stop orbit
            _orbiting = false;
            _pinching = true;
        }
    }

    private void OnTouchMove(Float2 position, int index)
    {
        if (index == 0 && _orbiting)
        {
            Float2 delta = position - _lastPointer;
            _yaw -= delta.X * 0.4f;
            _pitch = Mathf.Clamp(_pitch + delta.Y * 0.3f, 5f, 85f);
            _lastPointer = position;
        }
        if (index == 1 && _pinching)
        {
            // Approximate pinch zoom using second finger movement along Y
            // (two-finger distance is not exposed; use per-finger deltas)
            _distance = Mathf.Clamp(_distance + (position.Y - _lastPointer.Y) * 4f, 200f, 2500f);
            _lastPointer = position;
        }
    }

    private void OnTouchUp(Float2 position, int index)
    {
        if (index == 0)
            _orbiting = false;
        if (index == 1)
            _pinching = false;
    }
}
