// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace FlaxEditor.Localization
{
    /// <summary>
    /// Interface for a localization service
    /// </summary>
    public interface ILocalizationManager
    {
        /// <summary>
        /// Selects and loads the given language.
        /// </summary>
        /// <param name="language">The Language.</param>
        void SelectLanguage(LanguageFormats language);

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
        void Load(string directory, LanguageFormats language);

        /// <summary>
        /// The selected localization language
        /// </summary>
        LanguageFormats SelectedLanguage { get; set; }

    }
}
