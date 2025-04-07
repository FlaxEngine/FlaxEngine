// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="TextRender"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class TextRenderNode : ActorNode
    {
        /// <inheritdoc />
        public TextRenderNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override void PostSpawn()
        {
            base.PostSpawn();

            if (Actor.HasPrefabLink)
            {
                return;
            }

            // Setup for default values
            var text = (TextRender)Actor;
            text.Text = "My Text";
            text.Font = FlaxEngine.Content.LoadAsyncInternal<FontAsset>(EditorAssets.PrimaryFont);
            text.Material = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>(EditorAssets.DefaultFontMaterial);
        }
    }
}
