// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Content.Thumbnails
{
    /// <summary>
    /// Contains information about asset thumbnail rendering.
    /// </summary>
    [HideInEditor]
    public class ThumbnailRequest
    {
        /// <summary>
        /// The request state types.
        /// </summary>
        public enum States
        {
            /// <summary>
            /// The initial state.
            /// </summary>
            Created,

            /// <summary>
            /// Request has been prepared for the rendering but still may wait for resources to load fully.
            /// </summary>
            Prepared,

            /// <summary>
            /// The thumbnail has been rendered. Request can be finalized.
            /// </summary>
            Rendered,

            /// <summary>
            /// The finalized state.
            /// </summary>
            Disposed,

            /// <summary>
            /// The request has failed (eg. asset cannot be loaded).
            /// </summary>
            Failed,
        };

        /// <summary>
        /// Gets the state.
        /// </summary>
        public States State { get; private set; } = States.Created;

        /// <summary>
        /// The item.
        /// </summary>
        public readonly AssetItem Item;

        /// <summary>
        /// The proxy object for the asset item.
        /// </summary>
        public readonly AssetProxy Proxy;

        /// <summary>
        /// The asset reference. May be null if not cached yet.
        /// </summary>
        public Asset Asset;

        /// <summary>
        /// The custom tag object used by the thumbnails rendering pipeline. Can be used to store the data related to the thumbnail rendering by the asset proxy.
        /// </summary>
        public object Tag;

        /// <summary>
        /// Determines whether thumbnail can be drawn for the item.
        /// </summary>
        public bool IsReady => State == States.Prepared && Asset && Asset.IsLoaded && Proxy.CanDrawThumbnail(this);

        /// <summary>
        /// Initializes a new instance of the <see cref="ThumbnailRequest"/> class.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="proxy">The proxy.</param>
        public ThumbnailRequest(AssetItem item, AssetProxy proxy)
        {
            Item = item;
            Proxy = proxy;
        }

        internal void Update()
        {
            if (State == States.Prepared && (!Asset || Asset.LastLoadFailed))
            {
                State = States.Failed;
            }
        }

        /// <summary>
        /// Prepares this request.
        /// </summary>
        public void Prepare()
        {
            if (State != States.Created)
                throw new InvalidOperationException();
            Asset = FlaxEngine.Content.LoadAsync(Item.Path);
            Proxy.OnThumbnailDrawPrepare(this);
            State = States.Prepared;
        }

        /// <summary>
        /// Finishes the rendering and updates the item thumbnail.
        /// </summary>
        /// <param name="icon">The icon.</param>
        public void FinishRender(ref SpriteHandle icon)
        {
            if (State != States.Prepared)
                throw new InvalidOperationException();
            Item.Thumbnail = icon;
            State = States.Rendered;
        }

        /// <summary>
        /// Finalizes this request.
        /// </summary>
        public void Dispose()
        {
            if (State == States.Disposed)
                throw new InvalidOperationException();

            if (State != States.Created)
            {
                // Cleanup
                Proxy.OnThumbnailDrawCleanup(this);
                Asset = null;
            }

            Tag = null;
            State = States.Disposed;
        }
    }
}
