// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
            }
        }
    }
}
