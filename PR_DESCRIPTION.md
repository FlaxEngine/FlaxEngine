## Summary

Fix macOS Delete key handling so the main keyboard Delete key works with editor shortcuts and runtime input checks.

On macOS, the main keyboard Delete key is reported by the platform as `Backspace`. Several editor paths bind deletion to `KeyboardKeys.Delete`, so pressing the physical Delete key could trigger Backspace-specific behavior, such as Content window history navigation, instead of deleting the current selection.

## Changes

- Treat macOS `Backspace` as an alias for `KeyboardKeys.Delete` in editor input bindings.
- Treat macOS `Backspace` as an alias for `KeyboardKeys.Delete` in keyboard polling (`GetKey`, `GetKeyDown`, `GetKeyUp`).
- Forward runtime `KeyDown`/`KeyUp` delegates for `KeyboardKeys.Delete` when macOS reports the physical Delete key as `Backspace`.
- Preserve text editing behavior by keeping the original key event as `Backspace` for UI controls.
- Check the Delete binding before Content view Backspace navigation so deleting a selected content item wins over folder history navigation.

## Text Input Notes

The platform key mapping is not changed from `Backspace` to `Delete` at the window event level. Text controls still receive `Backspace`, so deleting text to the left of the caret keeps working as expected. The alias is applied only where shortcut matching or input polling asks whether `KeyboardKeys.Delete` is active.

## Testing

- Built the managed project with:

```bash
dotnet build Source/FlaxEngine.csproj --no-restore
```

Result: build succeeded with 0 warnings and 0 errors.
