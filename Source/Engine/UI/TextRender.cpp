// Copyright (c) Wojciech Figat. All rights reserved.

#include "TextRender.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Models/Types.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Render2D/Font.h"
#include "Engine/Render2D/FontAsset.h"
#include "Engine/Render2D/FontManager.h"
#include "Engine/Render2D/FontTextureAtlas.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Content/Assets/MaterialInstance.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Deprecated.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "Engine/Localization/Localization.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif

PACK_STRUCT(struct TextRenderVertex
{
    Float3 Position;
    Color32 Color;
    FloatR10G10B10A2 Normal;
    FloatR10G10B10A2 Tangent;
    Half2 TexCoord;

    static GPUVertexLayout* GetLayout()
    {
        return GPUVertexLayout::Get({
            { VertexElement::Types::Position, 0, 0, 0, PixelFormat::R32G32B32_Float },
            { VertexElement::Types::Color, 0, 0, 0, PixelFormat::R8G8B8A8_UNorm },
            { VertexElement::Types::Normal, 0, 0, 0, PixelFormat::R10G10B10A2_UNorm },
            { VertexElement::Types::Tangent, 0, 0, 0, PixelFormat::R10G10B10A2_UNorm },
            { VertexElement::Types::TexCoord, 0, 0, 0, PixelFormat::R16G16_Float },
        });
    }
});

TextRender::TextRender(const SpawnParams& params)
    : Actor(params)
    , _size(32)
    , _ib(0, sizeof(uint16))
    , _vb(0, sizeof(TextRenderVertex), String::Empty, TextRenderVertex::GetLayout())
{
    _color = Color::White;
    _localBox = BoundingBox(Vector3::Zero);
    _layoutOptions.Bounds = Rectangle(-100, -100, 200, 200);
    _layoutOptions.HorizontalAlignment = TextAlignment::Center;
    _layoutOptions.VerticalAlignment = TextAlignment::Center;
    _layoutOptions.TextWrapping = TextWrapping::NoWrap;
    _layoutOptions.Scale = 1.0f;
    _layoutOptions.BaseLinesGapScale = 1.0f;

    // Link events
    Font.Changed.Bind<TextRender, &TextRender::Invalidate>(this);
    Font.Unload.Bind<TextRender, &TextRender::Invalidate>(this);
    Material.Changed.Bind<TextRender, &TextRender::Invalidate>(this);
}

const LocalizedString& TextRender::GetText() const
{
    return _text;
}

void TextRender::SetText(const LocalizedString& value)
{
    if (_text != value)
    {
        _text = value;
        _isDirty = true;
    }
}

Color TextRender::GetColor() const
{
    return _color;
}

void TextRender::SetColor(const Color& value)
{
    if (_color != value)
    {
        _color = value;
        _isDirty = true;
    }
}

float TextRender::GetFontSize() const
{
    return _size;
}

void TextRender::SetFontSize(float value)
{
    value = Math::Clamp(value, 1.0f, 1024.0f);
    if (_size != value)
    {
        _size = value;
        _isDirty = true;
    }
}

void TextRender::SetLayoutOptions(TextLayoutOptions& value)
{
    if (_layoutOptions != value)
    {
        _layoutOptions = value;
        _isDirty = true;
    }
}

