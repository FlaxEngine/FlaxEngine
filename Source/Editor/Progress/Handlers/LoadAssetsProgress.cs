// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.Progress.Handlers
{
    /// <summary>
    /// Loading assets progress reporting handler.
    /// </summary>
    /// <seealso cref="FlaxEditor.Progress.ProgressHandler" />
    public sealed class LoadAssetsProgress : ProgressHandler
    {
        private int _loadingAssetsCount;

        /// <summary>
        /// Initializes a new instance of the <see cref="LoadAssetsProgress"/> class.
        /// </summary>
        public LoadAssetsProgress()
        {
            Editor.Instance.EditorUpdate += OnEditorUpdate;
        }

        private void OnEditorUpdate()
        {
            var contentStats = FlaxEngine.Content.Stats;
            if (_loadingAssetsCount == contentStats.LoadingAssetsCount)
                return;

            if (contentStats.LoadingAssetsCount == 0)
                OnEnd();
            else if (_loadingAssetsCount == 0)
                OnStart();
            if (contentStats.LoadingAssetsCount > 0)
            {
                var progress = (float)contentStats.LoadedAssetsCount / contentStats.AssetsCount;
                var text = contentStats.LoadingAssetsCount == 1 ? "Loading 1 asset..." : $"Loading {contentStats.LoadingAssetsCount} assets...";
                OnUpdate(progress, text);
            }
            _loadingAssetsCount = contentStats.LoadingAssetsCount;
        }
    }
}
