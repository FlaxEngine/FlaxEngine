// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.Assertions;
using Newtonsoft.Json;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System;

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// All supported languages the editor provides
    /// </summary>
    public enum Languages
    {
        /// <summary>
        /// The English language
        /// </summary>
        English,

        /// <summary>
        /// The Polish language
        /// </summary>
        Polish, //TODO: Add support

        /// <summary>
        /// The German language
        /// </summary>
        German, //TODO: Add support

        /// <summary>
        /// The French language
        /// </summary>
        French  //TODO: Add support
    }
    
    /// <summary>
    /// Interface for a localization service
    /// </summary>
    public interface ILocalizationManager
    {
        /// <summary>
        /// Selects and loads the given language.
        /// </summary>
        /// <param name="language">The Language.</param>
        void SelectLanguage(Languages language);

        /// <summary>
        /// Get the localized <see cref="string"/> from a given key for the current language.
        /// </summary>
        /// <param name="key">They key to identify the <see cref="string"/>.</param>
        /// <returns>A localized string.</returns>
        string GetValue(string key);

        /// <summary>
        /// Set the localized <see cref="string"/> to a given key for the current language.
        /// If key is not valid, a new key with the value will be created instead.
        /// </summary>
        /// <param name="key">The key to identify the <see cref="string"/>.</param>
        /// <param name="value">They value associated with this key.</param>
        /// <returns>Whether the value for the key was updated/created.</returns>
        bool SetValue(string key, string value);

        /// <summary>
        /// Load localization from file path.
        /// </summary>
        /// <param name="directory">The file path.</param>
        /// <param name="language">The localization language.</param>
        void Load(string directory, Languages language);

        /// <summary>
        /// The selected localization language
        /// </summary>
        Languages SelectedLanguage { get; set; }

    }

    /// <summary>
    /// Provides localization services to the editor
    /// </summary>
    public class Localization : ILocalizationManager
    {
        /// <inheritdoc />
        public void SelectLanguage(Languages language)
        {
            m_SelectedLanguage = language;

            // Avoid picking the same the language
            if (m_Languages.TryGetValue(language, out _))
                return;

            Load(Path.Combine(Globals.BinariesFolder, "Localizations"), language);
        }

        /// <inheritdoc />
        public string GetValue(string key)
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
        public void Load(string directory, Languages language)
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
                        this.SetValue(entry.Key, entry.Value);
                }
                catch
                {
                    Assert.IsFalse(true, $"Couldn't read language file `{language}` ({path})");
                }
            }

        }

        /// <inheritdoc />
        public bool SetValue(string key, string value)
        {
            if (string.IsNullOrEmpty(key))
                throw new ArgumentException($"'{nameof(key)}' cannot be null or empty", nameof(key));

            if (string.IsNullOrEmpty(value))
                throw new ArgumentException($"'{nameof(value)}' cannot be null or empty", nameof(value));
            
            // Get the language
            if (m_Languages.TryGetValue(m_SelectedLanguage, out var map))
            {
                if(map.ContainsKey(key))
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
        public Languages SelectedLanguage
        {
            get => m_SelectedLanguage;

            set => this.SelectLanguage(value);
        }

        /// <summary>
        /// Singleton instance of the localization services
        /// </summary>
        public static Localization Manager
        {
            get => _instance;
        }

        private struct LocalizationEntry
        {
            public string Key { get; set; }

            public string Value { get; set; }
        }

        // Singleton
        private static readonly Localization _instance = new Localization();
        private Dictionary<Languages, Dictionary<string, string>> m_Languages = new Dictionary<Languages, Dictionary<string, string>>();
        private Languages m_SelectedLanguage;
    }
}
