// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Engine/Globals.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    // A 1x1 RGB PNG with the pixel (128, 128, 128)
    const byte PlainPng[] =
    {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53,
        0xDE, 0x00, 0x00, 0x00, 0x0F, 0x49, 0x44, 0x41, 0x54, 0x78, 0x01, 0x01, 0x04, 0x00, 0xFB, 0xFF,
        0x00, 0x80, 0x80, 0x80, 0x03, 0x04, 0x01, 0x81, 0xA4, 0x1F, 0x90, 0x57, 0x00, 0x00, 0x00, 0x00,
        0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
    };

    // The sRGB chunk (rendering intent: perceptual), placed after the IHDR chunk that ends at this offset
    const byte SrgbChunk[] = { 0x00, 0x00, 0x00, 0x01, 0x73, 0x52, 0x47, 0x42, 0x00, 0xAE, 0xCE, 0x1C, 0xE9 };
    constexpr int32 IhdrEnd = 33;

    // Restores the color space setting also when a check stops the test
    struct GammaColorSpaceRestore
    {
        const bool Value = GraphicsSettings::Get()->GammaColorSpace;

        ~GammaColorSpaceRestore()
        {
            GraphicsSettings::Get()->GammaColorSpace = Value;
        }
    };

    // Imports the PNG as an uncompressed sRGB color texture and returns the red value of its pixel
    int32 ImportRed(const Array<byte>& png, bool gammaColorSpace)
    {
        GraphicsSettings::Get()->GammaColorSpace = gammaColorSpace;
        const String path = Globals::TemporaryFolder / TEXT("TestTextureTool.png");
        REQUIRE(!File::WriteAllBytes(path, png));
        TextureTool::Options options;
        options.Type = TextureFormatType::ColorRGB;
        options.Compress = false;
        options.sRGB = true;
        options.GenerateMipMaps = false;
        TextureData data;
        String errorMsg;
        const bool failed = TextureTool::ImportTexture(path, data, options, errorMsg);
        FileSystem::DeleteFile(path);
        REQUIRE(!failed);
        REQUIRE(data.Items.Count() == 1);
        return data.GetData(0, 0)->Get<Color32>(0, 0).R;
    }
}

TEST_CASE("TextureTool")
{
    SECTION("Test sRGB chunk in PNG")
    {
        // A PNG that declares sRGB (sRGB chunk) has to import as the same PNG without the chunk
        Array<byte> plain;
        plain.Add(PlainPng, ARRAY_COUNT(PlainPng));
        Array<byte> tagged;
        tagged.Add(PlainPng, IhdrEnd);
        tagged.Add(SrgbChunk, ARRAY_COUNT(SrgbChunk));
        tagged.Add(PlainPng + IhdrEnd, ARRAY_COUNT(PlainPng) - IhdrEnd);

        GammaColorSpaceRestore restore;
        const int32 plainLinear = ImportRed(plain, false);
        const int32 taggedLinear = ImportRed(tagged, false);
        const int32 plainGamma = ImportRed(plain, true);
        const int32 taggedGamma = ImportRed(tagged, true);

        CAPTURE(plainLinear, taggedLinear, plainGamma, taggedGamma);
        CHECK(plainLinear == 128);
        CHECK(taggedLinear == plainLinear);
        CHECK(taggedGamma == plainGamma);
    }
}
