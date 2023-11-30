#include "MultiFont.h"
#include "FontManager.h"
#include "Engine/Core/Math/Math.h"

MultiFont::MultiFont(const Array<Font*>& fonts)
    : ManagedScriptingObject(SpawnParams(Guid::New(), Font::TypeInitializer)),
    _fonts(fonts)
{

}

void MultiFont::ProcessText(const StringView& text, Array<MultiFontLineCache>& outputLines, API_PARAM(Ref) const TextLayoutOptions& layout)
{
    float cursorX = 0;
    int32 kerning;
    MultiFontLineCache tmpLine;
    MultiFontBlockCache tmpBlock;
    FontCharacterEntry entry;
    FontCharacterEntry previous;
    int32 textLength = text.Length();
    float scale = layout.Scale / FontManager::FontScale;
    float boundsWidth = layout.Bounds.GetWidth();
    float baseLinesDistanceScale = layout.BaseLinesGapScale * scale;

    tmpBlock.Location = Float2::Zero;
    tmpBlock.Size = Float2::Zero;
    tmpBlock.FirstCharIndex = 0;
    tmpBlock.LastCharIndex = -1;

    tmpLine.Location = Float2::Zero;
    tmpLine.Size = Float2::Zero;
    tmpLine.Blocks = Array<MultiFontBlockCache>();

    if (textLength == 0) {
        return;
    }

    int32 lastWrapCharIndex = INVALID_INDEX;
    float lastWrapCharX = 0;
    bool lastMoveLine = false;
    // The index of the font used by the current block
    int32 currentFontIndex = GetCharFontIndex(text[0], 0);
    // The maximum font height of the current line
    float maxHeight = 0;
    float maxAscender = 0;
    float lastCursorX = 0;

    // Process each character to split text into single lines
    for (int32 currentIndex = 0; currentIndex < textLength;)
    {
        bool moveLine = false;
        bool moveBlock = false;
        float xAdvance = 0;
        int32 nextCharIndex = currentIndex + 1;

        // Submit line and block if text ends
        if (nextCharIndex == textLength) {
            moveLine = moveBlock = true;
        }

        // Cache current character
        const Char currentChar = text[currentIndex];
        const bool isWhitespace = StringUtils::IsWhitespace(currentChar);

        // Check if character can wrap words
        const bool isWrapChar = !StringUtils::IsAlnum(currentChar) || isWhitespace || StringUtils::IsUpper(currentChar) || (currentChar >= 0x3040 && currentChar <= 0x9FFF);
        if (isWrapChar && currentIndex != 0)
        {
            lastWrapCharIndex = currentIndex;
            lastWrapCharX = cursorX;
        }

        int32 nextFontIndex = currentFontIndex;
        // Check if it's a newline character
        if (currentChar == '\n')
        {
            // Break line
            moveLine = moveBlock = true;
            tmpBlock.LastCharIndex++;
        }
        else
        {
            // Get character entry
            if (nextCharIndex < textLength) {
                nextFontIndex = GetCharFontIndex(text[nextCharIndex], currentFontIndex);
            }

            // Get character entry
            _fonts[currentFontIndex]->GetCharacter(currentChar, entry);
            maxHeight = Math::Max(maxHeight, static_cast<float>(_fonts[currentFontIndex]->GetHeight()));
            maxAscender = Math::Max(maxAscender, static_cast<float>(_fonts[currentFontIndex]->GetAscender()));

            // Move block if the font changes or text ends
            if (nextFontIndex != currentFontIndex || nextCharIndex == textLength) {
                moveBlock = true;
            }

            // Get kerning, only when the font hasn't changed
            if (!isWhitespace && previous.IsValid && !moveBlock)
            {
                kerning = _fonts[currentFontIndex]->GetKerning(previous.Character, entry.Character);
            }
            else
            {
                kerning = 0;
            }
            previous = entry;
            xAdvance = (kerning + entry.AdvanceX) * scale;

            // Check if character fits the line or skip wrapping
            if (cursorX + xAdvance <= boundsWidth || layout.TextWrapping == TextWrapping::NoWrap)
            {
                // Move character
                cursorX += xAdvance;
                tmpBlock.LastCharIndex++;
            }
            else if (layout.TextWrapping == TextWrapping::WrapWords)
            {
                if (lastWrapCharIndex != INVALID_INDEX)
                {
                    // Skip moving twice for the same character
                    int32 lastLineLastCharIndex = outputLines.HasItems() && outputLines.Last().Blocks.HasItems() ? outputLines.Last().Blocks.Last().LastCharIndex : -10000;
                    if (lastLineLastCharIndex == lastWrapCharIndex || lastLineLastCharIndex == lastWrapCharIndex - 1 || lastLineLastCharIndex == lastWrapCharIndex - 2)
                    {
                        currentIndex = nextCharIndex;
                        lastMoveLine = moveLine;
                        continue;
                    }

                    // Move line
                    const Char wrapChar = text[lastWrapCharIndex];
                    moveLine = true;
                    moveBlock = tmpBlock.FirstCharIndex < lastWrapCharIndex;

                    cursorX = lastWrapCharX;
                    if (StringUtils::IsWhitespace(wrapChar))
                    {
                        // Skip whitespaces
                        tmpBlock.LastCharIndex = lastWrapCharIndex - 1;
                        nextCharIndex = currentIndex = lastWrapCharIndex + 1;
                    }
                    else
                    {
                        tmpBlock.LastCharIndex = lastWrapCharIndex - 1;
                        nextCharIndex = currentIndex = lastWrapCharIndex;
                    }
                }
            }
            else if (layout.TextWrapping == TextWrapping::WrapChars)
            {
                // Move line
                moveLine = true;
                moveBlock = tmpBlock.FirstCharIndex < currentChar;
                nextCharIndex = currentIndex;

                // Skip moving twice for the same character
                if (lastMoveLine)
                    break;
            }
        }

        if (moveBlock) {
            // Add block
            tmpBlock.Size.X = lastCursorX - cursorX;
            tmpBlock.Size.Y = baseLinesDistanceScale * _fonts[currentFontIndex]->GetHeight();
            tmpBlock.LastCharIndex = Math::Max(tmpBlock.LastCharIndex, tmpBlock.FirstCharIndex);
            tmpBlock.FontIndex = currentFontIndex;
            tmpLine.Blocks.Add(tmpBlock);

            // Reset block
            tmpBlock.Location.X = cursorX;
            tmpBlock.FirstCharIndex = nextCharIndex;
            tmpBlock.LastCharIndex = nextCharIndex - 1;

            currentFontIndex = nextFontIndex;
            lastCursorX = cursorX;
        }

        // Check if move to another line
        if (moveLine)
        {
            // Add line
            tmpLine.Size.X = cursorX;
            tmpLine.Size.Y = baseLinesDistanceScale * maxHeight;
            tmpLine.MaxAscender = maxAscender;
            outputLines.Add(tmpLine);

            // Reset line
            tmpLine.Blocks.Clear();
            tmpLine.Location.Y += baseLinesDistanceScale * maxHeight;
            cursorX = 0;
            tmpBlock.Location.X = cursorX;
            lastWrapCharIndex = INVALID_INDEX;
            lastWrapCharX = 0;
            previous.IsValid = false;

            // Reset max font height
            maxHeight = 0;
            maxAscender = 0;
            lastCursorX = 0;
        }

        currentIndex = nextCharIndex;
        lastMoveLine = moveLine;
    }

    // Check if an additional line should be created
    if (text[textLength - 1] == '\n')
    {
        // Add line
        tmpLine.Size.X = cursorX;
        tmpLine.Size.Y = baseLinesDistanceScale * maxHeight;
        outputLines.Add(tmpLine);

        tmpLine.Location.Y += baseLinesDistanceScale * maxHeight;
    }

    // Check amount of lines
    if (outputLines.IsEmpty())
        return;

    float totalHeight = tmpLine.Location.Y;

    Float2 offset = Float2::Zero;
    if (layout.VerticalAlignment == TextAlignment::Center)
    {
        offset.Y += (layout.Bounds.GetHeight() - totalHeight) * 0.5f;
    }
    else if (layout.VerticalAlignment == TextAlignment::Far)
    {
        offset.Y += layout.Bounds.GetHeight() - totalHeight;
    }
    for (int32 i = 0; i < outputLines.Count(); i++)
    {
        MultiFontLineCache& line = outputLines[i];
        Float2 rootPos = line.Location + offset;

        // Fix upper left line corner to match desire text alignment
        if (layout.HorizontalAlignment == TextAlignment::Center)
        {
            rootPos.X += (layout.Bounds.GetWidth() - line.Size.X) * 0.5f;
        }
        else if (layout.HorizontalAlignment == TextAlignment::Far)
        {
            rootPos.X += layout.Bounds.GetWidth() - line.Size.X;
        }

        line.Location = rootPos;

        // Align all blocks to center in case they have different heights
        for (int32 j = 0; j < line.Blocks.Count(); j++)
        {
            MultiFontBlockCache& block = line.Blocks[j];
            block.Location.Y += (line.MaxAscender - _fonts[block.FontIndex]->GetAscender()) / 2;
        }
    }
}

