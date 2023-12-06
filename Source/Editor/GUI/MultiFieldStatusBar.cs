
using System.Collections.Generic;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI;

/// <summary>
/// Status bar with multiple fields
/// </summary>
public class MultiFieldStatusBar : ContainerControl
{
    /// <summary>
    /// Name of the default field if only one is used
    /// </summary>
    public const string DefaultFieldName = "_DefaultField";
    public const int DefaultHeight = 22;

    public struct StatusBarField
    {
        public string Text;
        public Color TextColor;
        public float RelativeX;
        public float RelativeWidth;
    }

    /// <summary>
    /// Definition of a field inside a status bar
    /// </summary>
    public Dictionary<string, StatusBarField> Field = new();

    /// <summary>
    /// Set text for a default field
    /// </summary>
    public string Text
    {
        get => GetText(DefaultFieldName);
        set => SetText(DefaultFieldName, value);
    }

    /// <summary>
    /// Set text color for a default field
    /// </summary>
    public Color TextColor
    {
        get => GetTextColor(DefaultFieldName);
        set => SetTextColor(DefaultFieldName, value);
    }

    /// <summary>
    /// Gets or sets the color of the status strip.
    /// </summary>
    public Color StatusColor
    {
        get => BackgroundColor;
        set => BackgroundColor = value;
    }

    /// <summary>
    /// Set the Text for the named field
    /// </summary>
    /// <param name="name"></param>
    /// <param name="text"></param>
    public void SetText(string name, string text)
    {
        if (Field.TryGetValue(name, out var field))
        {
            field.Text = text;
            Field[name] = field;
        }
        else
        {
            // add a default field spanning the available width for compatibility with a single field status bar
            Field[DefaultFieldName] = new StatusBarField
            {
                Text = text,
                TextColor = Style.Current.Foreground,
                RelativeX = 0f,
                RelativeWidth = 1f
            };
        }
    }

    /// <summary>
    /// Get the text of the named field
    /// </summary>
    /// <param name="name"></param>
    /// <returns></returns>
    public string GetText(string name)
    {
        if (Field.TryGetValue(name, out var field))
        {
            return field.Text;
        }
        return "";
    }

    public void SetTextColor(string name, Color color)
    {
        if (Field.TryGetValue(name, out var field))
        {
            field.TextColor = color;
            Field[name] = field;
        }
        else
        {
            // add a default field spanning the available width for compatibility with a single field status bar
            Field[DefaultFieldName] = new StatusBarField
            {
                Text = "",
                TextColor = color,
                RelativeX = 0f,
                RelativeWidth = 1f
            };
        }
    }

    public Color GetTextColor(string name)
    {
        return Field.TryGetValue(name, out var field) ? field.TextColor : Style.Current.Foreground;
    }

    public void SetRelativeWidthDefaultField(float value)
    {
        if (Field.TryGetValue(DefaultFieldName, out var field))
        {
            field.RelativeWidth = value;
            Field[DefaultFieldName] = field;
        }
    }
    
    /// <inheritdoc />
    public override void Draw()
    {
        base.Draw();
        var style = Style.Current;

        // Draw size grip
        if (Root is WindowRootControl window && !window.IsMaximized)
            Render2D.DrawSprite(style.StatusBarSizeGrip, new Rectangle(Width - 12, 10, 12, 12), style.Foreground);

        foreach (var keyValue in Field)
        {
            var field = keyValue.Value;
            var x = (Width - 24) * field.RelativeX + 4;
            var width = (Width - 24) * field.RelativeWidth;
            // Draw status field text
            Render2D.DrawText(style.FontSmall, field.Text, new Rectangle(x, 0, width, Height),
                              field.TextColor, TextAlignment.Near, TextAlignment.Center);
        }
    }
}
