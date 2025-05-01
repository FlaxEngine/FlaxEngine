// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    /// <summary>
    /// The build module from 3rd Party source.
    /// </summary>
    /// <seealso cref="Flax.Build.Module" />
    public class ThirdPartyModule : Module
    {
        /// <summary>
        /// The 3rd Party Module license types.
        /// </summary>
        public enum LicenseTypes
        {
            /// <summary>
            /// The invalid.
            /// </summary>
            Invalid,

            /// <summary>
            /// The custom license (license file must be specified).
            /// </summary>
            Custom,

            /// <summary>
            /// Apache license 2.0 (apache-2.0)
            /// </summary>
            Apache2,

            /// <summary>
            /// Boost Software License 1.0 (bsl-1.0)
            /// </summary>
            BoostSoftwareLicense,

            /// <summary>
            /// BSD 2-clause "Simplified" license (bsd-2-clause)
            /// </summary>
            BSD2Clause,

            /// <summary>
            /// BSD 3-clause "New" or "Revised" license (bsd-3-clause)
            /// </summary>
            BSD3Clause,

            /// <summary>
            /// BSD 3-clause Clear license (bsd-3-clause-clear)
            /// </summary>
            BSD3ClauseClear,

            /// <summary>
            /// Creative Commons license family	(cc)
            /// </summary>
            CreativeCommons,

            /// <summary>
            /// Creative Commons Zero v1.0 Universal (cc0-1.0)
            /// </summary>
            CreativeCommonsZero,

            /// <summary>
            /// Creative Commons Attribution 4.0 (cc-by-4.0)
            /// </summary>
            CreativeCommonsAttribution,

            /// <summary>
            /// Creative Commons Attribution Share Alike 4.0 (cc-by-sa-4.0)
            /// </summary>
            CreativeCommonsAttributionShareAlike,

            /// <summary>
            /// Educational Community License v2.0 (ecl-2.0)
            /// </summary>
            ECL2,

            /// <summary>
            /// Eclipse Public License 1.0 (epl-1.0)
            /// </summary>
            EPL1,

            /// <summary>
            /// European Union Public License 1.1 (eupl-1.1)
            /// </summary>
            EUPL11,

            /// <summary>
            /// GNU Affero General Public License v3.0 (agpl-3.0)
            /// </summary>
            AGPL3,

            /// <summary>
            /// GNU General Public License family (gpl)
            /// </summary>
            GPL,

            /// <summary>
            /// GNU General Public License v2.0 (gpl-2.0)
            /// </summary>
            GPL2,

            /// <summary>
            /// GNU General Public License v3.0 (gpl-3.0)
            /// </summary>
            GPL3,

            /// <summary>
            /// GNU Lesser General Public License family (lgpl)
            /// </summary>
            LGPL,

            /// <summary>
            /// GNU Lesser General Public License v2.1 (lgpl-2.1)
            /// </summary>
            LGPL21,

            /// <summary>
            /// GNU Lesser General Public License v3.0 (lgpl-3.0)
            /// </summary>
            LGPL3,

            /// <summary>
            /// ISC (isc)
            /// </summary>
            ISC,

            /// <summary>
            /// LaTeX Project Public License v1.3c (lppl-1.3c)
            /// </summary>
            LaTeXProjectPublicLicense,

            /// <summary>
            /// Microsoft Public License (ms-pl)
            /// </summary>
            MicrosoftPublicLicense,

            /// <summary>
            /// MIT (mit)
            /// </summary>
            MIT,

            /// <summary>
            ///  Mozilla Public License 2.0 (mpl-2.0)
            /// </summary>
            MozillaPublicLicense2,

            /// <summary>
            /// Open Software License 3.0 (osl-3.0)
            /// </summary>
            OpenSoftwareLicense,

            /// <summary>
            /// SIL Open Font License 1.1 (ofl-1.1)
            /// </summary>
            OpenFontLicense,

            /// <summary>
            /// University of Illinois/NCSA Open Source License (ncsa)
            /// </summary>
            NCSA,

            /// <summary>
            /// The Unlicense (unlicense)
            /// </summary>
            Unlicense,

            /// <summary>
            /// zLib License (zlib)
            /// </summary>
            zLib,
        }

        /// <summary>
        /// The license type.
        /// </summary>
        public LicenseTypes LicenseType = LicenseTypes.Invalid;

        /// <summary>
        /// The path to the license file (relative to the module file).
        /// </summary>
        public string LicenseFilePath;

        /// <inheritdoc />
        public ThirdPartyModule()
        {
            // Third-party modules are native and don't use bindings by default
            BuildCSharp = false;
        }

        /// <inheritdoc />
        public override void Setup(BuildOptions options)
        {
            base.Setup(options);

            // Perform license validation
            if (LicenseType == LicenseTypes.Invalid)
                throw new Exception(string.Format("Cannot build module {0}. Third Party modules must have license type specified.", Name));
            if (LicenseFilePath == null)
                throw new Exception(string.Format("Cannot build module {0}. Third Party modules must have license file specified.", Name));
            if (!File.Exists(Path.Combine(FolderPath, LicenseFilePath)))
                throw new Exception(string.Format("Cannot build module {0}. Specified license file does not exist.", Name));
        }

        /// <inheritdoc />
        public override void GetFilesToDeploy(List<string> files)
        {
            if (LicenseFilePath != null && File.Exists(Path.Combine(FolderPath, LicenseFilePath)))
                files.Add(Path.Combine(FolderPath, LicenseFilePath));
        }
    }
}
