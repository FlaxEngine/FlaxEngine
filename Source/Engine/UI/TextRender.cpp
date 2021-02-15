// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "TextRender.h"
#include "Engine/Core/Math/OrientedBoundingBox.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Models/Types.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Render2D/Font.h"
#include "Engine/Render2D/FontAsset.h"
#include "Engine/Render2D/FontManager.h"
#include "Engine/Render2D/FontTextureAtlas.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Content/Assets/MaterialInstance.h"
#include "Engine/Content/Content.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Graphics/RenderTask.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif

TextRender::TextRender(const SpawnParams& params)
    : Actor(params)
    , _size(32)
    , _ib(0, sizeof(uint16))
    , _vb0(0, sizeof(VB0ElementType))
    , _vb1(0, sizeof(VB1ElementType))
    , _vb2(0, sizeof(VB2ElementType))
{
    _world = Matrix::Identity;
    _color = Color::White;
    _localBox = BoundingBox(Vector3::Zero, Vector3::Zero);
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

void TextRender::SetText(const StringView& value)
{
    if (_text != value)
    {
        _text = value;
        _isDirty = true;
    }
}

void TextRender::SetColor(const Color& value)
{
    if (_color != value)
    {
        _color = value;
        _isDirty = true;
    }
}

void TextRender::SetFontSize(int32 value)
{
    value = Math::Clamp(value, 1, 1024);
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
    if (!_isDirty)
        return;

    // Clear
    _ib.Clear();
    _vb0.Clear();
    _vb1.Clear();
    _vb2.Clear();
    _localBox = BoundingBox(Vector3::Zero, Vector3::Zero);
    BoundingBox::Transform(_localBox, _world, _box);
    BoundingSphere::FromBox(_box, _sphere);
#if USE_PRECISE_MESH_INTERSECTS
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
    if (_text.IsEmpty())
        return;

    // Pick a font (remove DPI text scale as the text is being placed in the world)
    auto font = Font->CreateFont(_size);
    float scale = 1.0f / FontManager::FontScale;

    // Prepare
    FontTextureAtlas* fontAtlas = nullptr;
    Vector2 invAtlasSize = Vector2::One;
    FontCharacterEntry previous;
    int32 kerning;

    // Perform layout
    Array<FontLineCache> lines;
    font->ProcessText(_text, lines, _layoutOptions);

    // Prepare buffers capacity
    _ib.Data.EnsureCapacity(_text.Length() * 6 * sizeof(uint16));
    _vb0.Data.EnsureCapacity(_text.Length() * 4 * sizeof(VB0ElementType));
    _vb1.Data.EnsureCapacity(_text.Length() * 4 * sizeof(VB1ElementType));
    _vb2.Data.EnsureCapacity(_text.Length() * 4 * sizeof(VB2ElementType));
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
        Vector2 pointer = line.Location;

        // Render all characters from the line
        for (int32 charIndex = line.FirstCharIndex; charIndex <= line.LastCharIndex; charIndex++)
        {
            const Char c = _text[charIndex];
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
                    fontAtlas->EnsureTextureCreated();
                    invAtlasSize = 1.0f / fontAtlas->GetSize();

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
                        param->SetValue(fontAtlas);
                        param->SetIsOverride(true);
                    }
                }

                // Get kerning
                const bool isWhitespace = StringUtils::IsWhitespace(c);
                if (!isWhitespace && previous.IsValid)
                {
                    kerning = font->GetKerning(previous.Character, entry.Character);
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

                    Vector2 upperLeftUV = entry.UV * invAtlasSize;
                    Vector2 rightBottomUV = (entry.UV + entry.UVSize) * invAtlasSize;

                    // Calculate bitangent sign
                    Vector3 normal = Vector3::UnitZ;
                    Vector3 tangent = Vector3::UnitX;
                    byte sign = 0;

                    // Write vertices
                    VB0ElementType vb0;
                    VB1ElementType vb1;
                    VB2ElementType vb2;
#define WRITE_VB(pos, uv) \
					vb0.Position = Vector3(-pos, 0.0f); \
					box.Merge(vb0.Position); \
					_vb0.Write(vb0); \
					vb1.TexCoord = Half2(uv); \
					vb1.Normal = Float1010102(normal * 0.5f + 0.5f, 0); \
					vb1.Tangent = Float1010102(tangent * 0.5f + 0.5f, sign); \
					vb1.LightmapUVs = Half2::Zero; \
					_vb1.Write(vb1); \
					vb2.Color = color; \
					_vb2.Write(vb2)
                    //
                    WRITE_VB(charRect.GetBottomRight(), rightBottomUV);
                    WRITE_VB(charRect.GetBottomLeft(), Vector2(upperLeftUV.X, rightBottomUV.Y));
                    WRITE_VB(charRect.GetUpperLeft(), upperLeftUV);
                    WRITE_VB(charRect.GetUpperRight(), Vector2(rightBottomUV.X, upperLeftUV.Y));
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

#if USE_PRECISE_MESH_INTERSECTS
    // Setup collision proxy for detailed collision detection for triangles
    const int32 totalIndicesCount = _ib.Data.Count() / sizeof(uint16);
    _collisionProxy.Init(_vb0.Data.Count() / sizeof(Vector3), totalIndicesCount / 3, (Vector3*)_vb0.Data.Get(), (uint16*)_ib.Data.Get());
#endif

    // Update text bounds (from build vertex positions)
    _localBox = box;
    BoundingBox::Transform(_localBox, _world, _box);
    BoundingSphere::FromBox(_box, _sphere);
}

bool TextRender::HasContentLoaded() const
{
    return (Material == nullptr || Material->IsLoaded()) && (Font == nullptr || Font->IsLoaded());
}

void TextRender::Draw(RenderContext& renderContext)
{
    UpdateLayout();

    GEOMETRY_DRAW_STATE_EVENT_BEGIN(_drawState, _world);

    const DrawPass drawModes = (DrawPass)(DrawModes & renderContext.View.Pass & (int32)renderContext.View.GetShadowsDrawPassMask(ShadowsMode));
    if (_vb0.Data.Count() > 0 && drawModes != DrawPass::None)
    {
#if USE_EDITOR
        // Disable motion blur effects in editor without play mode enabled to hide minor artifacts on objects moving
        if (!Editor::IsPlayMode)
            _drawState.PrevWorld = _world;
#endif

        // Flush buffers
        if (_buffersDirty)
        {
            _buffersDirty = false;
            _ib.Flush();
            _vb0.Flush();
            _vb1.Flush();
            _vb2.Flush();
        }

        // Setup draw call
        DrawCall drawCall;
        drawCall.World = _world;
        drawCall.ObjectPosition = drawCall.World.GetTranslation();
        drawCall.Surface.GeometrySize = _localBox.GetSize();
        drawCall.Surface.PrevWorld = _drawState.PrevWorld;
        drawCall.Surface.Lightmap = nullptr;
        drawCall.Surface.LightmapUVsArea = Rectangle::Empty;
        drawCall.Surface.Skinning = nullptr;
        drawCall.Surface.LODDitherFactor = 0.0f;
        drawCall.WorldDeterminantSign = Math::FloatSelect(_world.RotDeterminant(), 1, -1);
        drawCall.PerInstanceRandom = GetPerInstanceRandom();
        drawCall.Geometry.IndexBuffer = _ib.GetBuffer();
        drawCall.Geometry.VertexBuffers[0] = _vb0.GetBuffer();
        drawCall.Geometry.VertexBuffers[1] = _vb1.GetBuffer();
        drawCall.Geometry.VertexBuffers[2] = _vb2.GetBuffer();
        drawCall.Geometry.VertexBuffersOffsets[0] = 0;
        drawCall.Geometry.VertexBuffersOffsets[1] = 0;
        drawCall.Geometry.VertexBuffersOffsets[2] = 0;
        drawCall.InstanceCount = 1;

        // Submit draw calls
        for (const auto& e : _drawChunks)
        {
            drawCall.Draw.IndicesCount = e.IndicesCount;
            drawCall.Draw.StartIndex = e.StartIndex;
            drawCall.Material = e.Material;
            renderContext.List->AddDrawCall(drawModes, GetStaticFlags(), drawCall, true);
        }
    }

    GEOMETRY_DRAW_STATE_EVENT_END(_drawState, _world);
}

void TextRender::DrawGeneric(RenderContext& renderContext)
{
    Draw(renderContext);
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void TextRender::OnDebugDrawSelected()
{
    // Draw text bounds and layout bounds
    DEBUG_DRAW_WIRE_BOX(_box, Color::Orange, 0, true);
    OrientedBoundingBox layoutBox(Vector3(-_layoutOptions.Bounds.GetUpperLeft(), 0), Vector3(-_layoutOptions.Bounds.GetBottomRight(), 0));
    layoutBox.Transform(_world);
    DEBUG_DRAW_WIRE_BOX(layoutBox, Color::BlueViolet, 0, true);

    // Base
    Actor::OnDebugDrawSelected();
}

#endif

bool TextRender::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
#if USE_PRECISE_MESH_INTERSECTS
    if (_box.Intersects(ray))
    {
        return _collisionProxy.Intersects(ray, _world, distance, normal);
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
    DESERIALIZE_MEMBER(Bounds, _layoutOptions.Bounds);
    DESERIALIZE_MEMBER(HAlignment, _layoutOptions.HorizontalAlignment);
    DESERIALIZE_MEMBER(VAlignment, _layoutOptions.VerticalAlignment);
    DESERIALIZE_MEMBER(Wrapping, _layoutOptions.TextWrapping);
    DESERIALIZE_MEMBER(Scale, _layoutOptions.Scale);
    DESERIALIZE_MEMBER(GapScale, _layoutOptions.BaseLinesGapScale);

    _isDirty = true;
}

void TextRender::OnEnable()
{
    GetSceneRendering()->AddGeometry(this);

    // Base
    Actor::OnEnable();

    UpdateLayout();
}

void TextRender::OnDisable()
{
    GetSceneRendering()->RemoveGeometry(this);

    // Base
    Actor::OnDisable();
}

void TextRender::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _transform.GetWorld(_world);
    BoundingBox::Transform(_localBox, _world, _box);
    BoundingSphere::FromBox(_box, _sphere);
}
