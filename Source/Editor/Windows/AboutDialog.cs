// Copyright (c) Wojciech Figat. All rights reserved.
//#define USE_AUTODESK_FBX_SDK

using System.Collections.Generic;
using FlaxEditor.GUI.Dialogs;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// About this product dialog window.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Dialogs.Dialog" />
    [HideInEditor]
    internal sealed class AboutDialog : Dialog
    {
        /// <inheritdoc />
        public AboutDialog()
        : base("About Flax")
        {
            _dialogSize = Size = new Float2(400, 320);

            Control header = CreateHeader();
            Control authorsLabel = CreateAuthorsLabels(header);
            CreateFooter(authorsLabel);
        }

        /// <summary>
        /// Create header with Flax engine icon and version number
        /// </summary>
        /// <returns>Returns icon controller (most top left)</returns>
        private Control CreateHeader()
        {
            Image icon = new Image(4, 4, 80, 80)
            {
                Brush = new SpriteBrush(Editor.Instance.Icons.FlaxLogo128),
                Parent = this
            };
            var nameLabel = new Label(icon.Right + 10, icon.Top, 200, 34)
            {
                Text = "Flax Engine",
                Font = new FontReference(Style.Current.FontTitle),
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Center,
                Parent = this
            };
            new Label(nameLabel.Left, nameLabel.Bottom + 4, nameLabel.Width, 50)
            {
                Text = string.Format("Version: {0}\nCopyright (c) 2012-2025 Wojciech Figat.\nAll rights reserved.", Globals.EngineVersion),
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                Parent = this
            };
            var buttonText = "Copy version info";
            var fontSize = Style.Current.FontMedium.MeasureText(buttonText);
            var copyVersionButton = new Button(Width - fontSize.X - 8, 6, fontSize.X + 4, 20)
            {
                Text = buttonText,
                TooltipText = "Copies the current engine version information to system clipboard.",
                Parent = this
            };
            copyVersionButton.Clicked += () => Clipboard.Text = Globals.EngineVersion;
            return icon;
        }

        /// <summary>
        /// Create footer label
        /// </summary>
        /// <param name="topParentControl">Top element that this footer should be put under</param>
        private void CreateFooter(Control topParentControl)
        {
            Panel thirdPartyPanel = GenerateThirdPartyLabels(topParentControl);
            new Label(4, thirdPartyPanel.Bottom, Width - 8, Height - thirdPartyPanel.Bottom)
            {
                HorizontalAlignment = TextAlignment.Far,
                Text = "Made with <3 in Poland",
                Parent = this
            };
        }

        /// <summary>
        /// Generates authors labels.
        /// </summary>
        /// <param name="topParentControl">The top element that this labels should be put under.</param>
        /// <returns>The created control</returns>
        private Control CreateAuthorsLabels(Control topParentControl)
        {
            var authors = new List<string>(new[]
            {
                "Wojciech Figat",
                "Tomasz Juszczak",
                "Damian Korczowski",
                "Michał Winiarski",
                "Stefan Brandmair",
                "Lukáš Jech",
                "Jean-Baptiste Perrier",
                "Chandler Cox",
                "Ari Vuollet",
            });
            authors.Sort();
            var authorsLabel = new Label(4, topParentControl.Bottom + 20, Width - 8, 70)
            {
                Text = "People who made it:\n" + string.Join(", ", authors),
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                Wrapping = TextWrapping.WrapWords,
                Parent = this
            };
            return authorsLabel;
        }

        /// <summary>
        /// Generates 3rdParty software and other licenses labels.
        /// </summary>
        /// <param name="topParentControl">The top element that this labels should be put under.</param>
        /// <returns>The created control</returns>
        private Panel GenerateThirdPartyLabels(Control topParentControl)
        {
            var thirdPartyPanel = new Panel(ScrollBars.Vertical)
            {
                Bounds = new Rectangle(4, topParentControl.Bottom + 4, Width - 8, Height - topParentControl.Bottom - 24),
                Parent = this
            };
            var thirdPartyEntries = new[]
            {
                "Used third party software:",
                "",
                "Mono Project - www.mono-project.com",
#if USE_NETCORE
                ".NET - www.dotnet.microsoft.com",
#endif
                "FreeType Project - www.freetype.org",
                "Assimp - www.assimp.sourceforge.net",
                "DirectXMesh - Copyright (c) Microsoft Corporation. All rights reserved.",
                "DirectXTex - Copyright (c) Microsoft Corporation. All rights reserved.",
                "UVAtlas - Copyright (c) Microsoft Corporation. All rights reserved.",
                "LZ4 Library - Copyright (c) Yann Collet. All rights reserved.",
                "fmt - www.fmtlib.net",
                "minimp3 - www.github.com/lieff/minimp3",
                "Tracy Profiler - www.github.com/wolfpld/tracy",
                "Ogg and Vorbis - Xiph.org Foundation",
                "OpenAL Soft - www.github.com/kcat/openal-soft",
                "OpenFBX - www.github.com/nem0/OpenFBX",
                "Recast & Detour - www.github.com/recastnavigation/recastnavigation",
                "pugixml - www.pugixml.org",
                "rapidjson - www.rapidjson.org",
#if USE_AUTODESK_FBX_SDK
				"Autodesk FBX - Copyright (c) Autodesk",
#endif
                "Editor icons - www.icons8.com, www.iconfinder.com",
                "",
#if USE_AUTODESK_FBX_SDK
				"This software contains Autodesk® FBX® code developed by Autodesk, Inc.",
				"Copyright 2017 Autodesk, Inc. All rights, reserved.",
				"Such code is provided “as is” and Autodesk, Inc. disclaims any and all",
				"warranties, whether express or implied, including without limitation",
				"the implied warranties of merchantability, fitness for a particular",
				"purpose or non - infringement of third party rights.In no event shall",
				"Autodesk, Inc.be liable for any direct, indirect, incidental, special,",
				"exemplary, or consequential damages(including, but not limited to," ,
				"procurement of substitute goods or services; loss of use, data, or",
				"profits; or business interruption) however caused and on any theory" ,
				"of liability, whether in contract, strict liability, or tort" ,
				"(including negligence or otherwise)",
				"arising in any way out of such code.",
#endif
            };
            float y = 0;
            float width = Width;
            for (var i = 0; i < thirdPartyEntries.Length; i++)
            {
                var entry = thirdPartyEntries[i];
                var entryLabel = new Label(0, y, width, 14)
                {
                    Text = entry,
                    HorizontalAlignment = TextAlignment.Near,
                    VerticalAlignment = TextAlignment.Center,
                    Parent = thirdPartyPanel
                };
                y += entryLabel.Height + 2;
            }

            return thirdPartyPanel;
        }
    }
}
