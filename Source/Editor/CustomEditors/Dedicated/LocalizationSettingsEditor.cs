// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;
using Object = FlaxEngine.Object;

namespace FlaxEditor.CustomEditors.Dedicated
{
    [CustomEditor(typeof(LocalizationSettings))]
    sealed class LocalizationSettingsEditor : GenericEditor
    {
        private CultureInfo _theMostTranslatedCulture;
        private int _theMostTranslatedCultureCount;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var settings = (LocalizationSettings)Values[0];
            var tablesLength = settings.LocalizedStringTables?.Length ?? 0;
            var tables = new List<LocalizedStringTable>(tablesLength);
            for (int i = 0; i < tablesLength; i++)
            {
                var table = settings.LocalizedStringTables[i];
                if (table && !table.WaitForLoaded())
                    tables.Add(table);
            }
            var locales = tables.GroupBy(x => x.Locale);
            var tableEntries = new Dictionary<LocalizedStringTable, Dictionary<string, string[]>>();
            var allKeys = new HashSet<string>();
            foreach (var e in locales)
            {
                foreach (var table in e)
                {
                    var entries = table.Entries;
                    tableEntries[table] = entries;
                    allKeys.AddRange(entries.Keys);
                }
            }

            {
                var group = layout.Group("Preview");

                // Current language and culture preview management
                group.Object("Current Language", new CustomValueContainer(new ScriptType(typeof(CultureInfo)), Localization.CurrentLanguage, (instance, index) => Localization.CurrentLanguage, (instance, index, value) => Localization.CurrentLanguage = value as CultureInfo), null, "Current UI display language for the game preview.");
                group.Object("Current Culture", new CustomValueContainer(new ScriptType(typeof(CultureInfo)), Localization.CurrentCulture, (instance, index) => Localization.CurrentCulture, (instance, index, value) => Localization.CurrentCulture = value as CultureInfo), null, "Current values formatting culture for the game preview.");
            }

            {
                var group = layout.Group("Locales");

                // Show all existing locales
                _theMostTranslatedCulture = null;
                _theMostTranslatedCultureCount = -1;
                foreach (var e in locales)
                {
                    var culture = new CultureInfo(e.Key);
                    var prop = group.AddPropertyItem(CultureInfoEditor.GetName(culture), culture.NativeName);
                    int count = e.Sum(x => tableEntries[x].Count);
                    int validCount = e.Sum(x => tableEntries[x].Values.Count(y => y != null && y.Length != 0 && !string.IsNullOrEmpty(y[0])));
                    if (count > _theMostTranslatedCultureCount)
                    {
                        _theMostTranslatedCulture = culture;
                        _theMostTranslatedCultureCount = count;
                    }
                    prop.Label(string.Format("Progress: {0}% ({1}/{2})", (int)(((float)validCount / allKeys.Count * 100.0f)), validCount, allKeys.Count));
                    prop.Label("Tables:");
                    foreach (var table in e)
                    {
                        var namePath = table.Path;
                        if (namePath.StartsWith(Globals.ProjectFolder))
                            namePath = namePath.Substring(Globals.ProjectFolder.Length + 1);
                        var tableLabel = prop.ClickableLabel(namePath).CustomControl;
                        tableLabel.TextColorHighlighted = Color.Wheat;
                        tableLabel.DoubleClick += delegate { Editor.Instance.Windows.ContentWin.Select(table); };
                    }
                    group.Space(10);
                }

                // New locale add button
                var addLocale = group.Button("Add Locale..").Button;
                addLocale.Height = 16.0f;
                addLocale.ButtonClicked += delegate(Button button)
                {
                    var menu = CultureInfoEditor.CreatePicker(null, culture =>
                    {
                        var displayName = CultureInfoEditor.GetName(culture);
                        if (locales.Any(x => x.Key == culture.Name))
                        {
                            MessageBox.Show($"Culture '{displayName}' is already added.");
                            return;
                        }
                        Editor.Log($"Adding culture '{displayName}' to localization settings");
                        var newTables = settings.LocalizedStringTables.ToList();
                        if (_theMostTranslatedCulture != null)
                        {
                            // Duplicate localization for culture with the highest amount of keys
                            var g = locales.First(x => x.Key == _theMostTranslatedCulture.Name);
                            foreach (var e in g)
                            {
                                var path = e.Path;
                                var filename = Path.GetFileNameWithoutExtension(path);
                                if (filename.EndsWith(_theMostTranslatedCulture.Name))
                                    filename = filename.Substring(0, filename.Length - _theMostTranslatedCulture.Name.Length);
                                path = Path.Combine(Path.GetDirectoryName(path), filename + culture.Name + ".json");
                                var table = FlaxEngine.Content.CreateVirtualAsset<LocalizedStringTable>();
                                table.Locale = culture.Name;
                                var entries = new Dictionary<string, string[]>();
                                foreach (var ee in tableEntries[e])
                                {
                                    var vv = (string[])ee.Value.Clone();
                                    for (var i = 0; i < vv.Length; i++)
                                        vv[i] = string.Empty;
                                    entries.Add(ee.Key, vv);
                                }
                                table.Entries = entries;
                                if (!table.Save(path))
                                {
                                    Object.Destroy(table);
                                    newTables.Add(FlaxEngine.Content.LoadAsync<LocalizedStringTable>(path));
                                }
                            }
                        }
                        else
                        {
                            // No localization so initialize with empty table
                            var path = Path.Combine(Path.Combine(Path.GetDirectoryName(GameSettings.Load().Localization.Path), "Localization", culture.Name + ".json"));
                            var table = FlaxEngine.Content.CreateVirtualAsset<LocalizedStringTable>();
                            table.Locale = culture.Name;
                            if (!table.Save(path))
                            {
                                Object.Destroy(table);
                                newTables.Add(FlaxEngine.Content.LoadAsync<LocalizedStringTable>(path));
                            }
                        }
                        settings.LocalizedStringTables = newTables.ToArray();
                        Presenter.OnModified();
                        RebuildLayout();
                    });
                    menu.Show(button, new Vector2(0, button.Height));
                };
            }

            {
                // Raw asset data editing
                var group = layout.Group("Data");
                base.Initialize(group);
            }
        }
    }
}
