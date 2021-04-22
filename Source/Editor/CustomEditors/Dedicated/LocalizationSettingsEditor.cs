// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors.Dedicated
{
    [CustomEditor(typeof(LocalizationSettings))]
    sealed class LocalizationSettingsEditor : GenericEditor
    {
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
                foreach (var e in locales)
                {
                    var culture = new CultureInfo(e.Key);
                    var prop = group.AddPropertyItem(CultureInfoEditor.GetName(culture), culture.NativeName);
                    int count = e.Sum(x => tableEntries[x].Values.Count(y => y != null && y.Length != 0 && !string.IsNullOrEmpty(y[0])));
                    prop.Label(string.Format("Progress: {0}% ({1}/{2})", (int)(((float)count / allKeys.Count * 100.0f)), count, allKeys.Count));
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
            }

            {
                // Raw asset data editing
                var group = layout.Group("Data");
                base.Initialize(group);
            }
        }
    }
}
