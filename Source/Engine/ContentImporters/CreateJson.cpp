// Copyright (c) Wojciech Figat. All rights reserved.

#include "CreateJson.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Storage/JsonStorageProxy.h"
#include "Engine/Content/Cache/AssetsCache.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Localization/LocalizedStringTable.h"
#include "Engine/Utilities/TextProcessing.h"
#include "FlaxEngine.Gen.h"

bool CreateJson::Create(const StringView& path, rapidjson_flax::StringBuffer& data, const String& dataTypename)
{
    auto str = dataTypename.ToStringAnsi();
    return Create(path, data, str.Get());
}

bool CreateJson::Create(const StringView& path, rapidjson_flax::StringBuffer& data, const char* dataTypename)
{
    StringAnsiView data1((char*)data.GetString(), (int32)data.GetSize());
    StringAnsiView data2((char*)dataTypename, StringUtils::Length(dataTypename));
    return Create(path, data1, data2);
}

bool CreateJson::Create(const StringView& path, const StringAnsiView& data, const StringAnsiView& dataTypename)
{
    Guid id = Guid::New();

    LOG(Info, "Creating json resource of type \'{1}\' at \'{0}\'", path, String(dataTypename.Get()));

    // Try use the same asset ID
    if (FileSystem::FileExists(path))
    {
        String typeName;
        JsonStorageProxy::GetAssetInfo(path, id, typeName);
        if (typeName != String(dataTypename.Get(), dataTypename.Length()))
        {
            LOG(Warning, "Asset will have different type name {0} -> {1}", typeName, String(dataTypename.Get()));
        }
    }
    else
    {
        const String directory = StringUtils::GetDirectoryName(path);
        if (!FileSystem::DirectoryExists(directory))
        {
            if (FileSystem::CreateDirectory(directory))
            {
                LOG(Warning, "Failed to create directory '{}'", directory);
                return true;
            }
        }
    }

    rapidjson_flax::StringBuffer buffer;

    // Serialize to json
    PrettyJsonWriter writerObj(buffer);
    JsonWriter& writer = writerObj;
    writer.StartObject();
    {
        // Json resource header
        writer.JKEY("ID");
        writer.Guid(id);
        writer.JKEY("TypeName");
        writer.String(dataTypename.Get(), dataTypename.Length());
        writer.JKEY("EngineBuild");
        writer.Int(FLAXENGINE_VERSION_BUILD);

        // Json resource data
        writer.JKEY("Data");
        writer.RawValue(data.Get(), data.Length());
    }
    writer.EndObject();

    // Save json to file
    if (File::WriteAllBytes(path, (byte*)buffer.GetString(), (int32)buffer.GetSize()))
    {
        LOG(Warning, "Failed to save json to file");
        return true;
    }

    // Reload asset at the target location if is loaded
    auto asset = Content::GetAsset(path);
    if (asset)
    {
        asset->Reload();
    }
    else
    {
        Content::GetRegistry()->RegisterAsset(id, String(dataTypename), path);
    }

    return false;
}

void FormatPoValue(String& value)
{
    value.Replace(TEXT("\\n"), TEXT("\n"));
    value.Replace(TEXT("%s"), TEXT("{}"));
    value.Replace(TEXT("%d"), TEXT("{}"));
}

