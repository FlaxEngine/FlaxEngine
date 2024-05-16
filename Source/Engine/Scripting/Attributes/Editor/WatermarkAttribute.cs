using System;

namespace FlaxEngine;

/// <summary>
/// Used to add a watermark to a string textbox in the editor field
/// </summary>
[Serializable]
[AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
public class WatermarkAttribute : Attribute
{
    /// <summary>
    /// The watermark text.
    /// </summary>
    public string WatermarkText;

    /// <summary>
    /// The watermark color (as 32-bit uint in RGB order, bottom bits contain Blue).
    /// </summary>
    public uint WatermarkColor;

    /// <summary>
    /// Initializes a new instance of the  <see cref="WatermarkAttribute"/> class.
    /// </summary>
    /// <param name="text">The watermark text.</param>
    public WatermarkAttribute(string text)
    {
        WatermarkText = text;
        WatermarkColor = 0; // Default color of watermark in textbox
    }

    /// <summary>
    /// Initializes a new instance of the  <see cref="WatermarkAttribute"/> class.
    /// </summary>
    /// <param name="text">The watermark text.</param>
    /// <param name="color">The watermark color. 0 to use default.</param>
    public WatermarkAttribute(string text, uint color)
    {
        WatermarkText = text;
        WatermarkColor = color;
    }
}