void TextRender::UpdateLayout()
{
    // Clear
    _ib.Clear();
    _vb.Clear();
    _localBox = BoundingBox(Vector3::Zero);
    BoundingBox::Transform(_localBox, _transform, _box);
    BoundingSphere::FromBox(_box, _sphere);
#if MODEL_USE_PRECISE_MESH_INTERSECTS
    _collisionProxy.Clear();
#endif

    // Skip if no font in use
    if (Font == nullptr || !Font->IsLoaded())
        return;

    // Skip if material is not ready
    if (Material == nullptr || !Material->IsLoaded() || !Material->IsReady())
        return;

    // Clear flag
    _isDirty = false;

    // Skip if no need to calculate the layout
    String textData;
    String* textPtr = &_text.Value;
    if (textPtr->IsEmpty())
    {
        if (_text.Id.IsEmpty())
            return;
        textData = Localization::GetString(_text.Id);
        textPtr = &textData;
        if (!_isLocalized)
        {
            _isLocalized = true;
            Localization::LocalizationChanged.Bind<TextRender, &TextRender::UpdateLayout>(this);
        }
        if (textPtr->IsEmpty())
            return;
    }
    else if (_isLocalized)
    {
        _isLocalized = false;
        Localization::LocalizationChanged.Unbind<TextRender, &TextRender::UpdateLayout>(this);
    }
    const String& text = *textPtr;

    // Pick a font (remove DPI text scale as the text is being placed in the world)
    auto font = Font->CreateFont(_size);
    float scale = _layoutOptions.Scale / FontManager::FontScale;

    // Prepare
    FontTextureAtlas* fontAtlas = nullptr;
    Float2 invAtlasSize = Float2::One;
    FontCharacterEntry previous;
    int32 kerning;

    // Perform layout
    Array<FontLineCache, InlinedAllocation<8>> lines;
    font->ProcessText(text, lines, _layoutOptions);

    // Prepare buffers capacity
    _ib.Data.EnsureCapacity(text.Length() * 6 * sizeof(uint16));
    _vb.Data.EnsureCapacity(text.Length() * 4 * sizeof(TextRenderVertex));
    _buffersDirty = true;

    // Init draw chunks data
    DrawChunk drawChunk;
    drawChunk.Actor = this;
    drawChunk.StartIndex = 0;
    _drawChunks.Clear();

    // Render all lines
    uint16 vertexCounter = 0;
    uint16 indexCounter = 0;
    FontCharacterEntry entry;
    BoundingBox box = BoundingBox::Empty;
    Color32 color(_color);
    for (int32 lineIndex = 0; lineIndex < lines.Count(); lineIndex++)
    {
        const FontLineCache& line = lines[lineIndex];
        Float2 pointer = line.Location;

        // Render all characters from the line
        for (int32 charIndex = line.FirstCharIndex; charIndex <= line.LastCharIndex; charIndex++)
        {
            const Char c = text[charIndex];
            if (c != '\n')
            {
                font->GetCharacter(c, entry);

                // Check if need to select/change font atlas (since characters even in the same font may be located in different atlases)
                if (fontAtlas == nullptr || entry.TextureIndex != drawChunk.FontAtlasIndex)
                {
                    // Check if need to change atlas (enqueue draw chunk)
                    if (fontAtlas)
                    {
                        drawChunk.IndicesCount = (_ib.Data.Count() / sizeof(uint16)) - drawChunk.StartIndex;
                        if (drawChunk.IndicesCount > 0)
                        {
                            _drawChunks.Add(drawChunk);
                        }
                        drawChunk.StartIndex = indexCounter;
                    }

                    // Get texture atlas that contains current character
                    drawChunk.FontAtlasIndex = entry.TextureIndex;
                    fontAtlas = FontManager::GetAtlas(drawChunk.FontAtlasIndex);
                    if (fontAtlas)
                    {
                        fontAtlas->EnsureTextureCreated();
                        invAtlasSize = 1.0f / fontAtlas->GetSize();
                    }
                    else
                    {
                        invAtlasSize = 1.0f;
                    }

                    // Setup material
                    drawChunk.Material = Content::CreateVirtualAsset<MaterialInstance>();
                    drawChunk.Material->SetBaseMaterial(Material.Get());
                    for (auto& param : drawChunk.Material->Params)
                        param.SetIsOverride(false);

                    // Set the font parameter
                    static StringView FontParamName = TEXT("Font");
                    const auto param = drawChunk.Material->Params.Get(FontParamName);
                    if (param && param->GetParameterType() == MaterialParameterType::Texture)
                    {
                        param->SetValue(Variant(fontAtlas));
                        param->SetIsOverride(true);
                    }
                }

                // Get kerning
                const bool isWhitespace = StringUtils::IsWhitespace(c);
                if (!isWhitespace && previous.IsValid)
                {
                    kerning = entry.Font->GetKerning(previous.Character, entry.Character);
                }
                else
                {
                    kerning = 0;
                }
                pointer.X += (float)kerning * scale;
                previous = entry;

                // Omit whitespace characters
                if (!isWhitespace)
                {
                    // Calculate character size and atlas coordinates
                    const float x = pointer.X + (float)entry.OffsetX * scale;
                    const float y = pointer.Y + (float)(font->GetHeight() + font->GetDescender() - entry.OffsetY) * scale;

                    Rectangle charRect(x, y, entry.UVSize.X * scale, entry.UVSize.Y * scale);
                    charRect.Offset(_layoutOptions.Bounds.Location);

                    Float2 upperLeftUV = entry.UV * invAtlasSize;
                    Float2 rightBottomUV = (entry.UV + entry.UVSize) * invAtlasSize;

                    // Calculate bitangent sign
                    Float3 normal = Float3::UnitZ;
                    Float3 tangent = Float3::UnitX;
                    byte sign = 0;

                    // Write vertices
                    TextRenderVertex v;
#define WRITE_VB(pos, uv) \
					v.Position = Float3(-pos, 0.0f); \
					box.Merge(v.Position); \
					v.TexCoord = Half2(uv); \
					v.Normal = FloatR10G10B10A2(normal * 0.5f + 0.5f, 0); \
					v.Tangent = FloatR10G10B10A2(tangent * 0.5f + 0.5f, sign); \
					v.Color = color; \
					_vb.Write(v)
                    //
                    WRITE_VB(charRect.GetBottomRight(), rightBottomUV);
                    WRITE_VB(charRect.GetBottomLeft(), Float2(upperLeftUV.X, rightBottomUV.Y));
                    WRITE_VB(charRect.GetUpperLeft(), upperLeftUV);
                    WRITE_VB(charRect.GetUpperRight(), Float2(rightBottomUV.X, upperLeftUV.Y));
                    //
#undef WRITE_VB

                    const uint16 startVertex = vertexCounter;
                    vertexCounter += 4;
                    indexCounter += 6;

                    // Write indices
                    _ib.Write((uint16)(startVertex + (uint16)0));
                    _ib.Write((uint16)(startVertex + (uint16)1));
                    _ib.Write((uint16)(startVertex + (uint16)2));
                    _ib.Write((uint16)(startVertex + (uint16)2));
                    _ib.Write((uint16)(startVertex + (uint16)3));
                    _ib.Write((uint16)(startVertex + (uint16)0));
                }

                // Move
                pointer.X += (float)entry.AdvanceX * scale;
            }
        }
    }
    drawChunk.IndicesCount = (_ib.Data.Count() / sizeof(uint16)) - drawChunk.StartIndex;
    if (drawChunk.IndicesCount > 0)
    {
        _drawChunks.Add(drawChunk);
    }

#if MODEL_USE_PRECISE_MESH_INTERSECTS
    // Setup collision proxy for detailed collision detection for triangles
    const int32 totalIndicesCount = _ib.Data.Count() / sizeof(uint16);
    _collisionProxy.Init(_vb.Data.Count() / sizeof(TextRenderVertex), totalIndicesCount / 3, (const Float3*)_vb.Data.Get(), (const uint16*)_ib.Data.Get(), sizeof(TextRenderVertex));
#endif

    // Update text bounds (from build vertex positions)
    if (_ib.Data.IsEmpty())
    {
        // Empty
        box = BoundingBox(_transform.Translation);
    }
    _localBox = box;
    BoundingBox::Transform(_localBox, _transform, _box);
    BoundingSphere::FromBox(_box, _sphere);
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey);
}

