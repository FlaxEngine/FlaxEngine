// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using FlaxEditor.Content;
using FlaxEditor.GUI;
using FlaxEditor.Modules;
using FlaxEditor.Scripting;
using FlaxEditor.Surface;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Editor tool window for figuring out what is referencing an <see cref="Asset"/> by Guid which can be useful to debug what assets are missing.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    internal sealed class ReferenceLocatorWindow : EditorWindow
    {
        private Panel _resultsPanel;
        private GridPanel _assetIDInputControl;
        private Panel _labelPanel;
        private Label _resultsLabel;
        private TextBox _assetIDInput;
        private CancellationTokenSource _token;
        private Task _task;
        private const float SizePerResult = 0.1f;
        private const float ButtonScaleDifference = 0.25f;
        private const float ButtonStartPosition = 0.755f;
        private const float ButtonEndPosition = 0.775f;

        // Async task data
        private float _progress;
        private Guid[] _refs;

        /// <summary>
        /// Initializes a new instance of the <see cref="PluginsWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public ReferenceLocatorWindow(Editor editor)
        : base(editor, false, ScrollBars.None)
        {
            Title = "Asset Reference Locator";

            _resultsPanel = new Panel
            {
                BackgroundColor = Style.Current.LightBackground,
                AnchorPreset = AnchorPresets.Custom,
                AnchorMin = new Float2(0, 0.075f),
                AnchorMax = new Float2(1f, 1f),
                Parent = this,
            };

            _resultsLabel = new Label
            {
                Text = "Results:",
                Margin = new Margin(5, 0, 5, 0),
                HorizontalAlignment = TextAlignment.Near,
                AnchorPreset = AnchorPresets.TopLeft,
                Parent = _resultsPanel,
            };

            _assetIDInput = new TextBox
            {
                WatermarkText = "Search Asset By GUID",
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Proxy_Offset_Top = 5,
                Parent = this,
            };

            _assetIDInput.EditEnd += AssetIdSubmitted;

            // Start async initialization
            //_token = new CancellationTokenSource();
            //_task = Task.Run(Load, _token.Token);
        }

        private void AssetIdSubmitted()
        {
            SearchRefs(_assetIDInput.Text);
        }

        private void CreateReferenceDisplay(Asset referencingAsset, int displayIndex)
        {
            Label assetLabel = new Label
            {
                Text = referencingAsset.ToString(),
                Wrapping = TextWrapping.WrapWords,
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Center,

                Margin = new Margin(5, 0, 0, 0),
                AnchorPreset = AnchorPresets.Custom,
                AnchorMin = new Float2(0f, (displayIndex + 1) * SizePerResult),
                AnchorMax = new Float2(0.65f, (displayIndex + 2) * SizePerResult),
                BackgroundColor = Style.Current.BackgroundNormal,
                Parent = _resultsPanel,
                IndexInParent = displayIndex + 1,
            };

            Button assetSelectorButton = new Button
            {
                Text = "Go To Asset",
                AnchorPreset = AnchorPresets.Custom,
                AnchorMin = new Float2(ButtonStartPosition, ((displayIndex + 1) * SizePerResult) + ButtonScaleDifference * SizePerResult),
                AnchorMax = new Float2(ButtonEndPosition, ((displayIndex + 2) * SizePerResult) - ButtonScaleDifference * SizePerResult),
                BackgroundColor = Style.Current.BackgroundNormal,
                Parent = _resultsPanel,
                IndexInParent = displayIndex + 1,
            };

            assetSelectorButton.ButtonClicked += (Button b) =>
            {
                AssetItem assetItem = Editor.Instance.ContentDatabase.FindAsset(referencingAsset.ID);
                Editor.Instance.ContentEditing.Open(assetItem);
            };
        }

        private void ClearReferenceDisplay()
        {
            _resultsPanel.RemoveChildren();
            _resultsLabel.Parent = _resultsPanel;
        }

        private void SearchRefs(string assetId)
        {
            Dictionary<Guid, Asset> assetsReferencingDictionary = new Dictionary<Guid, Asset>();

            int assetCount = FlaxEngine.Content.Assets.Length;
            foreach (Asset asset in FlaxEngine.Content.Assets)
            {
                Guid[] references = asset.GetReferences();
                foreach (Guid reference in references)
                {
                    string referenceIdString = FlaxEngine.Json.JsonSerializer.GetStringID(reference);
                    string assetIdString = assetId;

                    //Debug.Log("Comparing Reference '"+ referenceIdString + "' in " + "'" + assetIdString + "'");
                    if (string.Compare(referenceIdString, assetIdString) == 0)
                    {
                        assetsReferencingDictionary[asset.ID] = asset;
                        //Debug.Log("Asset " + asset.ToString() + " has reference to specified GUID.");
                    }
                }
            }

            ClearReferenceDisplay();
            Asset[] assetsReferencing = assetsReferencingDictionary.Values.ToArray();
            for (int referencingIndex = 0; referencingIndex < assetsReferencing.Length; referencingIndex++)
            {
                CreateReferenceDisplay(assetsReferencing[referencingIndex], referencingIndex);
            }
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            base.OnUpdate();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Wait for async end
            //_token.Cancel();
            //_task.Wait();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public Undo Undo => null;
    }
}
