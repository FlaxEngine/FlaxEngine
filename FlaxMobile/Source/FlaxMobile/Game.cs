using FlaxEngine;

/// <summary>
/// Game bootstrap script. Builds the whole demo scene at runtime:
/// camera rig, lights, sky, UI canvas and the demo cube.
/// This keeps the project asset-free (no .flax assets to cook beyond a scene).
/// </summary>
public class Game : Script
{
    /// <inheritdoc />
    public override void OnStart()
    {
        // Lighting
        var sun = Actor.AddChild<DirectionalLight>();
        sun.Name = "Sun";
        sun.Position = new Float3(600, 1000, 400);
        sun.LookAt(Float3.Zero);
        sun.Brightness = 8f;
        sun.Color = new Color(1.0f, 0.96f, 0.9f);

        var skyLight = Actor.AddChild<SkyLight>();
        skyLight.Name = "SkyLight";
        skyLight.Position = new Float3(0, 500, 0);
        skyLight.Brightness = 0.6f;

        // Sky atmosphere
        var sky = Actor.AddChild<Sky>();
        sky.Name = "Sky";
        sky.Position = new Float3(0, 300, 0);
        sky.SunLight = sun;

        // Camera
        var camera = Actor.AddChild<Camera>();
        camera.Name = "MainCamera";
        camera.AddScript<TouchCameraRig>();

        // Demo cube
        var cube = new EmptyActor
        {
            Name = "DemoCube",
            Position = Float3.Zero,
        };
        cube.AddScript<DemoCube>();
        cube.Parent = Actor;
    }
}