bool TextRender::HasContentLoaded() const
{
    return (Material == nullptr || Material->IsLoaded()) && (Font == nullptr || Font->IsLoaded());
}

void TextRender::Draw(RenderContext& renderContext)
{
    if (renderContext.View.Pass == DrawPass::GlobalSDF)
        return; // TODO: Text rendering to Global SDF
    if (renderContext.View.Pass == DrawPass::GlobalSurfaceAtlas)
        return; // TODO: Text rendering to Global Surface Atlas
    if (_isDirty)
        UpdateLayout();
    Matrix world;
    renderContext.View.GetWorldMatrix(_transform, world);
    GEOMETRY_DRAW_STATE_EVENT_BEGIN(_drawState, world);

    const DrawPass drawModes = DrawModes & renderContext.View.Pass & renderContext.View.GetShadowsDrawPassMask(ShadowsMode);
    if (_vb.Data.Count() > 0 && drawModes != DrawPass::None)
    {
        // Flush buffers
        if (_buffersDirty)
        {
            _buffersDirty = false;
            _ib.Flush();
            _vb.Flush();
        }

        // Setup draw call
        DrawCall drawCall;
        drawCall.World = world;
        drawCall.ObjectPosition = drawCall.World.GetTranslation();
        drawCall.ObjectRadius = (float)_sphere.Radius;
        drawCall.Surface.GeometrySize = _localBox.GetSize();
        drawCall.Surface.PrevWorld = _drawState.PrevWorld;
        drawCall.WorldDeterminantSign = RenderTools::GetWorldDeterminantSign(drawCall.World);
        drawCall.PerInstanceRandom = GetPerInstanceRandom();
        drawCall.Geometry.IndexBuffer = _ib.GetBuffer();
        drawCall.Geometry.VertexBuffers[0] = _vb.GetBuffer();
        drawCall.InstanceCount = 1;

        // Submit draw calls
        for (const auto& e : _drawChunks)
        {
            const DrawPass chunkDrawModes = drawModes & e.Material->GetDrawModes();
            if (chunkDrawModes == DrawPass::None)
                continue;
            drawCall.Draw.IndicesCount = e.IndicesCount;
            drawCall.Draw.StartIndex = e.StartIndex;
            drawCall.Material = e.Material;
            renderContext.List->AddDrawCall(renderContext, chunkDrawModes, GetStaticFlags(), drawCall, true, SortOrder);
        }
    }

    GEOMETRY_DRAW_STATE_EVENT_END(_drawState, world);
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void TextRender::OnDebugDrawSelected()
{
    // Draw text bounds and layout bounds
    DEBUG_DRAW_WIRE_BOX(_box, Color::Orange, 0, true);
    OrientedBoundingBox layoutBox(Vector3(-_layoutOptions.Bounds.GetUpperLeft(), 0), Vector3(-_layoutOptions.Bounds.GetBottomRight(), 0));
    layoutBox.Transform(_transform);
    DEBUG_DRAW_WIRE_BOX(layoutBox, Color::BlueViolet, 0, true);

    // Base
    Actor::OnDebugDrawSelected();
}

#endif

void TextRender::OnLayerChanged()
{
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Layer);
}

