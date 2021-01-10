// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    /// <summary>
    /// The Android platform settings asset archetype. Allows to edit asset via editor.
    /// </summary>
    public class AndroidPlatformSettings : SettingsBase
    {
        /// <summary>
        /// The application package name (eg. com.company.product). Custom tokens: ${PROJECT_NAME}, ${COMPANY_NAME}.
        /// </summary>
        [EditorOrder(0), EditorDisplay("General"), Tooltip("The application package name (eg. com.company.product). Custom tokens: ${PROJECT_NAME}, ${COMPANY_NAME}.")]
        public string PackageName = "com.${COMPANY_NAME}.${PROJECT_NAME}";

        /// <summary>
        /// The application permissions list (eg. android.media.action.IMAGE_CAPTURE). Added to the generated manifest file.
        /// </summary>
        [EditorOrder(100), EditorDisplay("General"), Tooltip("The application permissions list (eg. android.media.action.IMAGE_CAPTURE). Added to the generated manifest file.")]
        public string[] Permissions;

        /// <summary>
        /// Custom icon texture to use for the application (overrides the default one).
        /// </summary>
        [EditorOrder(1030), EditorDisplay("Other"), Tooltip("Custom icon texture to use for the application (overrides the default one).")]
        public Texture OverrideIcon;
    }
}
