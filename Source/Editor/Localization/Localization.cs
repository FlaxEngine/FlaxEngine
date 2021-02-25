// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.Assertions;
using Newtonsoft.Json;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System;

namespace FlaxEditor.LocalizationServices
{
    /// <summary>
    /// Provides localization services to the editor
    /// </summary>
    public static class Localization
    {
        // Key - Language
        // Value - Namespace, (Key, Value)
        private static Dictionary<Languages, Dictionary<string, Dictionary<string, string>>> m_Languages = new Dictionary<Languages, Dictionary<string, Dictionary<string, string>>>();

        private static Languages m_SelectedLanguage;

        /// <summary>
        /// Selects and loads the given language.
        /// </summary>
        /// <param name="language">The Language.</param>
        public static void SelectLanguage(Languages language)
        {
            m_SelectedLanguage = language;

            // Avoid picking the same the language
            if (m_Languages.TryGetValue(language, out _))
                return;

            string path = Path.Combine(Globals.BinariesFolder, "Localization", language.ToString() + ".json");

            if (!File.Exists(path))
            {
                Assert.IsTrue(false, $"Language file `{language}` was not found");
                return;
            }

            var json = JsonSerializer.CreateDefault(new JsonSerializerSettings()
            {
                Culture = CultureInfo.InvariantCulture
            });

            m_Languages.Add(language, new Dictionary<string, Dictionary<string, string>>());

            using (JsonReader reader = new JsonTextReader(new StreamReader(new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite), true)))
            {
                try
                {
                    var asset = json.Deserialize<LocalizationAsset>(reader);

                    // Add the values to that language
                    for (int i = 0; i < asset.Namespaces.Length; i++)
                    {
                        var _namespace = asset.Namespaces[i];

                        for (int j = 0; j < _namespace.Entries.Length; j++)
                        {
                            var _entry = _namespace.Entries[j];

                            SetValue(_namespace.Namespace, _entry.Key, _entry.Value);
                        }
                    }
                }
                catch (Exception e)
                {
                    Assert.IsTrue(false, $"Couldn't read language file `{language}` ({path}) \n" + e);
                }
            }
            Editor.Log("Finished reading localization asset...");
        }

        /// <summary>
        /// Get the localized <see cref="string"/> from a given key for the current language.
        /// </summary>
        /// <param name="inNamespace">The namespace from which to get the key</param>
        /// <param name="key">The key to identify the <see cref="string"/>.</param>
        /// <returns>A localized string.</returns>
        public static string GetValue(string inNamespace, string key)
        {
            if (string.IsNullOrEmpty(inNamespace))
                throw new ArgumentException($"'{nameof(inNamespace)}' cannot be null or empty", nameof(inNamespace));

            if (string.IsNullOrEmpty(key))
                throw new ArgumentException($"'{nameof(key)}' cannot be null or empty", nameof(key));

            // Get language, get namespace then finally get the key's value
            if (m_Languages.TryGetValue(m_SelectedLanguage, out var map))
            {
                if (map.TryGetValue(inNamespace, out var namespaceMap) && namespaceMap.TryGetValue(key, out string value))
                {
                    return value;
                }
            }

            // Use fallback if no value.
            Editor.LogWarning($"Specified key `{key}` or namespace `{inNamespace}` was not found.");
            return "INVALID_ID";
        }

        /// <summary>
        /// Set the localized <see cref="string"/> to a given key for the current language.
        /// If key is not valid, a new key with the value will be created instead.
        /// </summary>
        /// <param name="inNamespace">The name space</param>
        /// <param name="key">The key to identify the <see cref="string"/>.</param>
        /// <param name="value">They value associated with this key.</param>
        /// <returns>Whether the value for the key was updated/created.</returns>
        public static bool SetValue(string inNamespace, string key, string value)
        {
            if (string.IsNullOrEmpty(inNamespace))
                throw new ArgumentException($"'{nameof(inNamespace)}' cannot be null or empty", nameof(inNamespace));

            if (string.IsNullOrEmpty(key))
                throw new ArgumentException($"'{nameof(key)}' cannot be null or empty", nameof(key));

            if (string.IsNullOrEmpty(value))
                throw new ArgumentException($"'{nameof(value)}' cannot be null or empty", nameof(value));

            // Get the language
            if (m_Languages.TryGetValue(m_SelectedLanguage, out var map))
            {
                // Update key if namespace does exist
                if (map.TryGetValue(inNamespace, out var namespaceMap))
                {
                    if (namespaceMap.ContainsKey(key))
                        namespaceMap[key] = value;
                    else
                        namespaceMap.Add(key, value);

                    return true;
                }

                // Add namespace
                map.Add(inNamespace, new Dictionary<string, string> { { key, value } });
                return true;
            }

            return false;
        }

        /// <summary>
        /// The selected localization language
        /// </summary>
        public static Languages SelectedLanguage
        {
            get => m_SelectedLanguage;

            set => SelectLanguage(value);
        }
    }
}
