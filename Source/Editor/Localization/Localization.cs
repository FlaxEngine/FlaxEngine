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
        /// <inheritdoc />
        public static void SelectLanguage(Languages language)
        {
            m_SelectedLanguage = language;

            // Avoid picking the same the language
            if (m_Languages.TryGetValue(language, out _))
                return;

            Load(Path.Combine(Globals.BinariesFolder, "Localizations"), language);
        }

        /// <inheritdoc />
        public static string GetValue(string key)
        {
            if (string.IsNullOrEmpty(key))
                throw new ArgumentException($"'{nameof(key)}' cannot be null or empty", nameof(key));

            // If key has value, use it.
            if (m_Languages.TryGetValue(m_SelectedLanguage, out var map) && map.TryGetValue(key, out var value))
                return value;

            // Use as fallback if no value.
            Editor.LogWarning($"Specified key `{key}` was not found, using key as fallback value.");
            return key;
        }

        /// <inheritdoc />
        public static void Load(string directory, Languages language)
        {
            string path = Path.Combine(directory, language.ToString() + ".json");

            if (!File.Exists(path))
            {
                Assert.IsTrue(false, $"Language file `{language}` was not found");
                return;
            }

            var json = JsonSerializer.CreateDefault(new JsonSerializerSettings()
            {
                Culture = CultureInfo.InvariantCulture
            });

            m_Languages.Add(language, new Dictionary<string, string>());

            using (JsonReader reader = new JsonTextReader(new StreamReader(new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite), true)))
            {
                try
                {
                    var entries = json.Deserialize<LocalizationEntry[]>(reader);

                    // Add the values to that language
                    foreach (var entry in entries)
                        SetValue(entry.Key, entry.Value);
                }
                catch
                {
                    Assert.IsFalse(true, $"Couldn't read language file `{language}` ({path})");
                }
            }

        }

        /// <inheritdoc />
        public static bool SetValue(string key, string value)
        {
            if (string.IsNullOrEmpty(key))
                throw new ArgumentException($"'{nameof(key)}' cannot be null or empty", nameof(key));

            if (string.IsNullOrEmpty(value))
                throw new ArgumentException($"'{nameof(value)}' cannot be null or empty", nameof(value));

            // Get the language
            if (m_Languages.TryGetValue(m_SelectedLanguage, out var map))
            {
                if (map.ContainsKey(key))
                {
                    // Update
                    map[key] = value;
                    return true;
                }
                else
                {
                    // Add
                    map.Add(key, value);
                    return true;
                }
            }

            return false;
        }

        /// <inheritdoc />
        public static Languages SelectedLanguage
        {
            get => m_SelectedLanguage;

            set => SelectLanguage(value);
        }

        /// <summary>
        /// Localization entry for the JSON format
        /// </summary>
        private struct LocalizationEntry
        {
            [JsonProperty(nameof(Key))]
            public string Key { get; set; }

            [JsonProperty(nameof(Value))]
            public string Value { get; set; }
        }

        private static Dictionary<Languages, Dictionary<string, string>> m_Languages = new Dictionary<Languages, Dictionary<string, string>>();
        private static Languages m_SelectedLanguage;
    }
}
