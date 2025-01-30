// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.Surface;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="ParticleEffect"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(ParticleEffect)), DefaultEditor]
    public class ParticleEffectEditor : ActorEditor
    {
        private Label _infoLabel;
        private bool _isValid;
        private bool _isActive;
        private uint _parametersVersion;

        private bool IsValid
        {
            get
            {
                // All selected particle effects use the same system
                var effect = Values[0] as ParticleEffect;
                var system = effect ? effect.ParticleSystem : null;
                if (system && Values.TrueForAll(x => x is ParticleEffect fx && fx && fx.ParticleSystem == system))
                {
                    // All parameters can be accessed
                    var parameters = effect.Parameters;
                    foreach (var parameter in parameters)
                    {
                        if (!parameter)
                            return false;
                    }
                    return true;
                }
                return false;
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

        private void Foreach(Action<ParticleEffect> func)
        {
            foreach (var value in Values)
            {
                if (value is ParticleEffect player)
                    func(player);
            }
        }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            _isValid = IsValid;
            if (!_isValid)
                return;
            var effect = (ParticleEffect)Values[0];
            _parametersVersion = effect.ParametersVersion;
            _isActive = effect.IsActive;

            // Show playback options during simulation
            if (Editor.IsPlayMode)
            {
                var playbackGroup = layout.Group("Playback");
                playbackGroup.Panel.Open();

                _infoLabel = playbackGroup.Label(string.Empty).Label;
                _infoLabel.AutoHeight = true;

                var grid = playbackGroup.CustomContainer<UniformGridPanel>();
                var gridControl = grid.CustomControl;
                gridControl.ClipChildren = false;
                gridControl.Height = Button.DefaultHeight;
                gridControl.SlotsHorizontally = 3;
                gridControl.SlotsVertically = 1;
                grid.Button("Play").Button.Clicked += () => Foreach(x => x.Play());
                grid.Button("Pause").Button.Clicked += () => Foreach(x => x.Pause());
                grid.Button("Stop").Button.Clicked += () => Foreach(x => x.Stop());
            }

            // Show all effect parameters grouped by the emitter track name
            var groups = layout.Group("Parameters");
            groups.Panel.Open();
            var parameters = effect.Parameters;
            var parametersGroups = parameters.GroupBy(x => x.EmitterIndex);
            foreach (var parametersGroup in parametersGroups)
            {
                var trackName = parametersGroup.First().TrackName;
                var group = groups.Group(trackName);
                group.Panel.Open();

                var data = SurfaceUtils.InitGraphParameters(parametersGroup);
                SurfaceUtils.DisplayGraphParameters(group, data, ParameterGet, ParameterSet, Values, ParameterDefaultValue);
            }
            
            if (!parameters.Any())
                groups.Label("No parameters", TextAlignment.Center);
        }

        /// <inheritdoc />
        internal override void RefreshRootChild()
        {
            var effect = (ParticleEffect)Values[0];
            if (effect == null)
            {
                base.RefreshRootChild();
                return;
            }

            // Custom refreshing that handles particle effect parameters list editing during refresh (eg. effect gets disabled)
            if (_isValid != IsValid)
            {
                RebuildLayout();
                return;
            }
            Refresh();
            var parameters = effect.Parameters;
            if (parameters.Length == 0)
            {
                base.RefreshRootChild();
                return;
            }

            for (int i = 0; i < ChildrenEditors.Count; i++)
            {
                if (_isActive != effect.IsActive || _parametersVersion != effect.ParametersVersion)
                {
                    RebuildLayout();
                    return;
                }
                ChildrenEditors[i].RefreshInternal();
            }
        }
    }
}
