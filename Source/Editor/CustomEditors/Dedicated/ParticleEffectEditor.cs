// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.Surface;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="ParticleEffect"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(ParticleEffect)), DefaultEditor]
    public class ParticleEffectEditor : ActorEditor
    {
        private bool _isValid;

        private bool IsValid
        {
            get
            {
                // All selected particle effects use the same system
                var effect = (ParticleEffect)Values[0];
                var system = effect.ParticleSystem;
                return system != null && Values.TrueForAll(x => (x as ParticleEffect)?.ParticleSystem == system);
            }
        }

        private object ParameterGet(object instance, GraphParameter parameter, object tag)
        {
            if (instance is ParticleEffect particleEffect && particleEffect && parameter && tag is ParticleEffectParameter effectParameter && effectParameter)
                return particleEffect.GetParameterValue(effectParameter.TrackName, parameter.Name);
            return null;
        }

        private void ParameterSet(object instance, object value, GraphParameter parameter, object tag)
        {
            if (instance is ParticleEffect particleEffect && particleEffect && parameter && tag is ParticleEffectParameter effectParameter && effectParameter)
                particleEffect.SetParameterValue(effectParameter.TrackName, parameter.Name, value);
        }

        private object ParameterDefaultValue(object instance, GraphParameter parameter, object tag)
        {
            if (tag is ParticleEffectParameter effectParameter)
                return effectParameter.DefaultValue;
            return null;
        }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            _isValid = IsValid;
            if (!_isValid)
                return;

            // Show all effect parameters grouped by the emitter track name
            var effect = (ParticleEffect)Values[0];
            var groups = layout.Group("Parameters");
            groups.Panel.Open(false);
            var parameters = effect.Parameters;
            var parametersGroups = parameters.GroupBy(x => x.EmitterIndex);
            foreach (var parametersGroup in parametersGroups)
            {
                var trackName = parametersGroup.First().TrackName;
                var group = groups.Group(trackName);
                group.Panel.Open(false);

                var data = SurfaceUtils.InitGraphParameters(parametersGroup);
                SurfaceUtils.DisplayGraphParameters(group, data, ParameterGet, ParameterSet, Values, ParameterDefaultValue);
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            if (_isValid != IsValid)
            {
                RebuildLayout();
                return;
            }

            base.Refresh();
        }
    }
}
