// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Content.Create
{
    /// <summary>
    /// Prefab asset creating handler. Allows to specify base actor to use as the root.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.Create.CreateFileEntry" />
    public class PrefabCreateEntry : CreateFileEntry
    {
        /// <inheritdoc />
        public override bool CanBeCreated => _options.RootActorType != null;

        /// <summary>
        /// The create options.
        /// </summary>
        public class Options
        {
            /// <summary>
            /// The root actor.
            /// </summary>
            [TypeReference(typeof(FlaxEngine.Actor), nameof(IsValid))]
            [Tooltip("The actor type of the root of the new Prefab.")]
            public Type RootActorType = typeof(EmptyActor);

            private static bool IsValid(Type type)
            {
                return (type.IsPublic || type.IsNestedPublic) && !type.IsAbstract && !type.IsGenericType;
            }
        }

        private readonly Options _options = new Options();

        /// <inheritdoc />
        public override object Settings => _options;

        /// <summary>
        /// Initializes a new instance of the <see cref="PrefabCreateEntry"/> class.
        /// </summary>
        /// <param name="resultUrl">The result file url.</param>
        public PrefabCreateEntry(string resultUrl)
        : base("Settings", resultUrl)
        {
        }

        /// <inheritdoc />
        public override bool Create()
        {
            var actorType = new ScriptType(_options.RootActorType ?? typeof(EmptyActor));
            Actor actor;
            try
            {
                actor = actorType.CreateInstance() as Actor;
                Object.Destroy(actor, 20.0f);
            }
            catch (Exception ex)
            {
                Editor.LogError("Failed to create prefab with root actor type: " + actorType.Name);
                Editor.LogWarning(ex);
                return true;
            }

            return PrefabManager.CreatePrefab(actor, ResultUrl, true);
        }
    }

    /// <summary>
    /// Widget asset creating handler. Allows to specify base UIControl to use as the root.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.Create.CreateFileEntry" />
    public class WidgetCreateEntry : CreateFileEntry
    {
        /// <inheritdoc/>
        public override bool CanBeCreated => _options.RootControlType != null;

        /// <summary>
        /// The create options.
        /// </summary>
        public class Options
        {
            /// <summary>
            /// Which mode is used to initialize this widget.
            /// </summary>
            public enum WidgetMode
            {
                /// <summary>
                /// Initialize the widget with a UICanvas.
                /// </summary>
                Canvas,

                /// <summary>
                /// Initialize the widget with a UIControl.
                /// </summary>
                Control
            }

            /// <summary>
            /// The mode used to initialize the widget.
            /// </summary>
            [Tooltip("Whether to initialize the widget with a canvas or a control.")]
            public WidgetMode WidgetInitializationMode = WidgetMode.Control;

            bool ShowRoot => WidgetInitializationMode == WidgetMode.Control;

            /// <summary>
            /// The root control.
            /// </summary>
            [TypeReference(typeof(Control), nameof(IsValid))]
            [Tooltip("The control type of the root of the new Widget's root control."), VisibleIf(nameof(ShowRoot))]
            public Type RootControlType = typeof(Button);

            private static bool IsValid(Type type)
            {
                return (type.IsPublic || type.IsNestedPublic) && !type.IsAbstract && !type.IsGenericType;
            }
        }

        private readonly Options _options = new Options();

        /// <inheritdoc />
        public override object Settings => _options;

        /// <summary>
        /// Initializes a new instance of the <see cref="WidgetCreateEntry"/> class.
        /// </summary>
        /// <param name="resultUrl">The result file url.</param>
        public WidgetCreateEntry(string resultUrl)
        : base("Settings", resultUrl)
        {
        }

        /// <inheritdoc />
        public override bool Create()
        {
            Actor actor = null;

            if (_options.WidgetInitializationMode == Options.WidgetMode.Control)
            {
                var controlType = new ScriptType(_options.RootControlType ?? typeof(Control));
                Control control;
                try
                {
                    control = controlType.CreateInstance() as Control;
                }
                catch (Exception ex)
                {
                    Editor.LogError("Failed to create widget with root control type: " + controlType.Name);
                    Editor.LogWarning(ex);
                    return true;
                }

                actor = new UIControl
                {
                    Control = control,
                    Name = controlType.Name
                };
            }
            else if (_options.WidgetInitializationMode == Options.WidgetMode.Canvas)
            {
                actor = new UICanvas();
            }

            if (actor == null)
            {
                Editor.LogError("Failed to create widget. Final actor was null.");
                return true;
            }
            Object.Destroy(actor, 20.0f);

            return PrefabManager.CreatePrefab(actor, ResultUrl, true);
        }
    }
}
