// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using Newtonsoft.Json;

namespace FlaxEditor.LocalizationServices
{
    /// <summary>
    /// The layout of a localization file
    /// </summary>
    public struct LocalizationAsset
    {
        /// <summary>
        /// A collection of all existing namespaces
        /// </summary>
        [JsonProperty(nameof(Namespaces))]
        public LocalizationNamespace[] Namespaces { get; set; }
    }

    /// <summary>
    /// A name space with localization entries
    /// </summary>
    public struct LocalizationNamespace
    {
        /// <summary>
        /// The name of the namespace
        /// </summary>
        [JsonProperty(nameof(Namespace))]
        public string Namespace { get; set; }

        /// <summary>
        /// The localization entries of the given namespace
        /// </summary>
        [JsonProperty(nameof(Entries))]
        public LocalizationEntry[] Entries { get; set; }
    }

    /// <summary>
    /// An entry that holds a key which associates a given string
    /// </summary>
    public struct LocalizationEntry
    {
        [JsonProperty(nameof(Key))]
        public string Key { get; set; }

        [JsonProperty(nameof(Value))]
        public string Value { get; set; }
    }
}
