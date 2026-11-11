using Flax.Build;

public class FlaxGameTarget : GameProjectTarget
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        // Reference the modules for game
        Modules.Add("FlaxMobile");
    }
}
