// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using FlaxEngine;

namespace FlaxEditor.Content.Create
{
    /// <summary>
    /// Particle emitter asset creating handler. Allows to specify asset template.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.Create.CreateFileEntry" />
    public class ParticleEmitterCreateEntry : CreateFileEntry
    {
        /// <summary>
        /// Types of the emitter templates that can be created.
        /// </summary>
        public enum Templates
        {
            /// <summary>
            /// The empty asset.
            /// </summary>
            Empty,

            /// <summary>
            /// The simple particle system that uses constant emission rate.
            /// </summary>
            ConstantBurst,

            /// <summary>
            /// The simple periodic burst particle system.
            /// </summary>
            PeriodicBurst,

            /// <summary>
            /// The layers and tags settings.
            /// </summary>
            Smoke,

            /// <summary>
            /// The GPU sparks with depth-buffer collisions.
            /// </summary>
            Sparks,

            /// <summary>
            /// The ribbon spiral particles.
            /// </summary>
            RibbonSpiral,
        }

        /// <summary>
        /// The create options.
        /// </summary>
        public class Options
        {
            /// <summary>
            /// The template.
            /// </summary>
            [Tooltip("Particle emitter template.")]
            public Templates Template = Templates.ConstantBurst;
        }

        private readonly Options _options = new Options();

        /// <inheritdoc />
        public override object Settings => _options;

        /// <summary>
        /// Initializes a new instance of the <see cref="ParticleEmitterCreateEntry"/> class.
        /// </summary>
        /// <param name="resultUrl">The result file url.</param>
        public ParticleEmitterCreateEntry(string resultUrl)
        : base("Settings", resultUrl)
        {
        }

        /// <inheritdoc />
        public override bool Create()
        {
            string templateName;
            switch (_options.Template)
            {
            case Templates.Empty:
                return Editor.CreateAsset("ParticleEmitter", ResultUrl);
            case Templates.ConstantBurst:
                templateName = "Constant Burst";
                break;
            case Templates.PeriodicBurst:
                templateName = "Periodic Burst";
                break;
            case Templates.Smoke:
                templateName = "Smoke";
                break;
            case Templates.Sparks:
                templateName = "Sparks";
                break;
            case Templates.RibbonSpiral:
                templateName = "Ribbon Spiral";
                break;
            default: throw new ArgumentOutOfRangeException();
            }
            var templatePath = Path.Combine(Globals.EngineContentFolder, "Editor/Particles", templateName + ".flax");
            return Editor.Instance.ContentEditing.CloneAssetFile(templatePath, ResultUrl, Guid.NewGuid());
        }
    }
}