Float2 MultiFont::GetCharPosition(const StringView& text, int32 index, const TextLayoutOptions& layout)
{
    // Check if there is no need to do anything
    if (text.IsEmpty())
        return layout.Bounds.Location;

    // Process text
    Array<MultiFontLineCache> lines;
    ProcessText(text, lines, layout);
    ASSERT(lines.HasItems());
    float scale = layout.Scale / FontManager::FontScale;
    float baseLinesDistance = layout.BaseLinesGapScale * scale;
    Float2 rootOffset = layout.Bounds.Location;

    // Find line with that position
    FontCharacterEntry previous;
    FontCharacterEntry entry;
    for (int32 lineIndex = 0; lineIndex < lines.Count(); lineIndex++)
    {
        const MultiFontLineCache& line = lines[lineIndex];
        for (int32 blockIndex = 0; blockIndex < line.Blocks.Count(); blockIndex++)
        {
            const MultiFontBlockCache& block = line.Blocks[blockIndex];
            // Check if desire position is somewhere inside characters in line range
            if (Math::IsInRange(index, block.FirstCharIndex, block.LastCharIndex))
            {
                float x = line.Location.X + block.Location.X;
                float y = line.Location.Y + block.Location.Y;

                // Check all characters in the line
                for (int32 currentIndex = block.FirstCharIndex; currentIndex < index; currentIndex++)
                {
                    // Cache current character
                    const Char currentChar = text[currentIndex];
                    _fonts[block.FontIndex]->GetCharacter(currentChar, entry);
                    const bool isWhitespace = StringUtils::IsWhitespace(currentChar);

                    // Apply kerning
                    if (!isWhitespace && previous.IsValid)
                    {
                        x += _fonts[block.FontIndex]->GetKerning(previous.Character, entry.Character);
                    }
                    previous = entry;

                    // Move
                    x += entry.AdvanceX * scale;
                }

                // Upper left corner of the character
                return rootOffset + Float2(x, y);
            }
        }
    }

    // Position after last character in the last line
    return rootOffset + Float2(lines.Last().Location.X + lines.Last().Size.X, lines.Last().Location.Y);
}

