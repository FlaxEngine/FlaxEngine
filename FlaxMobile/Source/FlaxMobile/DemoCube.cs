using System;
using FlaxEngine;

/// <summary>
/// Spinning wireframe cube demo built purely from DebugDraw (no assets needed).
/// Shows C# scripting, per-frame updates and 3D rendering on mobile.
/// </summary>
public class DemoCube : Script
{
    private Float3[] _corners;
    private Float3[] _lines;
    private Color _cubeColor = new Color(0.2f, 0.8f, 1.0f);
    private float _angle;

    /// <inheritdoc />
    public override void OnStart()
    {
        base.OnStart();
        float s = 150f;
        // 8 corners of the cube (y-up in Flax)
        _corners = new Float3[8]
        {
            new Float3(-s, -s + 150f, -s),
            new Float3(+s, -s + 150f, -s),
            new Float3(+s, +s + 150f, -s),
            new Float3(-s, +s + 150f, -s),
            new Float3(-s, -s + 150f, +s),
            new Float3(+s, -s + 150f, +s),
            new Float3(+s, +s + 150f, +s),
            new Float3(-s, +s + 150f, +s),
        };
        // 12 edges as line pairs
        int[] edges =
        {
            0, 1, 1, 2, 2, 3, 3, 0, // back face
            4, 5, 5, 6, 6, 7, 7, 4, // front face
            0, 4, 1, 5, 2, 6, 3, 7, // connectors
        };
        _lines = new Float3[24];
        for (int i = 0; i < edges.Length; i++)
            _lines[i] = _corners[edges[i]];
    }

    /// <inheritdoc />
    public override void OnUpdate()
    {
        _angle += Time.DeltaTime * 45f; // 45 deg per second
        Quaternion rotation = Quaternion.Euler(0, _angle, MathF.Sin(_angle * Mathf.DegreesToRadians) * 20f);
        Matrix transform = Matrix.Transformation(Float3.One, rotation, Actor.Position);
        DebugDraw.DrawLines(_lines, transform, _cubeColor, 0f, false);

        // Ground grid reference (drawn as line strips)
        if (_gridLines == null)
        {
            const float extent = 3000f;
            const float step = 250f;
            int lineCount = (int)(extent * 2f / step) + 1;
            var grid = new System.Collections.Generic.List<Float3>(lineCount * 4);
            for (float p = -extent; p <= extent; p += step)
            {
                grid.Add(new Float3(-extent, 0f, p));
                grid.Add(new Float3(+extent, 0f, p));
                grid.Add(new Float3(p, 0f, -extent));
                grid.Add(new Float3(p, 0f, +extent));
            }
            _gridLines = grid.ToArray();
        }
        DebugDraw.DrawLines(_gridLines, Matrix.Identity, new Color(0.35f, 0.35f, 0.35f), 0f, false);
    }

    private Float3[] _gridLines;
}
