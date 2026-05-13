namespace FlaxEngine;

partial class GameplayGlobals
{
    /// <summary>
    /// Gets a value of a given type from the global variables. (it must be added first).
    /// </summary>
    /// <param name="name">The name of the variable to retrieve.</param>
    /// <typeparam name="T">The type of the variable to retrieve.</typeparam>
    /// <returns>The value of the variable, or default if not found or type mismatch.</returns>
    public T GetValue<T>(string name)
    {
        var obj = GetValue(name);
        if (obj is T t)
            return t;
        return default;
    }
}
