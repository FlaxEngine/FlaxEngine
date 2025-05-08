// Copyright (c) Wojciech Figat. All rights reserved.

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
        /// <inheritdoc/>
        public override bool CanBeCreated => true;

        /// <summary>
        /// Types of the emitter templates that can be created.
        /// </summary>
        public enum Templates
        {
            /// <summary>
            /// An empty emitter.
            /// </summary>
            Empty,

            /// <summary>
            /// An emitter that emits particles at a constant emission rate.
            /// </summary>
            ConstantBurst,

            /// <summary>
            /// An emitter that produces simple, periodic bursts of particles.
            /// </summary>
            PeriodicBurst,

            /// <summary>
            /// An emitter that uses a blended spritesheet to produce a smooth, thick cloud of smoke.
            /// </summary>
            Smoke,

            /// <summary>
            /// A GPU emitter that produces sparks that can collide, thanks to depth-buffer based collisions.
            /// </summary>
            Sparks,

            /// <summary>
            /// An emitter that produces a spiral shaped ribbon.
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
