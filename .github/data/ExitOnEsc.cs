using FlaxEngine;

public class ExitOnEsc : Script
{
    /// <inheritdoc />
    public override void OnUpdate()
    {
        // Exit as soon as game starts update loaded level
        Engine.RequestExit();
    }
}
