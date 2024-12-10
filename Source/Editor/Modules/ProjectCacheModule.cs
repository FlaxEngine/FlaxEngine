// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Runtime.CompilerServices;
using FlaxEngine;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Caching local project data manager. Used to store and manage the information about expanded actor nodes in the scene tree and other local user data used by the editor. Stores data in the project cache directory.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class ProjectCacheModule : EditorModule
    {
        private readonly string _cachePath;
        private bool _isDirty;
        private DateTime _lastSaveTime;

        private readonly HashSet<Guid> _expandedActors = new HashSet<Guid>();
        private readonly HashSet<string> _toggledGroups = new HashSet<string>();
        private readonly Dictionary<string, string> _customData = new Dictionary<string, string>();

        /// <summary>
        /// Gets or sets the automatic data save interval.
        /// </summary>
        public TimeSpan AutoSaveInterval { get; set; } = TimeSpan.FromSeconds(3);

        /// <inheritdoc />
        internal ProjectCacheModule(Editor editor)
        : base(editor)
        {
            // After editor options but before the others
            InitOrder = -900;

            _cachePath = StringUtils.CombinePaths(Globals.ProjectCacheFolder, "ProjectCache.dat");
            _isDirty = true;
        }

        /// <summary>
        /// Determines whether actor identified by the given ID is expanded in the scene tree UI.
        /// </summary>
        /// <param name="id">The actor identifier.</param>
        /// <returns><c>true</c> if actor is expanded; otherwise, <c>false</c>.</returns>
        public bool IsExpandedActor(ref Guid id)
        {
            return _expandedActors.Contains(id);
        }

        /// <summary>
        /// Sets the actor expanded cached value.
        /// </summary>
        /// <param name="id">The identifier.</param>
        /// <param name="isExpanded">If set to <c>true</c> actor will be cached as an expanded, otherwise false.</param>
        public void SetExpandedActor(ref Guid id, bool isExpanded)
        {
            if (isExpanded)
                _expandedActors.Add(id);
            else
                _expandedActors.Remove(id);
            _isDirty = true;
        }

        /// <summary>
        /// Determines whether group identified by the given title is collapsed/opened in the UI.
        /// </summary>
        /// <param name="title">The group title.</param>
        /// <returns><c>true</c> if group is toggled; otherwise, <c>false</c>.</returns>
        public bool IsGroupToggled(string title)
        {
            return _toggledGroups.Contains(title);
        }

        /// <summary>
        /// Sets the group collapsed/opened cached value.
        /// </summary>
        /// <param name="title">The group title.</param>
        /// <param name="isToggled">If set to <c>true</c> group will be cached as a toggled, otherwise false.</param>
        public void SetGroupToggle(string title, bool isToggled)
        {
            if (isToggled)
                _toggledGroups.Add(title);
            else
                _toggledGroups.Remove(title);
            _isDirty = true;
        }

        /// <summary>
        /// Determines whether project cache contains custom data of the specified key.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <returns><c>true</c> if has custom data of the specified key; otherwise, <c>false</c>.</returns>
        public bool HasCustomData(string key)
        {
            return _customData.ContainsKey(key);
        }

        /// <summary>
        /// Gets the custom data by the key.
        /// </summary>
        /// <remarks>
        /// Use <see cref="HasCustomData"/> to check if a key is valid.
        /// </remarks>
        /// <param name="key">The key.</param>
        /// <returns>The custom data.</returns>
        public string GetCustomData(string key)
        {
            return _customData[key];
        }

        /// <summary>
        /// Tries to get the custom data by the key.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="value">When this method returns, contains the value associated with the specified key, if the key is found; otherwise, the default value for the type of the <paramref name="value" /> parameter. This parameter is passed uninitialized.</param>
        /// <returns>The custom data.</returns>
        public bool TryGetCustomData(string key, out string value)
        {
            return _customData.TryGetValue(key, out value);
        }

        /// <summary>
        /// Tries to get the custom data by the key.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="value">When this method returns, contains the value associated with the specified key, if the key is found; otherwise, the default value for the type of the <paramref name="value" /> parameter. This parameter is passed uninitialized.</param>
        /// <returns>The custom data.</returns>
        public bool TryGetCustomData(string key, out bool value)
        {
            value = false;
            return _customData.TryGetValue(key, out var valueStr) && bool.TryParse(valueStr, out value);
        }

        /// <summary>
        /// Tries to get the custom data by the key.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="value">When this method returns, contains the value associated with the specified key, if the key is found; otherwise, the default value for the type of the <paramref name="value" /> parameter. This parameter is passed uninitialized.</param>
        /// <returns>The custom data.</returns>
        public bool TryGetCustomData(string key, out float value)
        {
            value = 0.0f;
            return _customData.TryGetValue(key, out var valueStr) && float.TryParse(valueStr, out value);
        }

        /// <summary>
        /// Sets the custom data.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="value">The value.</param>
        public void SetCustomData(string key, string value)
        {
            _customData[key] = value;
            _isDirty = true;
        }

        /// <summary>
        /// Sets the custom data.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="value">The value.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetCustomData(string key, bool value)
        {
            SetCustomData(key, value.ToString());
        }

        /// <summary>
        /// Sets the custom data.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="value">The value.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetCustomData(string key, float value)
        {
            SetCustomData(key, value.ToString(CultureInfo.InvariantCulture));
        }

        /// <summary>
        /// Removes the custom data.
        /// </summary>
        /// <param name="key">The key.</param>
        public void RemoveCustomData(string key)
        {
            bool removed = _customData.Remove(key);
            _isDirty |= removed;
        }

        private void LoadGuarded()
        {
            using (var stream = new FileStream(_cachePath, FileMode.Open, FileAccess.Read, FileShare.Read))
            using (var reader = new BinaryReader(stream))
            {
                var version = reader.ReadInt32();

                switch (version)
                {
                case 1:
                {
                    int expandedActorsCount = reader.ReadInt32();
                    _expandedActors.Clear();
                    var bytes16 = new byte[16];
                    for (int i = 0; i < expandedActorsCount; i++)
                    {
                        reader.Read(bytes16, 0, 16);
                        _expandedActors.Add(new Guid(bytes16));
                    }

                    _toggledGroups.Clear();
                    _customData.Clear();

                    break;
                }
                case 2:
                {
                    int expandedActorsCount = reader.ReadInt32();
                    _expandedActors.Clear();
                    var bytes16 = new byte[16];
                    for (int i = 0; i < expandedActorsCount; i++)
                    {
                        reader.Read(bytes16, 0, 16);
                        _expandedActors.Add(new Guid(bytes16));
                    }

                    _toggledGroups.Clear();

                    _customData.Clear();
                    int customDataCount = reader.ReadInt32();
                    for (int i = 0; i < customDataCount; i++)
                    {
                        var key = reader.ReadString();
                        var value = reader.ReadString();
                        _customData.Add(key, value);
                    }

                    break;
                }
                case 3:
                {
                    int expandedActorsCount = reader.ReadInt32();
                    _expandedActors.Clear();
                    var bytes16 = new byte[16];
                    for (int i = 0; i < expandedActorsCount; i++)
                    {
                        reader.Read(bytes16, 0, 16);
                        _expandedActors.Add(new Guid(bytes16));
                    }

                    int collapsedGroupsCount = reader.ReadInt32();
                    _toggledGroups.Clear();
                    for (int i = 0; i < collapsedGroupsCount; i++)
                        _toggledGroups.Add(reader.ReadString());

                    _customData.Clear();
                    int customDataCount = reader.ReadInt32();
                    for (int i = 0; i < customDataCount; i++)
                    {
                        var key = reader.ReadString();
                        var value = reader.ReadString();
                        _customData.Add(key, value);
                    }

                    break;
                }
                default:
                    Editor.LogWarning("Unknown editor cache version.");
                    return;
                }
            }
        }

        private void Load()
        {
            if (!File.Exists(_cachePath))
                return;
            _lastSaveTime = DateTime.UtcNow;

            try
            {
                LoadGuarded();
            }
            catch (Exception ex)
            {
                Editor.LogWarning(ex);
                Editor.LogError("Failed to load editor cache. " + ex.Message);
            }
        }

        private void SaveGuarded()
        {
            using (var stream = new FileStream(_cachePath, FileMode.Create, FileAccess.Write, FileShare.Read))
            using (var writer = new BinaryWriter(stream))
            {
                writer.Write(3);

                writer.Write(_expandedActors.Count);
                foreach (var e in _expandedActors)
                {
                    writer.Write(e.ToByteArray());
                }

                writer.Write(_toggledGroups.Count);
                foreach (var e in _toggledGroups)
                    writer.Write(e);

                writer.Write(_customData.Count);
                foreach (var e in _customData)
                {
                    writer.Write(e.Key);
                    writer.Write(e.Value);
                }
            }
        }

        private void Save()
        {
            if (!_isDirty)
                return;

            _lastSaveTime = DateTime.UtcNow;

            try
            {
                SaveGuarded();
                _isDirty = false;
            }
            catch (Exception ex)
            {
                Editor.LogWarning(ex);
                Editor.LogError("Failed to save editor cache. " + ex.Message);
            }
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            Load();
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            var dt = DateTime.UtcNow - _lastSaveTime;
            if (dt >= AutoSaveInterval)
            {
                Save();
            }
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            Save();
        }
    }
}
