// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.Surface;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="AnimatedModel"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(AnimatedModel)), DefaultEditor]
    public class AnimatedModelEditor : ActorEditor
    {
        private bool _parametersAdded = false;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            // Show instanced parameters to view/edit at runtime
            if (Values.IsSingleObject && Editor.Instance.StateMachine.IsPlayMode)
            {
                var group = SurfaceUtils.InitGraphParametersGroup(layout);
                group.Panel.Open(false);
                group.Panel.IndexInParent -= 2;

                var animatedModel = (AnimatedModel)Values[0];
                var parameters = animatedModel.Parameters;
                var data = SurfaceUtils.InitGraphParameters(parameters);
                SurfaceUtils.DisplayGraphParameters(group, data,
                                                    (instance, parameter, tag) => ((AnimatedModel)instance).GetParameterValue(parameter.Identifier),
                                                    (instance, value, parameter, tag) => ((AnimatedModel)instance).SetParameterValue(parameter.Identifier, value),
                                                    Values);
                if (!parameters.Any())
                    group.Label("No parameters", TextAlignment.Center);
                _parametersAdded = true;
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            // Check if parameters group is still showing if not in play mode and hide it.
            if (!Editor.Instance.StateMachine.IsPlayMode && _parametersAdded)
            {
                var group = Layout.Children.Find(x => x is GroupElement g && g.Panel.HeaderText.Equals("Parameters", StringComparison.Ordinal));
                if (group != null)
                {
                    RebuildLayout();
                    _parametersAdded = false;
                }
            }
        }
    }
}
