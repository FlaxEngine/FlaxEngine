using System;
using System.Collections.Generic;
using FlaxEditor.Viewport;
using FlaxEditor.Viewport.Widgets;
using FlaxEngine;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Used to store and manage widgets used in viewports.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public class ViewportWidgetsModule : EditorModule
    {

        //holds all widget instances
        private Dictionary<EditorViewport, Dictionary<WidgetNames, ViewportWidget>> _widgetsByViewport = new();

        //factory
        private readonly Dictionary<WidgetNames, Func<EditorViewport, ViewportWidget>> _widgetFactories = new()
        {
            [WidgetNames.Camera] = vp => new CameraSettingsWidget(0, 0, vp),
            [WidgetNames.Viewmode] = vp => new ViewmodeWidget(0, 0, vp),
        };


        public enum WidgetNames
        {
            Camera,
            Viewmode
        }

        internal ViewportWidgetsModule(Editor editor)
        : base(editor)
        {
            InitOrder = -74;
        }

        public override void OnInit()
        {
            InitializeViewportWidgets();
        }

        public void InitializeViewportWidgets()
        {
            //defines which widgets are in a viewport
            Dictionary<EditorViewport, List<WidgetNames>> _editorViewportWidgets = new()
            {
                [Editor.Instance.Windows.EditWin.Viewport] = new List<WidgetNames>
            {
                WidgetNames.Camera,
                WidgetNames.Viewmode
            },
            };

            foreach (var pair in _editorViewportWidgets)
            {
                var viewport = pair.Key;
                if (viewport == null)
                {
                    Debug.LogWarning("InitializeViewportWidgets: Encountered null EditorViewport.");
                    continue;
                }

                var widgetNames = pair.Value;
                if (widgetNames == null || widgetNames.Count == 0)
                {
                    Debug.LogWarning($"InitializeViewportWidgets: No widget names defined for viewport {viewport}.");
                    continue;
                }

                // Ensure dictionary entry exists
                if (!_widgetsByViewport.ContainsKey(viewport))
                    _widgetsByViewport[viewport] = new Dictionary<WidgetNames, ViewportWidget>();

                foreach (var name in widgetNames)
                {
                    AddToViewport(viewport, name);
                }
            }
        }


        public void AddToViewport(EditorViewport viewport, WidgetNames name)
        {
            if (viewport == null)
            {
                Debug.LogWarning("AddToViewport: Provided viewport is null.");
                return;
            }

            if (!_widgetFactories.TryGetValue(name, out var factory))
            {
                Debug.LogWarning($"AddToViewport: No factory found for widget '{name}'.");
                return;
            }

            if (!_widgetsByViewport.TryGetValue(viewport, out var widgetMap))
            {
                widgetMap = new Dictionary<WidgetNames, ViewportWidget>();
                _widgetsByViewport[viewport] = widgetMap;
            }

            var widget = factory(viewport);
            if (widget == null)
            {
                Debug.LogWarning($"AddToViewport: Factory for widget '{name}' returned null.");
                return;
            }

            widgetMap[name] = widget;
        }
    }
}