CreateAssetResult CreateJson::ImportPo(CreateAssetContext& context)
{
    // Base
    IMPORT_SETUP(LocalizedStringTable, 1);

    // Load file (UTF-16)
    String inputData;
    if (File::ReadAllText(context.InputPath, inputData))
    {
        return CreateAssetResult::InvalidPath;
    }

    // Use virtual asset for data storage and serialization
    AssetReference<LocalizedStringTable> asset = Content::CreateVirtualAsset<LocalizedStringTable>();
    if (!asset)
        return CreateAssetResult::Error;

    // Parse PO format
    int32 pos = 0;
    int32 pluralCount = 0;
    int32 lineNumber = 0;
    bool fuzzy = false, hasNewContext = false;
    StringView msgctxt, msgid;
    String idTmp;
    while (pos < inputData.Length())
    {
        // Read line
        const int32 startPos = pos;
        while (pos < inputData.Length() && inputData[pos] != '\n')
            pos++;
        const StringView line(&inputData[startPos], pos - startPos);
        lineNumber++;
        pos++;
        const int32 valueStart = line.Find('\"') + 1;
        const int32 valueEnd = line.FindLast('\"');
        const StringView value(line.Get() + valueStart, Math::Max(valueEnd - valueStart, 0));

        if (line.StartsWith(StringView(TEXT("msgid_plural"))))
        {
            // Plural form
        }
        else if (line.StartsWith(StringView(TEXT("msgid"))))
        {
            // Id
            msgid = value;

            // Reset context if already used
            if (!hasNewContext)
                msgctxt = StringView();
            hasNewContext = false;
        }
        else if (line.StartsWith(StringView(TEXT("msgstr"))))
        {
            // String
            if (msgid.HasChars())
            {
                // Format message
                String msgstr(value);
                FormatPoValue(msgstr);

                // Get message id
                StringView id = msgid;
                if (msgctxt.HasChars())
                {
                    idTmp = String(msgctxt) + TEXT(".") + String(msgid);
                    id = idTmp;
                }

                int32 indexStart = line.Find('[');
                if (indexStart != -1 && indexStart < valueStart)
                {
                    indexStart++;
                    while (indexStart < line.Length() && StringUtils::IsWhitespace(line[indexStart]))
                        indexStart++;
                    int32 indexEnd = line.Find(']');
                    while (indexEnd > indexStart && StringUtils::IsWhitespace(line[indexEnd - 1]))
                        indexEnd--;
                    int32 index = -1;
                    StringUtils::Parse(line.Get() + indexStart, (uint32)(indexEnd - indexStart), &index);
                    if (pluralCount <= 0)
                    {
                        LOG(Error, "Missing 'nplurals'. Cannot use plural message at line {0}", lineNumber);
                        return CreateAssetResult::Error;
                    }
                    if (index < 0 || index > pluralCount)
                    {
                        LOG(Error, "Invalid plural message index at line {0}", lineNumber);
                        return CreateAssetResult::Error;
                    }

                    // Plural message
                    asset->AddPluralString(id, msgstr, index);
                }
                else
                {
                    // Message
                    asset->AddString(id, msgstr);
                }
            }
        }
        else if (line.StartsWith(StringView(TEXT("msgctxt"))))
        {
            // Context
            msgctxt = value;
            hasNewContext = true;
        }
        else if (line.StartsWith('\"'))
        {
            // Config
            const Char* pluralForms = StringUtils::Find(line.Get(), TEXT("Plural-Forms"));
            if (pluralForms != nullptr && pluralForms < line.Get() + line.Length() - 1)
            {
                // Process plural forms rule
                const Char* nplurals = StringUtils::Find(pluralForms, TEXT("nplurals"));
                if (nplurals && nplurals < line.Get() + line.Length())
                {
                    while (*nplurals && *nplurals != '=')
                        nplurals++;
                    while (*nplurals && (StringUtils::IsWhitespace(*nplurals) || *nplurals == '='))
                        nplurals++;
                    const Char* npluralsStart = nplurals;
                    while (*nplurals && !StringUtils::IsWhitespace(*nplurals) && *nplurals != ';')
                        nplurals++;
                    StringUtils::Parse(npluralsStart, (uint32)(nplurals - npluralsStart), &pluralCount);
                    if (pluralCount < 0 || pluralCount > 100)
                    {
                        LOG(Error, "Invalid 'nplurals' at line {0}", lineNumber);
                        return CreateAssetResult::Error;
                    }
                }
                // TODO: parse plural forms rule
            }
            const Char* language = StringUtils::Find(line.Get(), TEXT("Language"));
            if (language != nullptr && language < line.Get() + line.Length() - 1)
            {
                // Process language locale
                while (*language && *language != ':')
                    language++;
                language++;
                while (*language && StringUtils::IsWhitespace(*language))
                    language++;
                const Char* languageStart = language;
                while (*language && !StringUtils::IsWhitespace(*language) && *language != '\\' && *language != '\"')
                    language++;
                asset->Locale.Set(languageStart, (int32)(language - languageStart));
                if (asset->Locale == TEXT("English"))
                    asset->Locale = TEXT("en");
                if (asset->Locale.Length() > 5)
                    LOG(Warning, "Imported .po file uses invalid locale '{0}'", asset->Locale);
            }
        }
        else if (line.StartsWith('#') || line.IsEmpty())
        {
            // Comment
            const Char* fuzzyPos = StringUtils::Find(line.Get(), TEXT("fuzzy"));
            fuzzy |= fuzzyPos != nullptr && fuzzyPos < line.Get() + line.Length() - 1;
        }
    }
    if (asset->Locale.IsEmpty())
        LOG(Warning, "Imported .po file has missing locale");

    // Save asset
    return asset->Save(context.TargetAssetPath) ? CreateAssetResult::CannotSaveFile : CreateAssetResult::Ok;
}

#endif
