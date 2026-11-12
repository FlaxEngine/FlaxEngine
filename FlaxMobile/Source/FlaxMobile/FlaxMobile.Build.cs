using Flax.Build;
using Flax.Build.NativeCpp;

public class FlaxMobile : GameModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        // C#-only scripting
        BuildNativeCode = false;
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        // Ignore compilation warnings due to missing code documentation comments
        options.ScriptingAPI.IgnoreMissingDocumentationWarnings = true;
    }
}
