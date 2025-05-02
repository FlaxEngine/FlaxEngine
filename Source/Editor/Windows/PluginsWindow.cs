// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Tabs;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Editor tool window for plugins management using <see cref="PluginManager"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public sealed class PluginsWindow : EditorWindow
    {
        private Tabs _tabs;
        private Button _addPluginProjectButton;
        private Button _cloneProjectButton;
        private readonly List<CategoryEntry> _categories = new List<CategoryEntry>();
        private readonly Dictionary<Plugin, PluginEntry> _entries = new Dictionary<Plugin, PluginEntry>();

        /// <summary>
        /// Plugin entry control.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
        public class PluginEntry : ContainerControl
        {
            /// <summary>
            /// The plugin.
            /// </summary>
            public readonly Plugin Plugin;

            /// <summary>
            /// The category.
            /// </summary>
            public readonly CategoryEntry Category;

            /// <summary>
            /// Initializes a new instance of the <see cref="PluginEntry"/> class.
            /// </summary>
            /// <param name="plugin">The plugin.</param>
            /// <param name="category">The category.</param>
            /// <param name="desc">Plugin description</param>
            public PluginEntry(Plugin plugin, CategoryEntry category, ref PluginDescription desc)
            {
                Plugin = plugin;
                Category = category;

                float margin = 4;
                float iconSize = 64;

                var iconImage = new Image
                {
                    Brush = new SpriteBrush(Editor.Instance.Icons.Plugin128),
                    Parent = this,
                    Bounds = new Rectangle(margin, margin, iconSize, iconSize),
                };

                var icon = PluginUtils.TryGetPluginIcon(plugin);
                if (icon)
                    iconImage.Brush = new TextureBrush(icon);

                Size = new Float2(300, 100);

                float tmp1 = iconImage.Right + margin;
                var nameLabel = new Label
                {
                    HorizontalAlignment = TextAlignment.Near,
                    AnchorPreset = AnchorPresets.HorizontalStretchTop,
                    Text = desc.Name,
                    Font = new FontReference(Style.Current.FontLarge),
                    Parent = this,
                    Bounds = new Rectangle(tmp1, margin, Width - tmp1 - margin, 28),
                };

                tmp1 = nameLabel.Bottom + margin + 8;
                var descLabel = new Label
                {
                    HorizontalAlignment = TextAlignment.Near,
                    VerticalAlignment = TextAlignment.Near,
                    Wrapping = TextWrapping.WrapWords,
                    AnchorPreset = AnchorPresets.HorizontalStretchTop,
                    Text = desc.Description,
                    Parent = this,
                    Bounds = new Rectangle(nameLabel.X, tmp1, nameLabel.Width, Height - tmp1 - margin),
                };

                var xOffset = nameLabel.Width;
                string versionString = string.Empty;
                if (desc.IsAlpha)
                    versionString = "ALPHA ";
                else if (desc.IsBeta)
                    versionString = "BETA ";
                versionString += "Version ";
                versionString += desc.Version != null ? desc.Version.ToString() : "1.0";
                var versionLabel = new Label
                {
                    HorizontalAlignment = TextAlignment.Far,
                    VerticalAlignment = TextAlignment.Center,
                    AnchorPreset = AnchorPresets.TopRight,
                    Text = versionString,
                    Parent = this,
                    Bounds = new Rectangle(Width - 140 - margin, margin, 140, 14),
                };

                string url = null;
                if (!string.IsNullOrEmpty(desc.AuthorUrl))
                    url = desc.AuthorUrl;
                else if (!string.IsNullOrEmpty(desc.HomepageUrl))
                    url = desc.HomepageUrl;
                else if (!string.IsNullOrEmpty(desc.RepositoryUrl))
                    url = desc.RepositoryUrl;
                versionLabel.Font.Font.WaitForLoaded();
                var font = versionLabel.Font.GetFont();
                var authorWidth = font.MeasureText(desc.Author).X + 8;
                var authorLabel = new ClickableLabel
                {
                    HorizontalAlignment = TextAlignment.Far,
                    VerticalAlignment = TextAlignment.Center,
                    AnchorPreset = AnchorPresets.TopRight,
                    Text = desc.Author,
                    Parent = this,
                    Bounds = new Rectangle(Width - authorWidth - margin, versionLabel.Bottom + margin, authorWidth, 14),
                };
                if (url != null)
                {
                    authorLabel.TextColorHighlighted = Style.Current.BackgroundSelected;
                    authorLabel.DoubleClick = () => Platform.OpenUrl(url);
                }
            }
        }

        /// <summary>
        /// Plugins category control.
        /// </summary>
        /// <seealso cref="Tab" />
        public class CategoryEntry : Tab
        {
            /// <summary>
            /// The panel for the plugin entries.
            /// </summary>
            public VerticalPanel Panel;

            /// <summary>
            /// Initializes a new instance of the <see cref="CategoryEntry"/> class.
            /// </summary>
            /// <param name="text">The text.</param>
            public CategoryEntry(string text)
            : base(text)
            {
                var scroll = new Panel(ScrollBars.Vertical)
                {
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                    Pivot = Float2.Zero,
                    Parent = this,
                };
                var panel = new VerticalPanel
                {
                    AnchorPreset = AnchorPresets.HorizontalStretchTop,
                    Pivot = Float2.Zero,
                    Offsets = Margin.Zero,
                    IsScrollable = true,
                    Parent = scroll,
                };
                Panel = panel;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="PluginsWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public PluginsWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Plugins";

            var vp = new Panel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this,
            };
            _addPluginProjectButton = new Button
            {
                Text = "Create Project",
                TooltipText = "Add new plugin project.",
                AnchorPreset = AnchorPresets.TopLeft,
                LocalLocation = new Float2(70, 18),
                Size = new Float2(150, 25),
                Parent = vp,
            };
            _addPluginProjectButton.Clicked += OnAddButtonClicked;

            _cloneProjectButton = new Button
            {
                Text = "Clone Project",
                TooltipText = "Git Clone a plugin project.",
                AnchorPreset = AnchorPresets.TopLeft,
                LocalLocation = new Float2(70 + _addPluginProjectButton.Size.X + 8, 18),
                Size = new Float2(150, 25),
                Parent = vp,
            };
            _cloneProjectButton.Clicked += OnCloneProjectButtonClicked;

            _tabs = new Tabs
            {
                Orientation = Orientation.Vertical,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _addPluginProjectButton.Bottom + 8, 0),
                TabsSize = new Float2(120, 32),
                Parent = vp
            };

            OnPluginsChanged();
            PluginManager.PluginsChanged += OnPluginsChanged;
        }

        private void OnCloneProjectButtonClicked()
        {
            var popup = new ContextMenuBase
            {
                Size = new Float2(300, 125),
                ClipChildren = false,
                CullChildren = false,
            };
            popup.Show(_cloneProjectButton, new Float2(_cloneProjectButton.Width, 0));

            var nameLabel = new Label
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                AutoWidth = true,
                Text = "Name",
                HorizontalAlignment = TextAlignment.Near,
            };
            nameLabel.LocalX -= 10;
            nameLabel.LocalY += 10;

            var nameTextBox = new TextBox
            {
                Parent = popup,
                WatermarkText = "Plugin Name",
                TooltipText = "If left blank, this will take the git name.",
                AnchorPreset = AnchorPresets.TopLeft,
                IsMultiline = false,
            };
            nameTextBox.LocalX += (300 - (10)) * 0.5f;
            nameTextBox.LocalY += 10;
            nameLabel.LocalX += (300 - (nameLabel.Width + nameTextBox.Width)) * 0.5f + 10;

            var defaultTextBoxBorderColor = nameTextBox.BorderColor;
            var defaultTextBoxBorderSelectedColor = nameTextBox.BorderSelectedColor;
            nameTextBox.TextChanged += () =>
            {
                if (string.IsNullOrEmpty(nameTextBox.Text))
                {
                    nameTextBox.BorderColor = defaultTextBoxBorderColor;
                    nameTextBox.BorderSelectedColor = defaultTextBoxBorderSelectedColor;
                    return;
                }

                var pluginPath = Path.Combine(Globals.ProjectFolder, "Plugins", nameTextBox.Text);
                if (Directory.Exists(pluginPath))
                {
                    nameTextBox.BorderColor = Color.Red;
                    nameTextBox.BorderSelectedColor = Color.Red;
                }
                else
                {
                    nameTextBox.BorderColor = defaultTextBoxBorderColor;
                    nameTextBox.BorderSelectedColor = defaultTextBoxBorderSelectedColor;
                }
            };

            var gitPathLabel = new Label
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                AutoWidth = true,
                Text = "Git Path",
                HorizontalAlignment = TextAlignment.Near,
            };
            gitPathLabel.LocalX += (250 - gitPathLabel.Width) * 0.5f;
            gitPathLabel.LocalY += 35;

            var gitPathTextBox = new TextBox
            {
                Parent = popup,
                WatermarkText = "https://github.com/FlaxEngine/ExamplePlugin.git",
                AnchorPreset = AnchorPresets.TopLeft,
                Size = new Float2(280, TextBox.DefaultHeight),
                IsMultiline = false,
            };
            gitPathTextBox.LocalY += 60;
            gitPathTextBox.LocalX += 10;

            var submitButton = new Button
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Clone",
                Width = 70,
            };
            submitButton.LocalX += 300 * 0.5f - submitButton.Width - 10;
            submitButton.LocalY += 90;

            submitButton.Clicked += () =>
            {
                if (Directory.Exists(Path.Combine(Globals.ProjectFolder, "Plugins", nameTextBox.Text)) && !string.IsNullOrEmpty(nameTextBox.Text))
                {
                    Editor.LogWarning("Cannot create plugin due to name conflict.");
                    return;
                }
                OnCloneButtonClicked(nameTextBox.Text, gitPathTextBox.Text);
                nameTextBox.Clear();
                gitPathTextBox.Clear();
                popup.Hide();
            };

            var cancelButton = new Button
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Cancel",
                Width = 70,
            };
            cancelButton.LocalX += 300 * 0.5f + 10;
            cancelButton.LocalY += 90;

            cancelButton.Clicked += () =>
            {
                nameTextBox.Clear();
                gitPathTextBox.Clear();
                popup.Hide();
            };
        }

        private async void OnCloneButtonClicked(string pluginName, string gitPath)
        {
            if (string.IsNullOrEmpty(gitPath))
            {
                Editor.LogError("Failed to create plugin project due to no GIT path.");
                return;
            }
            if (string.IsNullOrEmpty(pluginName))
            {
                var split = gitPath.Split('/');
                if (string.IsNullOrEmpty(split[^1]))
                {
                    var name = split[^2].Replace(".git", "");
                    pluginName = name;
                }
                else
                {
                    var name = split[^1].Replace(".git", "");
                    pluginName = name;
                }
            }

            var clonePath = Path.Combine(Globals.ProjectFolder, "Plugins", pluginName);
            if (Directory.Exists(clonePath))
            {
                Editor.LogError("Plugin Name is already used. Pick a different Name.");
                return;
            }
            Directory.CreateDirectory(clonePath);
            
            try
            {
                // Start git clone
                var settings = new CreateProcessSettings
                {
                    FileName = "git",
                    Arguments = $"clone {gitPath} \"{clonePath}\"",
                    ShellExecute = false,
                    LogOutput = true,
                    WaitForEnd = true
                };
                var asSubmodule = Directory.Exists(Path.Combine(Globals.ProjectFolder, ".git"));
                if (asSubmodule)
                {
                    // Clone as submodule to the existing repo
                    settings.Arguments = $"submodule add {gitPath} \"Plugins/{pluginName}\"";
                    
                    // Submodule add need the target folder to not exist
                    Directory.Delete(clonePath);
                }
                int result = Platform.CreateProcess(ref settings);
                if (result != 0)
                    throw new Exception($"'{settings.FileName} {settings.Arguments}' failed with result {result}");

                // Ensure that cloned repo exists
                var checkPath = Path.Combine(clonePath, ".git");
                if (asSubmodule)
                {
                    if (!File.Exists(checkPath))
                        throw new Exception("Failed to clone repo.");
                }
                else
                {
                    if (!Directory.Exists(checkPath))
                        throw new Exception("Failed to clone repo.");

                }
            }
            catch (Exception e)
            {
                Editor.LogError($"Failed Git process. {e}");
                return;
            }

            Editor.Log("Plugin project has been cloned.");

            try
            {
                // Start git submodule clone
                var settings = new CreateProcessSettings
                {
                    FileName = "git",
                    WorkingDirectory = clonePath,
                    Arguments = "submodule update --init",
                    ShellExecute = false,
                    LogOutput = true,
                    WaitForEnd = true
                };
                Platform.CreateProcess(ref settings);
            }
            catch (Exception e)
            {
                Editor.LogError($"Failed Git submodule process. {e}");
                return;
            }

            // Find project config file. Could be different then what the user named the folder.
            string pluginProjectName = "";
            foreach (var file in Directory.GetFiles(clonePath))
            {
                if (file.Contains(".flaxproj", StringComparison.OrdinalIgnoreCase))
                {
                    pluginProjectName = Path.GetFileNameWithoutExtension(file);
                    break;
                }
            }
            if (string.IsNullOrEmpty(pluginProjectName))
            {
                Editor.LogError("Failed to find plugin project file to add to Project config. Please add manually.");
                return;
            }

            await AddModuleReferencesInGameModule(clonePath);
            await AddReferenceToProject(pluginName, pluginProjectName);

            if (Editor.Options.Options.SourceCode.AutoGenerateScriptsProjectFiles)
                Editor.ProgressReporting.GenerateScriptsProjectFiles.RunAsync();

            MessageBox.Show($"{pluginName} has been successfully cloned. Restart editor for changes to take effect.", "Plugin Project Created", MessageBoxButtons.OK);
        }

        private void OnAddButtonClicked()
        {
            var popup = new ContextMenuBase
            {
                Size = new Float2(230, 125),
                ClipChildren = false,
                CullChildren = false,
            };
            popup.Show(_addPluginProjectButton, new Float2(_addPluginProjectButton.Width, 0));

            var nameLabel = new Label
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Name",
                HorizontalAlignment = TextAlignment.Near,
            };
            nameLabel.LocalX += 10;
            nameLabel.LocalY += 10;

            var nameTextBox = new TextBox
            {
                Parent = popup,
                WatermarkText = "Plugin Name",
                AnchorPreset = AnchorPresets.TopLeft,
                IsMultiline = false,
            };
            nameTextBox.LocalX += 100;
            nameTextBox.LocalY += 10;
            var defaultTextBoxBorderColor = nameTextBox.BorderColor;
            var defaultTextBoxBorderSelectedColor = nameTextBox.BorderSelectedColor;
            nameTextBox.TextChanged += () =>
            {
                if (string.IsNullOrEmpty(nameTextBox.Text))
                {
                    nameTextBox.BorderColor = defaultTextBoxBorderColor;
                    nameTextBox.BorderSelectedColor = defaultTextBoxBorderSelectedColor;
                    return;
                }

                var pluginPath = Path.Combine(Globals.ProjectFolder, "Plugins", nameTextBox.Text);
                if (Directory.Exists(pluginPath))
                {
                    nameTextBox.BorderColor = Color.Red;
                    nameTextBox.BorderSelectedColor = Color.Red;
                }
                else
                {
                    nameTextBox.BorderColor = defaultTextBoxBorderColor;
                    nameTextBox.BorderSelectedColor = defaultTextBoxBorderSelectedColor;
                }
            };

            var versionLabel = new Label
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Version",
                HorizontalAlignment = TextAlignment.Near,
            };
            versionLabel.LocalX += 10;
            versionLabel.LocalY += 35;

            var versionTextBox = new TextBox
            {
                Parent = popup,
                WatermarkText = "1.0.0",
                AnchorPreset = AnchorPresets.TopLeft,
                IsMultiline = false,
            };
            versionTextBox.LocalY += 35;
            versionTextBox.LocalX += 100;

            var companyLabel = new Label
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Company",
                HorizontalAlignment = TextAlignment.Near,
            };
            companyLabel.LocalX += 10;
            companyLabel.LocalY += 60;

            var companyTextBox = new TextBox
            {
                Parent = popup,
                WatermarkText = "Company Name",
                AnchorPreset = AnchorPresets.TopLeft,
                IsMultiline = false,
            };
            companyTextBox.LocalY += 60;
            companyTextBox.LocalX += 100;

            var submitButton = new Button
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Create",
                Width = 70,
            };
            submitButton.LocalX += 40;
            submitButton.LocalY += 90;

            submitButton.Clicked += () =>
            {
                if (Directory.Exists(Path.Combine(Globals.ProjectFolder, "Plugins", nameTextBox.Text)))
                {
                    Editor.LogWarning("Cannot create plugin due to name conflict.");
                    return;
                }
                OnCreateButtonClicked(nameTextBox.Text, versionTextBox.Text, companyTextBox.Text);
                nameTextBox.Clear();
                versionTextBox.Clear();
                companyTextBox.Clear();
                popup.Hide();
            };

            var cancelButton = new Button
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Cancel",
                Width = 70,
            };
            cancelButton.LocalX += 120;
            cancelButton.LocalY += 90;

            cancelButton.Clicked += () =>
            {
                nameTextBox.Clear();
                versionTextBox.Clear();
                companyTextBox.Clear();
                popup.Hide();
            };
        }

        private async void OnCreateButtonClicked(string pluginName, string pluginVersion, string companyName)
        {
            if (string.IsNullOrEmpty(pluginName))
            {
                Editor.LogError("Failed to create plugin project due to no plugin name.");
                return;
            }

            var templateUrl = "https://github.com/FlaxEngine/ExamplePlugin/archive/refs/heads/master.zip";
            var localTemplateFolderLocation = Path.Combine(Editor.LocalCachePath, "TemplatePluginCache");
            if (!Directory.Exists(localTemplateFolderLocation))
                Directory.CreateDirectory(localTemplateFolderLocation);
            var localTemplatePath = Path.Combine(localTemplateFolderLocation, "TemplatePlugin.zip");

            try
            {
                // Download example plugin
                using (var client = new HttpClient())
                {
                    byte[] zipBytes = await client.GetByteArrayAsync(templateUrl);
                    await File.WriteAllBytesAsync(!File.Exists(localTemplatePath) ? Path.Combine(localTemplatePath) : Path.Combine(Editor.LocalCachePath, "TemplatePluginCache", "TemplatePlugin1.zip"), zipBytes);

                    Editor.Log("Template for plugin project has downloaded");
                }
            }
            catch (Exception e)
            {
                Editor.LogError($"Failed to download template project. Trying to use local file. {e}");
                if (!File.Exists(localTemplatePath))
                {
                    Editor.LogError("Failed to use local file. Does not exist.");
                    return;
                }
            }

            // Check if any changes in new downloaded file
            if (File.Exists(Path.Combine(Editor.LocalCachePath, "TemplatePluginCache", "TemplatePlugin1.zip")))
            {
                var localTemplatePath2 = Path.Combine(Editor.LocalCachePath, "TemplatePluginCache", "TemplatePlugin1.zip");
                bool areDifferent = false;
                using (var zip1 = ZipFile.OpenRead(localTemplatePath))
                {
                    using (var zip2 = ZipFile.OpenRead(localTemplatePath2))
                    {
                        if (zip1.Entries.Count != zip2.Entries.Count)
                        {
                            areDifferent = true;
                        }

                        foreach (ZipArchiveEntry entry1 in zip1.Entries)
                        {
                            ZipArchiveEntry entry2 = zip2.GetEntry(entry1.FullName);
                            if (entry2 == null)
                            {
                                areDifferent = true;
                                break;
                            }
                            if (entry1.Length != entry2.Length || entry1.CompressedLength != entry2.CompressedLength || entry1.Crc32 != entry2.Crc32)
                            {
                                areDifferent = true;
                                break;
                            }
                        }
                    }
                }
                if (areDifferent)
                {
                    File.Delete(localTemplatePath);
                    File.Move(localTemplatePath2, localTemplatePath);
                }
                else
                {
                    File.Delete(localTemplatePath2);
                }
            }

            var extractPath = Path.Combine(Globals.ProjectFolder, "Plugins");
            if (!Directory.Exists(extractPath))
                Directory.CreateDirectory(extractPath);

            try
            {
                await Task.Run(() => ZipFile.ExtractToDirectory(localTemplatePath, extractPath));
                Editor.Log("Template for plugin project successfully moved to project.");
            }
            catch (IOException e)
            {
                Editor.LogError($"Failed to add plugin to project. {e}");
            }

            // Format plugin name into a valid name for code (C#/C++ typename)
            var pluginCodeName = Content.ScriptItem.FilterScriptName(pluginName);
            if (string.IsNullOrEmpty(pluginCodeName))
                pluginCodeName = "MyPlugin";
            Editor.Log($"Using plugin code type name: {pluginCodeName}");

            var oldPluginPath = Path.Combine(extractPath, "ExamplePlugin-master");
            var newPluginPath = Path.Combine(extractPath, pluginCodeName);
            Directory.Move(oldPluginPath, newPluginPath);

            var oldFlaxProjFile = Path.Combine(newPluginPath, "ExamplePlugin.flaxproj");
            var newFlaxProjFile = Path.Combine(newPluginPath, $"{pluginCodeName}.flaxproj");
            File.Move(oldFlaxProjFile, newFlaxProjFile);

            var readme = Path.Combine(newPluginPath, "README.md");
            if (File.Exists(readme))
                File.Delete(readme);
            var license = Path.Combine(newPluginPath, "LICENSE");
            if (File.Exists(license))
                File.Delete(license);

            // Flax plugin project file
            var flaxPluginProjContents = JsonSerializer.Deserialize<ProjectInfo>(await File.ReadAllTextAsync(newFlaxProjFile));
            flaxPluginProjContents.Name = pluginCodeName;
            if (!string.IsNullOrEmpty(pluginVersion))
                flaxPluginProjContents.Version = new Version(pluginVersion);
            if (!string.IsNullOrEmpty(companyName))
                flaxPluginProjContents.Company = companyName;
            flaxPluginProjContents.GameTarget = $"{pluginCodeName}Target";
            flaxPluginProjContents.EditorTarget = $"{pluginCodeName}EditorTarget";
            await File.WriteAllTextAsync(newFlaxProjFile, JsonSerializer.Serialize(flaxPluginProjContents, typeof(ProjectInfo)), Encoding.UTF8);

            // Format game settings
            var gameSettingsPath = Path.Combine(newPluginPath, "Content", "GameSettings.json");
            if (File.Exists(gameSettingsPath))
            {
                var contents = await File.ReadAllTextAsync(gameSettingsPath);
                contents = contents.Replace("Example Plugin", pluginName);
                contents = contents.Replace("\"CompanyName\": \"Flax\"", $"\"CompanyName\": \"{companyName}\"");
                contents = contents.Replace("1.0", pluginVersion);
                await File.WriteAllTextAsync(gameSettingsPath, contents, Encoding.UTF8);
            }

            // Rename source directories
            var sourcePath = Path.Combine(newPluginPath, "Source");
            var sourceDirectories = Directory.GetDirectories(sourcePath);
            foreach (var directory in sourceDirectories)
            {
                var files = Directory.GetFiles(directory);
                foreach (var file in files)
                {
                    if (file.Contains("MyPluginEditor.cs"))
                    {
                        File.Delete(file);
                        continue;
                    }

                    var fileName = Path.GetFileName(file).Replace("ExamplePlugin", pluginCodeName);
                    var fileText = await File.ReadAllTextAsync(file);
                    fileText = fileText.Replace("ExamplePlugin", pluginCodeName);
                    if (file.Contains("MyPlugin.cs"))
                    {
                        fileName = "ExamplePlugin.cs";
                        fileText = fileText.Replace("MyPlugin", pluginCodeName);
                        fileText = fileText.Replace("My Plugin", pluginName);
                        fileText = fileText.Replace("Flax Engine", companyName);
                        fileText = fileText.Replace("new Version(1, 0)", $"new Version({pluginVersion.Trim().Replace(".", ", ")})");
                    }
                    await File.WriteAllTextAsync(file, fileText);
                    File.Move(file, Path.Combine(directory, fileName));
                }

                var newName = directory.Replace("ExamplePlugin", pluginCodeName);
                Directory.Move(directory, newName);
            }

            // Rename targets
            var targetFiles = Directory.GetFiles(sourcePath);
            foreach (var file in targetFiles)
            {
                var fileText = await File.ReadAllTextAsync(file);
                await File.WriteAllTextAsync(file, fileText.Replace("ExamplePlugin", pluginCodeName), Encoding.UTF8);
                var newName = file.Replace("ExamplePlugin", pluginCodeName);
                File.Move(file, newName);
            }
            Editor.Log($"Plugin project {pluginName} has successfully been created.");

            await AddReferenceToProject(pluginCodeName, pluginCodeName);
            MessageBox.Show($"{pluginName} has been successfully created. Restart editor for changes to take effect.", "Plugin Project Created", MessageBoxButtons.OK);
        }

        private async Task AddModuleReferencesInGameModule(string pluginFolderPath)
        {
            // Common game build script location
            var gameScript = Path.Combine(Globals.ProjectFolder, "Source/Game/Game.Build.cs");
            if (File.Exists(gameScript))
            {
                var gameScriptContents = await File.ReadAllTextAsync(gameScript);
                var insertLocation = gameScriptContents.IndexOf("base.Setup(options);", StringComparison.Ordinal);
                if (insertLocation != -1)
                {
                    insertLocation += 20;
                    var modifiedAny = false;

                    // Find all code modules in a plugin to auto-reference them in game build script
                    foreach (var subDir in Directory.GetDirectories(Path.Combine(pluginFolderPath, "Source")))
                    {
                        var pluginModuleName = Path.GetFileName(subDir);
                        var pluginModuleScriptPath = Path.Combine(subDir, pluginModuleName + ".Build.cs");
                        if (File.Exists(pluginModuleScriptPath))
                        {
                            var text = await File.ReadAllTextAsync(pluginModuleScriptPath);
                            if (!text.Contains("GameEditorModule", StringComparison.OrdinalIgnoreCase))
                            {
                                gameScriptContents = gameScriptContents.Insert(insertLocation, $"\n        options.PublicDependencies.Add(\"{pluginModuleName}\");");
                                modifiedAny = true;
                            }
                        }
                    }

                    if (modifiedAny)
                        await File.WriteAllTextAsync(gameScript, gameScriptContents, Encoding.UTF8);
                }
            }
        }

        private async Task AddReferenceToProject(string pluginFolderName, string pluginName)
        {
            // Project flax config file
            var flaxProjPath = Editor.GameProject.ProjectPath;
            if (File.Exists(flaxProjPath))
            {
                var flaxProjContents = JsonSerializer.Deserialize<ProjectInfo>(await File.ReadAllTextAsync(flaxProjPath));
                var oldReferences = flaxProjContents.References;
                var references = new List<ProjectInfo.Reference>(oldReferences);
                var newPath = $"$(ProjectPath)/Plugins/{pluginFolderName}/{pluginName}.flaxproj";
                if (!references.Exists(x => string.Equals(x.Name, newPath)))
                {
                    var newReference = new ProjectInfo.Reference
                    {
                        Name = newPath,
                    };
                    references.Add(newReference);
                }
                flaxProjContents.References = references.ToArray();
                await File.WriteAllTextAsync(flaxProjPath, JsonSerializer.Serialize(flaxProjContents, typeof(ProjectInfo)), Encoding.UTF8);
            }
        }

        private void OnPluginsChanged()
        {
            List<PluginEntry> toRemove = null;
            var gamePlugins = PluginManager.GamePlugins;
            var editorPlugins = PluginManager.EditorPlugins;
            foreach (var e in _entries)
            {
                if (!editorPlugins.Contains(e.Key) && !gamePlugins.Contains(e.Key))
                {
                    if (toRemove == null)
                        toRemove = new List<PluginEntry>();
                    toRemove.Add(e.Value);
                }
            }

            if (toRemove != null)
            {
                foreach (var plugin in toRemove)
                    OnPluginRemove(plugin);
            }

            foreach (var plugin in gamePlugins)
                OnPluginAdd(plugin);
            foreach (var plugin in editorPlugins)
                OnPluginAdd(plugin);
        }

        private void OnPluginAdd(Plugin plugin)
        {
            var entry = GetPluginEntry(plugin);
            if (entry != null)
                return;

            // Special case for editor plugins (merge with game plugin if has linked)
            if (plugin is EditorPlugin editorPlugin && GetPluginEntry(editorPlugin.GamePluginType) != null)
                return;

            // Special case for game plugins (merge with editor plugin if has linked)
            if (plugin is GamePlugin)
            {
                foreach (var e in _entries.Keys)
                {
                    if (e is EditorPlugin ee && ee.GamePluginType == plugin.GetType())
                        return;
                }
            }

            var desc = plugin.Description;
            var category = _categories.Find(x => string.Equals(x.Text, desc.Category, StringComparison.OrdinalIgnoreCase));
            if (category == null)
            {
                category = new CategoryEntry(desc.Category)
                {
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                    Parent = _tabs
                };
                _categories.Add(category);
                category.UnlockChildrenRecursive();
            }

            entry = new PluginEntry(plugin, category, ref desc)
            {
                Parent = category.Panel,
            };
            _entries.Add(plugin, entry);
            entry.UnlockChildrenRecursive();

            if (_tabs.SelectedTab == null)
                _tabs.SelectedTab = category;
        }

        private void OnPluginRemove(PluginEntry entry)
        {
            var category = entry.Category;
            _entries.Remove(entry.Plugin);
            entry.Dispose();

            // If category is not used anymore
            if (_entries.Values.Count(x => x.Category == category) == 0)
            {
                if (_tabs.SelectedTab == category)
                    _tabs.SelectedTab = null;

                category.Dispose();
                _categories.Remove(category);

                if (_tabs.SelectedTab == null && _categories.Count > 0)
                    _tabs.SelectedTab = _categories[0];
            }
        }

        /// <summary>
        /// Gets the plugin entry control.
        /// </summary>
        /// <param name="plugin">The plugin.</param>
        /// <returns>The entry.</returns>
        public PluginEntry GetPluginEntry(Plugin plugin)
        {
            _entries.TryGetValue(plugin, out var e);
            return e;
        }

        /// <summary>
        /// Gets the plugin entry control.
        /// </summary>
        /// <param name="pluginType">The plugin type.</param>
        /// <returns>The entry.</returns>
        public PluginEntry GetPluginEntry(Type pluginType)
        {
            if (pluginType == null)
                return null;
            foreach (var e in _entries.Keys)
            {
                if (e.GetType() == pluginType && _entries.ContainsKey(e))
                    return _entries[e];
            }
            return null;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_addPluginProjectButton != null)
                _addPluginProjectButton.Clicked -= OnAddButtonClicked;
            if (_cloneProjectButton != null)
                _cloneProjectButton.Clicked -= OnCloneProjectButtonClicked;
            PluginManager.PluginsChanged -= OnPluginsChanged;

            base.OnDestroy();
        }
    }
}