bool TextRender::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
#if MODEL_USE_PRECISE_MESH_INTERSECTS
    if (_box.Intersects(ray))
    {
        return _collisionProxy.Intersects(ray, _transform, distance, normal);
    }
    return false;
#else
    return _box.Intersects(ray, distance, normal);
#endif
}

void TextRender::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(TextRender);

    SERIALIZE_MEMBER(Text, _text);
    SERIALIZE_MEMBER(Color, _color);
    SERIALIZE_MEMBER(Size, _size);
    SERIALIZE(Material);
    SERIALIZE(Font);
    SERIALIZE(ShadowsMode);
    SERIALIZE(DrawModes);
    SERIALIZE(SortOrder);
    SERIALIZE_MEMBER(Bounds, _layoutOptions.Bounds);
    SERIALIZE_MEMBER(HAlignment, _layoutOptions.HorizontalAlignment);
    SERIALIZE_MEMBER(VAlignment, _layoutOptions.VerticalAlignment);
    SERIALIZE_MEMBER(Wrapping, _layoutOptions.TextWrapping);
    SERIALIZE_MEMBER(Scale, _layoutOptions.Scale);
    SERIALIZE_MEMBER(GapScale, _layoutOptions.BaseLinesGapScale);
}

void TextRender::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Text, _text);
    DESERIALIZE_MEMBER(Color, _color);
    DESERIALIZE_MEMBER(Size, _size);
    DESERIALIZE(Material);
    DESERIALIZE(Font);
    DESERIALIZE(ShadowsMode);
    DESERIALIZE(DrawModes);
    DESERIALIZE(SortOrder);
    DESERIALIZE_MEMBER(Bounds, _layoutOptions.Bounds);
    DESERIALIZE_MEMBER(HAlignment, _layoutOptions.HorizontalAlignment);
    DESERIALIZE_MEMBER(VAlignment, _layoutOptions.VerticalAlignment);
    DESERIALIZE_MEMBER(Wrapping, _layoutOptions.TextWrapping);
    DESERIALIZE_MEMBER(Scale, _layoutOptions.Scale);
    DESERIALIZE_MEMBER(GapScale, _layoutOptions.BaseLinesGapScale);

    // [Deprecated on 07.02.2022, expires on 07.02.2024]
    if (modifier->EngineBuild <= 6330)
    {
        MARK_CONTENT_DEPRECATED();
        DrawModes |= DrawPass::GlobalSDF;
    }
    // [Deprecated on 27.04.2022, expires on 27.04.2024]
    if (modifier->EngineBuild <= 6331)
    {
        MARK_CONTENT_DEPRECATED();
        DrawModes |= DrawPass::GlobalSurfaceAtlas;
    }

    _isDirty = true;
}

void TextRender::OnEnable()
{
    // Base
    Actor::OnEnable();

    if (_isDirty)
    {
        UpdateLayout();
    }
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);
}

void TextRender::OnDisable()
{
    if (_isLocalized)
    {
        _isLocalized = false;
        Localization::LocalizationChanged.Unbind<TextRender, &TextRender::UpdateLayout>(this);
    }
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);

    // Base
    Actor::OnDisable();
}

void TextRender::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    BoundingBox::Transform(_localBox, _transform, _box);
    BoundingSphere::FromBox(_box, _sphere);
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey);
}
