// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// Mono open source ECMA CLI, C# and .NET implementation. http://www.mono-project.com/
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class mono : Dependency
    {
        /// <inheritdoc />
        public override TargetPlatform[] Platforms
        {
            get
            {
                switch (BuildPlatform)
                {
                case TargetPlatform.Windows:
                    return new[]
                    {
                        TargetPlatform.Windows,
                        TargetPlatform.UWP,
                        TargetPlatform.XboxOne,
                        TargetPlatform.PS4,
                        TargetPlatform.XboxScarlett,
                    };
                case TargetPlatform.Linux:
                    return new[]
                    {
                        TargetPlatform.Linux,
                        TargetPlatform.Android,
                    };
                default: return new TargetPlatform[0];
                }
            }
        }

        private string root;
        private string monoPropsPath;
        private string monoPreprocesorDefines;
        private static List<string> monoExports;

        private static readonly string[] MonoExportsCustom =
        {
            "mono_value_box",
            "mono_add_internal_call",
        };

        private static readonly string[] MonoExportsIncludePrefixes =
        {
            "mono_free",
            "mono_array_",
            "mono_assembly_",
            "mono_class_",
            "mono_custom_attrs_",
            "mono_debug_",
            "mono_domain_",
            "mono_exception_",
            "mono_field_",
            "mono_free_",
            "mono_gc_",
            "mono_gchandle_",
            "mono_get_",
            "mono_image_",
            "mono_metadata_",
            "mono_method_",
            "mono_object_",
            "mono_profiler_",
            "mono_property_",
            "mono_raise_exception",
            "mono_reflection_",
            "mono_runtime_",
            "mono_signature_",
            "mono_stack_",
            "mono_string_",
            "mono_thread_",
            "mono_type_",
        };

        private static readonly string[] MonoExportsSkipPrefixes =
        {
            "mono_type_to_",
            "mono_thread_state_",
            "mono_thread_internal_",
            "mono_thread_info_",
            "mono_array_get_",
            "mono_array_to_byvalarray",
            "mono_assembly_apply_binding",
            "mono_assembly_invoke_search_hook_internal",
            "mono_assembly_is_in_gac",
            "mono_assembly_load_from_gac",
            "mono_assembly_load_full_gac_base_default",
            "mono_assembly_load_publisher_policy",
            "mono_assembly_name_from_token",
            "mono_assembly_remap_version",
            "mono_assembly_try_decode_skip_verification",
            "mono_class_create_runtime_vtable",
            "mono_class_from_name_checked_aux",
            "mono_class_get_appdomain_do_type_resolve_method",
            "mono_class_get_exception_handling_clause_class",
            "mono_class_get_field_idx",
            "mono_class_get_local_variable_info_class",
            "mono_class_get_pointer_class",
            "mono_class_get_runtime_generic_context_template",
            "mono_class_get_virtual_methods",
            "mono_class_has_default_constructor",
            "mono_class_has_gtd_parent",
            "mono_class_has_parent",
            "mono_class_implement_interface_slow",
            "mono_class_is_ginst",
            "mono_class_is_gtd",
            "mono_class_is_interface",
            "mono_class_is_magic_assembly",
            "mono_class_is_valid_generic_instantiation",
            "mono_class_is_variant_compatible_slow",
            "mono_class_need_stelemref_method",
            "mono_class_proxy_vtable",
            "mono_class_setup_vtable_full",
            "mono_class_try_get_unmanaged_function_pointer_attribute_class",
            "mono_class_unregister_image_generic_subclasses",
            "mono_custom_attrs_construct_by_type",
            "mono_custom_attrs_data_construct",
            "mono_debug_add_assembly",
            "mono_debug_format",
            "mono_debug_handles",
            "mono_debug_initialized",
            "mono_debug_log_thread_state_to_string",
            "mono_debug_open_image",
            "mono_debug_read_method",
            "mono_domain_asmctx_from_path",
            "mono_domain_assembly_preload",
            "mono_domain_assembly_search",
            "mono_domain_create_appdomain_checked",
            "mono_domain_create_appdomain_internal",
            "mono_domain_fire_assembly_load",
            "mono_domain_fire_assembly_unload",
            "mono_domain_from_appdomain_handle",
            "mono_exception_new_argument_internal",
            "mono_exception_new_by_name_domain",
            "mono_exception_stacktrace_obj_walk",
            "mono_field_get_addr",
            "mono_field_get_rva",
            "mono_field_resolve_flags",
            "mono_free_static_data",
            "mono_gc_init_finalizer_thread",
            "mono_get_array_new_va_signature",
            "mono_get_corlib_version",
            "mono_get_domainvar",
            "mono_get_exception_argument_internal",
            "mono_get_exception_missing_member",
            "mono_get_exception_runtime_wrapped_checked",
            "mono_get_exception_type_initialization_checked",
            "mono_get_field_token",
            "mono_get_int_type",
            "mono_get_method_from_token",
            "mono_get_reflection_missing_object",
            "mono_get_runtime_build_version",
            "mono_get_seq_point_for_native_offset",
            "mono_get_unique_iid",
            "mono_get_version_info",
            "mono_get_vtable_var",
            "mono_get_xdomain_marshal_type",
            "mono_image_add_cattrs",
            "mono_image_add_decl_security",
            "mono_image_add_memberef_row",
            "mono_image_add_methodimpl",
            "mono_image_basic_method",
            "mono_image_create_method_token",
            "mono_image_emit_manifest",
            "mono_image_fill_export_table",
            "mono_image_fill_export_table_from_class",
            "mono_image_fill_export_table_from_module",
            "mono_image_fill_export_table_from_type_forwarders",
            "mono_image_fill_file_table",
            "mono_image_fill_module_table",
            "mono_image_get_array_token",
            "mono_image_get_event_info",
            "mono_image_get_field_info",
            "mono_image_get_fieldref_token",
            "mono_image_get_generic_param_info",
            "mono_image_get_method_info",
            "mono_image_get_methodspec_token",
            "mono_image_get_property_info",
            "mono_image_get_type_info",
            "mono_image_walk_resource_tree",
            "mono_metadata_class_equal",
            "mono_metadata_field_info_full",
            "mono_metadata_fnptr_equal",
            "mono_metadata_generic_param_equal_internal",
            "mono_metadata_parse_array_internal",
            "mono_metadata_parse_generic_param",
            "mono_metadata_parse_type_internal",
            "mono_metadata_signature_dup_internal_with_padding",
            "mono_metadata_signature_vararg_match",
            "mono_method_check_inlining",
            "mono_method_get_equivalent_method",
            "mono_method_is_constructor",
            "mono_method_is_valid_generic_instantiation",
            "mono_method_is_valid_in_context",
            "mono_object_new_by_vtable",
            "mono_raise_exception_with_ctx",
            "mono_reflection_get_type_internal",
            "mono_reflection_get_type_internal_dynamic",
            "mono_reflection_get_type_with_rootimage",
            "mono_reflection_method_get_handle",
            "mono_reflection_type_get_underlying_system_type",
            "mono_runtime_capture_context",
            "mono_runtime_delegate_try_invoke_handle",
            "mono_runtime_set_execution_mode",
            "mono_runtime_walk_stack_with_ctx",
            "mono_signature_to_name",
            "mono_string_builder_new",
            "mono_string_from_bstr_common",
            "mono_string_get_pinned",
            "mono_string_is_interned_lookup",
            "mono_string_new_utf32_checked",
            "mono_string_to_utf8_internal",
            "mono_thread_abort",
            "mono_thread_abort_dummy",
            "mono_thread_attach_cb",
            "mono_thread_attach_internal",
            "mono_thread_cleanup_fn",
            "mono_thread_clear_interruption_requested",
            "mono_thread_current_for_thread",
            "mono_thread_detach_internal",
            "mono_thread_execute_interruption",
            "mono_thread_execute_interruption_ptr",
            "mono_thread_execute_interruption_void",
            "mono_thread_get_managed_sp",
            "mono_thread_resume",
            "mono_thread_set_interruption_requested",
            "mono_thread_start_cb",
            "mono_thread_suspend",
            "mono_type_array_get_and_resolve_raw",
            "mono_type_array_get_and_resolve_with_modifiers",
            "mono_type_elements_shift_bits",
            "mono_type_equal",
            "mono_type_from_opcode",
            "mono_type_get_name_recurse",
            "mono_type_get_underlying_type_any",
            "mono_type_hash",
            "mono_type_init_lock",
            "mono_type_initialization_lock",
            "mono_type_is_enum_type",
            "mono_type_is_native_blittable",
            "mono_type_is_valid_in_context",
            "mono_type_is_valid_type_in_context_full",
            "mono_type_normalize",
        };

        private void BuildMsvc(BuildOptions options, TargetPlatform platform, TargetArchitecture architecture, string configuration = "Release")
        {
            string buildPlatform;
            switch (architecture)
            {
            case TargetArchitecture.x86:
                buildPlatform = "Win32";
                break;
            default:
                buildPlatform = architecture.ToString();
                break;
            }

            // Build mono
            Deploy.VCEnvironment.BuildSolution(Path.Combine(root, "msvc", "libmono-static.vcxproj"), configuration, buildPlatform);
            Deploy.VCEnvironment.BuildSolution(Path.Combine(root, "msvc", "monoposixhelper.vcxproj"), configuration, buildPlatform);

            // Deploy binaries
            var binaries = new[]
            {
                Path.Combine("lib", configuration, "libmono-static.lib"),
                Path.Combine("bin", configuration, "MonoPosixHelper.dll"),
            };
            var srcBinaries = Path.Combine(root, "msvc", "build", "sgen", buildPlatform);
            var depsFolder = GetThirdPartyFolder(options, platform, architecture);
            Log.Verbose("Copy mono binaries from " + srcBinaries);
            foreach (var binary in binaries)
            {
                var src = Path.Combine(srcBinaries, binary);
                var dst = Path.Combine(depsFolder, Path.GetFileName(src));
                Utilities.FileCopy(src, dst);
            }

            // Deploy debug symbols
            var debugSymbolsLibs = new[]
            {
                "libmonoruntime",
                "libmonoutils",
                "libgcmonosgen",
                "libmini",
                "eglib",
            };
            foreach (var debugSymbol in debugSymbolsLibs)
            {
                var src = Path.Combine(srcBinaries, "obj", debugSymbol, configuration, debugSymbol + ".pdb");
                var dst = Path.Combine(depsFolder, Path.GetFileName(src));
                Utilities.FileCopy(src, dst);
            }
        }

        private void BuildBcl(BuildOptions options, TargetPlatform platform)
        {
            var configuration = "Release";
            string buildPlatform;
            switch (platform)
            {
            case TargetPlatform.Android:
                buildPlatform = "monodroid";
                break;
            case TargetPlatform.PS4:
                buildPlatform = "orbis";
                break;
            default:
                buildPlatform = "net_4_x";
                break;
            }

            // Build jay
            Deploy.VCEnvironment.BuildSolution(Path.Combine(root, "mcs", "jay", "jay.vcxproj"), "Release", "x64");

            // Build class library
            Utilities.Run(Deploy.VCEnvironment.CscPath, "prepare.cs", null, Path.Combine(root, "msvc", "scripts"));
            Utilities.Run(Path.Combine(root, "msvc", "scripts", "prepare.exe"), "..\\..\\mcs core", null, Path.Combine(root, "msvc", "scripts"));
            Deploy.VCEnvironment.BuildSolution(Path.Combine(root, "bcl.sln"), configuration, buildPlatform);
        }

        private static void ReplaceXmlNodeValue(XmlNode node, string name, string value)
        {
            foreach (XmlNode child in node.ChildNodes)
            {
                if (child.Name == name)
                {
                    child.InnerText = value;
                }

                ReplaceXmlNodeValue(child, name, value);
            }
        }

        protected static string FindXmlNodeValue(XmlNode node, string name)
        {
            foreach (XmlNode child in node.ChildNodes)
            {
                if (child.Name == name)
                {
                    return child.InnerText;
                }

                var value = FindXmlNodeValue(child, name);
                if (value != null)
                    return value;
            }

            return null;
        }

        protected void ConfigureMsvc(BuildOptions options, string vcTools, string winSdkVer, string winVer = "0x0601", string customDefines = null)
        {
            // Patch vcxproj files
            var files = Directory.GetFiles(Path.Combine(root, "msvc"), "*.vcxproj", SearchOption.TopDirectoryOnly);
            foreach (var file in files)
            {
                var projectXml = new XmlDocument();
                projectXml.Load(file);

                ReplaceXmlNodeValue(projectXml, "PlatformToolset", vcTools);
                ReplaceXmlNodeValue(projectXml, "WindowsTargetPlatformVersion", winSdkVer);

                projectXml.Save(file);
            }

            // Patch mono.props
            {
                var defines = monoPreprocesorDefines.Replace("WINVER=0x0601", "WINVER=" + winVer);
                defines = defines.Replace("_WIN32_WINNT=0x0601", "_WIN32_WINNT=" + winVer);
                if (customDefines != null)
                {
                    defines = customDefines + ';' + defines;
                }

                var monoProps = new XmlDocument();
                monoProps.Load(monoPropsPath);

                ReplaceXmlNodeValue(monoProps, "MONO_PREPROCESSOR_DEFINITIONS", defines);

                monoProps.Save(monoPropsPath);
            }
        }

        private static bool EnumSymbols(string name, ulong address, uint size, IntPtr context)
        {
            if (name.StartsWith("mono_") && !monoExports.Contains(name))
            {
                if (MonoExportsIncludePrefixes.Any(name.StartsWith) && !MonoExportsSkipPrefixes.Any(name.StartsWith))
                {
                    monoExports.Add(name);
                }
            }
            return true;
        }

        private void GetMonoExports(BuildOptions options)
        {
            // Search for all exported mono API functions from mono library
            monoExports = new List<string>(2048);
            monoExports.AddRange(MonoExportsCustom);
            IntPtr hCurrentProcess = Process.GetCurrentProcess().Handle;
            bool status = WinAPI.dbghelp.SymInitialize(hCurrentProcess, null, false);
            if (status == false)
            {
                Log.Error("Failed to initialize Sym.");
                return;
            }
            string dllPath = Path.Combine(root, "msvc\\build\\sgen\\x64\\bin\\Release\\mono-2.0.dll");
            ulong baseOfDll = WinAPI.dbghelp.SymLoadModuleEx(hCurrentProcess, IntPtr.Zero, dllPath, null, 0, 0, IntPtr.Zero, 0);
            if (baseOfDll == 0)
            {
                Log.Error($"Failed to load mono library for exports from \'{dllPath}\'.");
                WinAPI.dbghelp.SymCleanup(hCurrentProcess);
                return;
            }
            if (WinAPI.dbghelp.SymEnumerateSymbols64(hCurrentProcess, baseOfDll, EnumSymbols, IntPtr.Zero) == false)
            {
                Log.Error($"Failed to enumerate mono library exports from \'{dllPath}\'.");
            }
            WinAPI.dbghelp.SymCleanup(hCurrentProcess);

            // Make exports list stable
            monoExports.Sort();

            // Generate exports code
            var exports = new StringBuilder(monoExports.Count * 70);
            foreach (var monoExport in monoExports)
            {
                exports.AppendLine(string.Format("#pragma comment(linker, \"/export:{0}\")", monoExport));
            }

            // Update source file with exported symbols
            var mCorePath = Path.Combine(Globals.EngineRoot, "Source", "Engine", "Scripting", "ManagedCLR", "MCore.Mono.cpp");
            var contents = File.ReadAllText(mCorePath);
            var startPos = contents.IndexOf("#pragma comment(linker,");
            var endPos = contents.LastIndexOf("#pragma comment(linker,");
            endPos = contents.IndexOf(')', endPos);
            contents = contents.Remove(startPos, endPos - startPos + 3);
            contents = contents.Insert(startPos, exports.ToString());
            Utilities.WriteFileIfChanged(mCorePath, contents);
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            // Set it to the local path of the mono rep oto use for the build instead of cloning the remote one (helps with rapid testing)
            string localRepoPath = string.Empty;
            //localRepoPath = @"D:\Flax\3rdParty\mono";

            root = options.IntermediateFolder;
            if (!string.IsNullOrEmpty(localRepoPath))
                root = localRepoPath;
            monoPropsPath = Path.Combine(root, "msvc", "mono.props");

            // Get the source
            if (string.IsNullOrEmpty(localRepoPath))
                CloneGitRepo(root, "https://github.com/FlaxEngine/mono.git", null, "--recursive");

            // Pick a proper branch
            GitCheckout(root, "flax-master-5-20");
            GitResetLocalChanges(root);

            // Get the default preprocessor defines for Mono on Windows-based platforms
            {
                var monoProps = new XmlDocument();
                monoProps.Load(monoPropsPath);

                monoPreprocesorDefines = FindXmlNodeValue(monoProps, "MONO_PREPROCESSOR_DEFINITIONS");
            }

            foreach (var platform in options.Platforms)
            {
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    var configuration = "Release";
                    BuildMsvc(options, platform, TargetArchitecture.x64, configuration);
                    //BuildBcl(options, platform);

                    // Export header files
                    Deploy.VCEnvironment.BuildSolution(Path.Combine(root, "msvc", "libmono-dynamic.vcxproj"), configuration, "x64");
                    Deploy.VCEnvironment.BuildSolution(Path.Combine(root, "msvc", "build-install.vcxproj"), configuration, "x64");

                    // Get exported mono methods to forward them in engine module (on Win32 platforms)
                    GetMonoExports(options);

                    if (BuildPlatform == TargetPlatform.Windows)
                    {
                        // Copy header files
                        var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "mono-2.0");
                        var dstMonoIncludePath = Path.Combine(dstIncludePath, "mono");
                        if (Directory.Exists(dstMonoIncludePath))
                            Directory.Delete(dstMonoIncludePath, true);
                        Utilities.DirectoryCopy(Path.Combine(root, "msvc", "include"), dstIncludePath);
                    }

                    break;
                }
                case TargetPlatform.UWP:
                {
                    ConfigureMsvc(options, "v141", "10.0.17763.0", "0x0A00", "_UWP=1;DISABLE_JIT;WINAPI_FAMILY=WINAPI_FAMILY_PC_APP;HAVE_EXTERN_DEFINED_WINAPI_SUPPORT");

                    BuildMsvc(options, platform, TargetArchitecture.x64);

                    break;
                }
                case TargetPlatform.XboxOne:
                {
                    ConfigureMsvc(options, "v141", "10.0.17763.0", "0x0A00", "_XBOX_ONE=1;DISABLE_JIT;WINAPI_FAMILY=WINAPI_FAMILY_PC_APP;HAVE_EXTERN_DEFINED_WINAPI_SUPPORT");

                    BuildMsvc(options, platform, TargetArchitecture.x64);

                    break;
                }
                case TargetPlatform.Linux:
                {
                    var envVars = new Dictionary<string, string>
                    {
                        { "CC", "clang-7" },
                        { "CXX", "clang++-7" }
                    };
                    var binaries = new[]
                    {
                        "lib/libmono-2.0.a",
                    };
                    var buildDir = Path.Combine(root, "build-linux");

                    SetupDirectory(buildDir, true);
                    var archName = UnixToolchain.GetToolchainName(platform, TargetArchitecture.x64);
                    Utilities.Run(Path.Combine(root, "autogen.sh"), string.Format("--host={0} --prefix={1} --disable-boehm --disable-mcs-build --enable-static", archName, buildDir), null, root, Utilities.RunOptions.Default, envVars);
                    Utilities.Run("make", null, null, root, Utilities.RunOptions.None, envVars);
                    Utilities.Run("make", "install", null, root, Utilities.RunOptions.None, envVars);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var binary in binaries)
                    {
                        var src = Path.Combine(buildDir, binary);
                        var dst = Path.Combine(depsFolder, Path.GetFileName(binary));
                        Utilities.FileCopy(src, dst);
                    }
                    break;
                }
                case TargetPlatform.PS4:
                {
                    // TODO: implement automatic extraction of the package from mono-ps4-binaries
                    break;
                }
                case TargetPlatform.XboxScarlett:
                {
                    ConfigureMsvc(options, "v142", "10.0.19041.0", "0x0A00", "_XBOX_ONE=1;DISABLE_JIT;WINAPI_FAMILY=WINAPI_FAMILY_GAMES;HAVE_EXTERN_DEFINED_WINAPI_SUPPORT;CRITICAL_SECTION_NO_DEBUG_INFO=0x01000000");

                    BuildMsvc(options, platform, TargetArchitecture.x64);

                    break;
                }
                case TargetPlatform.Android:
                {
                    var sdk = AndroidSdk.Instance.RootPath;
                    var ndk = AndroidNdk.Instance.RootPath;
                    var apiLevel = Configuration.AndroidPlatformApi.ToString();
                    var archName = UnixToolchain.GetToolchainName(platform, TargetArchitecture.ARM64);
                    var toolchainRoot = Path.Combine(ndk, "toolchains", "llvm", "prebuilt", AndroidSdk.GetHostName());
                    var ndkBin = Path.Combine(toolchainRoot, "bin");
                    var compilerFlags = string.Format("-DANDROID -DMONODROID=1 -DANDROID64 -D__ANDROID_API__={0} --sysroot=\"{1}/sysroot\" --gcc-toolchain=\"{1}\"", apiLevel, toolchainRoot);
                    var envVars = new Dictionary<string, string>
                    {
                        { "ANDROID_SDK_ROOT", sdk },
                        { "ANDROID_SDK", sdk },
                        { "ANDROID_NDK_ROOT", ndk },
                        { "ANDROID_NDK", ndk },
                        { "NDK", ndk },
                        { "NDK_BIN", ndkBin },
                        { "ANDROID_PLATFORM", "android-" + apiLevel },
                        { "ANDROID_API", apiLevel },
                        { "ANDROID_API_VERSION", apiLevel },
                        { "ANDROID_NATIVE_API_LEVEL", apiLevel },
                        
                        { "CC", Path.Combine(ndkBin, archName + apiLevel + "-clang") },
                        { "CXX", Path.Combine(ndkBin, archName + apiLevel + "-clang++") },
                        { "AR", Path.Combine(ndkBin, archName + "-ar") },
                        { "AS", Path.Combine(ndkBin, archName + "-as") },
                        { "LD", Path.Combine(ndkBin, archName + "-ld") },
                        { "RANLIB", Path.Combine(ndkBin, archName + "-ranlib") },
                        { "STRIP", Path.Combine(ndkBin, archName + "-strip") },
                        { "SYSROOT", toolchainRoot },
                        { "CFLAGS", compilerFlags },
                        { "CXXFLAGS", compilerFlags },
                        { "CPPFLAGS", compilerFlags },
                    };
                    var monoOptions = new[]
                    {
                        "--disable-crash-reporting",
                        "--disable-executables",
                        "--disable-iconv",
                        "--disable-boehm",
                        "--disable-nls",
                        "--disable-mcs-build",
                        "--enable-maintainer-mode",
                        "--enable-dynamic-btls",
                        "--enable-monodroid",
                        "--with-btls-android-ndk",
                        "--with-sigaltstack=yes",
                        string.Format("--with-btls-android-ndk={0}", ndk),
                        string.Format("--with-btls-android-api={0}", apiLevel),
                        string.Format("--with-btls-android-cmake-toolchain={0}/build/cmake/android.toolchain.cmake", ndk),
                        "--without-ikvm-native",
                    };
                    var binaries = new[]
                    {
                        "lib/libmonosgen-2.0.so",
                    };
                    var buildDir = Path.Combine(root, "build-android");
                    var envArgs = "";
                    foreach (var e in envVars)
                    {
                        if (e.Value.Contains(' '))
                            envArgs += $" \"{e.Key}={e.Value}\"";
                        else
                            envArgs += $" {e.Key}={e.Value}";
                    }

                    // Compile mono
                    SetupDirectory(buildDir, true);
                    var toolchain = UnixToolchain.GetToolchainName(platform, TargetArchitecture.ARM64);
                    Utilities.Run(Path.Combine(root, "autogen.sh"), string.Format("--host={0} --prefix={1} {2}{3}", toolchain, buildDir, string.Join(" ", monoOptions), envArgs), null, root, Utilities.RunOptions.Default, envVars);
                    Utilities.Run("make", null, null, root, Utilities.RunOptions.None, envVars);
                    Utilities.Run("make", "install", null, root, Utilities.RunOptions.None, envVars);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.ARM64);
                    foreach (var binary in binaries)
                    {
                        var src = Path.Combine(buildDir, binary);
                        var dst = Path.Combine(depsFolder, Path.GetFileName(binary));
                        Utilities.FileCopy(src, dst);
                    }

                    // Clean before another build
                    GitResetLocalChanges(root);
                    Utilities.Run("make", "distclean", null, root, Utilities.RunOptions.None);

                    // Compile BCL
                    var installBcl = Path.Combine(root, "bcl-android");
                    var bclOutput = Path.Combine(GetBinariesFolder(options, platform), "Mono");
                    var bclLibOutput = Path.Combine(bclOutput, "lib");
                    var bclLibMonoOutput = Path.Combine(bclLibOutput, "mono");
                    SetupDirectory(installBcl, true);
                    SetupDirectory(bclOutput, true);
                    SetupDirectory(bclLibOutput, false);
                    SetupDirectory(bclLibMonoOutput, false);
                    Utilities.DirectoryCopy(Path.Combine(buildDir, "etc"), Path.Combine(bclOutput, "etc"), true, true);
                    Utilities.FileCopy(Path.Combine(buildDir, "lib", "libMonoPosixHelper.so"), Path.Combine(bclLibOutput, "libMonoPosixHelper.so"));
                    Utilities.FileCopy(Path.Combine(buildDir, "lib", "libmono-native.so"), Path.Combine(bclLibOutput, "libmono-native.so"));
                    Utilities.FileCopy(Path.Combine(buildDir, "lib", "libmono-btls-shared.so"), Path.Combine(bclLibOutput, "libmono-btls-shared.so"));
                    var bclOptions = new[]
                    {
                        "--disable-boehm",
                        "--disable-btls-lib",
                        "--disable-nls",
                        "--disable-support-build",
                        "--with-mcs-docs=no",
                    };
                    Utilities.Run(Path.Combine(root, "autogen.sh"), string.Format("--prefix={0} {1}", installBcl, string.Join(" ", bclOptions)), null, root);
                    Utilities.Run("make", $"-j1 -C {root} -C mono", null, root, Utilities.RunOptions.None);
                    Utilities.Run("make", $"-j2 -C {root} -C runtime all-mcs build_profiles=monodroid", null, root, Utilities.RunOptions.None);
                    Utilities.DirectoryCopy(Path.Combine(root, "mcs", "class", "lib", "monodroid"), Path.Combine(bclLibMonoOutput, "2.1"), true, true, "*.dll");
                    Utilities.DirectoryDelete(Path.Combine(bclLibMonoOutput, "2.1", "Facades"));
                    break;
                }
                }
            }
        }
    }
}