int32 MultiFont::HitTestText(const StringView& text, const Float2& location, const TextLayoutOptions& layout)
{
    // Check if there is no need to do anything
    if (text.Length() <= 0)
        return 0;

    // Process text
    Array<MultiFontLineCache> lines;
    ProcessText(text, lines, layout);
    ASSERT(lines.HasItems());
    float scale = layout.Scale / FontManager::FontScale;

    // Offset position to match lines origin space
    Float2 rootOffset = layout.Bounds.Location + lines.First().Location;
    Float2 testPoint = location - rootOffset;

    // Get block which may intersect with the position (it's possible because lines have fixed height)
    int32 lineIndex = 0;
    while (lineIndex < lines.Count())
    {
        if (lines[lineIndex].Location.Y + lines[lineIndex].Size.Y >= location.Y) {
            break;
        }

        lineIndex++;
    }
    lineIndex = Math::Clamp(lineIndex, 0, lines.Count() - 1);
    const MultiFontLineCache& line = lines[lineIndex];

    int32 blockIndex = 0;
    while (blockIndex < line.Blocks.Count() - 1)
    {
        if (line.Location.X + line.Blocks[blockIndex + 1].Location.X >= location.X) {
            break;
        }

        blockIndex++;
    }
    const MultiFontBlockCache& block = line.Blocks[blockIndex];
    float x = line.Location.X;

    // Check all characters in the line to find hit point
    FontCharacterEntry previous;
    FontCharacterEntry entry;
    int32 smallestIndex = INVALID_INDEX;
    float dst, smallestDst = MAX_float;
    for (int32 currentIndex = block.FirstCharIndex; currentIndex <= block.LastCharIndex; currentIndex++)
    {
        // Cache current character
        const Char currentChar = text[currentIndex];

        _fonts[block.FontIndex]->GetCharacter(currentChar, entry);
        const bool isWhitespace = StringUtils::IsWhitespace(currentChar);

        // Apply kerning
        if (!isWhitespace && previous.IsValid)
        {
            x += _fonts[block.FontIndex]->GetKerning(previous.Character, entry.Character);
        }
        previous = entry;

        // Test
        dst = Math::Abs(testPoint.X - x);
        if (dst < smallestDst)
        {
            // Found closer character
            smallestIndex = currentIndex;
            smallestDst = dst;
        }
        else if (dst > smallestDst)
        {
            // Current char is worse so return the best result
            return smallestIndex;
        }

        // Move
        x += entry.AdvanceX * scale;
    }

    // Test line end edge
    dst = Math::Abs(testPoint.X - x);
    if (dst < smallestDst)
    {
        // Pointer is behind the last character in the line
        smallestIndex = block.LastCharIndex;

        // Fix for last line
        if (lineIndex == lines.Count() - 1)
            smallestIndex++;
    }

    return smallestIndex;
}

Float2 MultiFont::MeasureText(const StringView& text, const TextLayoutOptions& layout)
{
    // Check if there is no need to do anything
    if (text.IsEmpty())
        return Float2::Zero;

    // Process text
    Array<MultiFontLineCache> lines;
    ProcessText(text, lines, layout);

    // Calculate bounds
    Float2 max = Float2::Zero;
    for (int32 i = 0; i < lines.Count(); i++)
    {
        const MultiFontLineCache& line = lines[i];
        max = Float2::Max(max, line.Location + line.Size);
    }

    return max;
}

