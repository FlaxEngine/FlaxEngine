// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.Content.Settings;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Manages assets database and searches for workspace directory changes.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class ContentDatabaseModule : EditorModule
    {
        private bool _enableEvents;
        private bool _isDuringFastSetup;
        private int _itemsCreated;
        private int _itemsDeleted;
        private readonly HashSet<MainContentTreeNode> _dirtyNodes = new HashSet<MainContentTreeNode>();

        /// <summary>
        /// The project directory.
        /// </summary>
        public ProjectTreeNode Game { get; private set; }

        /// <summary>
        /// The engine directory.
        /// </summary>
        public ProjectTreeNode Engine { get; private set; }

        /// <summary>
        /// The list of all projects workspace directories (including game, engine and plugins projects).
        /// </summary>
        public readonly List<ProjectTreeNode> Projects = new List<ProjectTreeNode>();

        /// <summary>
        /// The list with all content items proxy objects.
        /// </summary>
        public readonly List<ContentProxy> Proxy = new List<ContentProxy>(64);

        /// <summary>
        /// Occurs when new items is added to the workspace content database.
        /// </summary>
        public event Action<ContentItem> ItemAdded;

        /// <summary>
        /// Occurs when new items is removed from the workspace content database.
        /// </summary>
        public event Action<ContentItem> ItemRemoved;

        /// <summary>
        /// Occurs when workspace has been modified.
        /// </summary>
        public event Action WorkspaceModified;

        /// <summary>
        /// Gets the amount of created items.
        /// </summary>
        public int ItemsCreated => _itemsCreated;

        /// <summary>
        /// Gets the amount of deleted items.
        /// </summary>
        public int ItemsDeleted => _itemsDeleted;

        internal ContentDatabaseModule(Editor editor)
        : base(editor)
        {
            // Init content database after UI module
            InitOrder = -80;

            // Register AssetItems serialization helper (serialize ref ID only)
            FlaxEngine.Json.JsonSerializer.Settings.Converters.Add(new AssetItemConverter());
        }

        private void OnContentAssetDisposing(Asset asset)
        {
            // Handle deleted asset
            if (asset.ShouldDeleteFileOnUnload)
            {
                var item = Find(asset.ID);
                if (item != null)
                {
                    // Close all asset editors
                    Editor.Windows.CloseAllEditors(item);

                    // Dispose
                    item.Dispose();
                }
            }
        }

        /// <summary>
        /// Gets the project workspace used by the given project.
        /// </summary>
        /// <param name="project">The project.</param>
        /// <returns>The project workspace or null if not loaded into database.</returns>
        public ProjectTreeNode GetProjectWorkspace(ProjectInfo project)
        {
            return Projects.FirstOrDefault(x => x.Project == project);
        }

        /// <summary>
        /// Gets the proxy object for the given content item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>Content proxy for that item or null if cannot find.</returns>
        public ContentProxy GetProxy(ContentItem item)
        {
            if (item != null)
            {
                for (int i = 0; i < Proxy.Count; i++)
                {
                    if (Proxy[i].IsProxyFor(item))
                    {
                        return Proxy[i];
                    }
                }
            }

            return null;
        }

        /// <summary>
        /// Gets the proxy object for the given asset type.
        /// </summary>
        /// <returns>Content proxy for that asset type or null if cannot find.</returns>
        public ContentProxy GetProxy<T>() where T : Asset
        {
            for (int i = 0; i < Proxy.Count; i++)
            {
                if (Proxy[i].IsProxyFor<T>())
                {
                    return Proxy[i];
                }
            }

            return null;
        }

        /// <summary>
        /// Gets the proxy object for the given file extension. Warning! Different asset types may share the same file extension.
        /// </summary>
        /// <param name="extension">The file extension.</param>
        /// <returns>Content proxy for that item or null if cannot find.</returns>
        public ContentProxy GetProxy(string extension)
        {
            if (string.IsNullOrEmpty(extension))
                throw new ArgumentNullException();

            extension = StringUtils.NormalizeExtension(extension);

            for (int i = 0; i < Proxy.Count; i++)
            {
                if (Proxy[i].FileExtension == extension)
                {
                    return Proxy[i];
                }
            }

            return null;
        }

        /// <summary>
        /// Gets the proxy object for the given asset type id.
        /// </summary>
        /// <param name="typeName">The asset type name.</param>
        /// <param name="path">The asset path.</param>
        /// <returns>Asset proxy or null if cannot find.</returns>
        public AssetProxy GetAssetProxy(string typeName, string path)
        {
            for (int i = 0; i < Proxy.Count; i++)
            {
                if (Proxy[i] is AssetProxy proxy && proxy.AcceptsAsset(typeName, path))
                {
                    return proxy;
                }
            }

            return null;
        }

        /// <summary>
        /// Refreshes the given item folder. Tries to find new content items and remove not existing ones.
        /// </summary>
        /// <param name="item">Folder to refresh</param>
        /// <param name="checkSubDirs">True if search for changes inside a subdirectories, otherwise only top-most folder will be updated</param>
        public void RefreshFolder(ContentItem item, bool checkSubDirs)
        {
            // Peek folder to refresh
            ContentFolder folder = item.IsFolder ? item as ContentFolder : item.ParentFolder;
            if (folder == null)
                return;

            // Update
            LoadFolder(folder.Node, checkSubDirs);
        }

        /// <summary>
        /// Tries to find item at the specified path.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>Found item or null if cannot find it.</returns>
        public ContentItem Find(string path)
        {
            if (string.IsNullOrEmpty(path))
                return null;

            // Ensure path is normalized to the Flax format
            path = StringUtils.NormalizePath(path);

            // TODO: if it's a bottleneck try to optimize searching by spiting path

            foreach (var project in Projects)
            {
                var result = project.Folder.Find(path);
                if (result != null)
                    return result;
            }

            return null;
        }

        /// <summary>
        /// Tries to find item with the specified ID.
        /// </summary>
        /// <param name="id">The item ID.</param>
        /// <returns>Found item or null if cannot find it.</returns>
        public ContentItem Find(Guid id)
        {
            if (id == Guid.Empty)
                return null;

            // TODO: use AssetInfo via Content manager to get asset path very quickly (it's O(1))

            // TODO: if it's a bottleneck try to optimize searching by caching items IDs

            foreach (var project in Projects)
            {
                var result = project.Folder.Find(id);
                if (result != null)
                    return result;
            }
            return null;
        }

        /// <summary>
        /// Tries to find asset with the specified ID.
        /// </summary>
        /// <param name="id">The asset ID.</param>
        /// <returns>Found asset item or null if cannot find it.</returns>
        public AssetItem FindAsset(Guid id)
        {
            if (id == Guid.Empty)
                return null;

            // TODO: use AssetInfo via Content manager to get asset path very quickly (it's O(1))

            // TODO: if it's a bottleneck try to optimize searching by caching items IDs

            foreach (var project in Projects)
            {
                if (project.Content?.Folder.Find(id) is AssetItem result)
                    return result;
            }
            return null;
        }

        /// <summary>
        /// Tries to find script item at the specified path.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>Found script or null if cannot find it.</returns>
        public ScriptItem FindScript(string path)
        {
            foreach (var project in Projects)
            {
                if (project.Source?.Folder.Find(path) is ScriptItem result)
                    return result;
            }
            return null;
        }

        /// <summary>
        /// Tries to find script item with the specified ID.
        /// </summary>
        /// <param name="id">The item ID.</param>
        /// <returns>Found script or null if cannot find it.</returns>
        public ScriptItem FindScript(Guid id)
        {
            if (id == Guid.Empty)
                return null;
            foreach (var project in Projects)
            {
                if (project.Source?.Folder.Find(id) is ScriptItem result)
                    return result;
            }
            return null;
        }

        /// <summary>
        /// Tries to find script item with the specified name.
        /// </summary>
        /// <param name="scriptName">The name of the script.</param>
        /// <returns>Found script or null if cannot find it.</returns>
        public ScriptItem FindScriptWitScriptName(string scriptName)
        {
            foreach (var project in Projects)
            {
                if (project.Source?.Folder.FindScriptWitScriptName(scriptName) is ScriptItem result)
                    return result;
            }
            return null;
        }

        /// <summary>
        /// Tries to find script item that is used by the specified script object.
        /// </summary>
        /// <param name="script">The instance of the script.</param>
        /// <returns>Found script or null if cannot find it.</returns>
        public ScriptItem FindScriptWitScriptName(Script script)
        {
            return FindScriptWitScriptName(TypeUtils.GetObjectType(script));
        }

        /// <summary>
        /// Tries to find script item that is used by the specified script type.
        /// </summary>
        /// <param name="scriptType">The type of the script.</param>
        /// <returns>Found script or null if cannot find it.</returns>
        public ScriptItem FindScriptWitScriptName(ScriptType scriptType)
        {
            if (scriptType != ScriptType.Null)
            {
                var className = scriptType.Name;
                var scriptName = ScriptItem.CreateScriptName(className);
                return FindScriptWitScriptName(scriptName);
            }
            return null;
        }

        /// <summary>
        /// Renames a content item
        /// </summary>
        /// <param name="el">Content item</param>
        /// <param name="newPath">New path</param>
        /// <returns>True if failed, otherwise false</returns>
        private static bool RenameAsset(ContentItem el, ref string newPath)
        {
            string oldPath = el.Path;

            // Check if use content pool
            if (el.IsAsset)
            {
                // Rename asset
                // Note: we use content backend because file may be in use or sth, it's safe
                if (FlaxEngine.Content.RenameAsset(oldPath, newPath))
                {
                    // Error
                    Editor.LogError(string.Format("Cannot rename asset \'{0}\' to \'{1}\'", oldPath, newPath));
                    return true;
                }
            }
            else
            {
                // Rename file
                try
                {
                    File.Move(oldPath, newPath);
                }
                catch (Exception ex)
                {
                    // Error
                    Editor.LogWarning(ex);
                    Editor.LogError(string.Format("Cannot rename asset \'{0}\' to \'{1}\'", oldPath, newPath));
                    return true;
                }
            }

            // Change path
            el.UpdatePath(newPath);
            return false;
        }

        private static void UpdateAssetNewNameTree(ContentItem el)
        {
            string extension = Path.GetExtension(el.Path);
            string newPath = StringUtils.CombinePaths(el.ParentFolder.Path, el.ShortName + extension);

            // Special case for folders
            if (el.IsFolder)
            {
                // Cache data
                string oldPath = el.Path;
                var folder = (ContentFolder)el;

                // Create new folder
                try
                {
                    Directory.CreateDirectory(newPath);
                }
                catch (Exception ex)
                {
                    // Error
                    Editor.LogWarning(ex);
                    Editor.LogError(string.Format("Cannot move folder \'{0}\' to \'{1}\'", oldPath, newPath));
                    return;
                }

                // Change path
                el.UpdatePath(newPath);

                // Rename all child elements
                for (int i = 0; i < folder.Children.Count; i++)
                    UpdateAssetNewNameTree(folder.Children[i]);
            }
            else
            {
                RenameAsset(el, ref newPath);
            }
        }

        /// <summary>
        /// Moves the specified items to the different location. Handles moving whole directories and single assets.
        /// </summary>
        /// <param name="items">The items.</param>
        /// <param name="newParent">The new parent.</param>
        public void Move(List<ContentItem> items, ContentFolder newParent)
        {
            for (int i = 0; i < items.Count; i++)
                Move(items[i], newParent);
        }

        /// <summary>
        /// Moves the specified item to the different location. Handles moving whole directories and single assets.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="newParent">The new parent.</param>
        public void Move(ContentItem item, ContentFolder newParent)
        {
            if (newParent == null || item == null)
                throw new ArgumentNullException();

            // Skip nothing to change
            if (item.ParentFolder == newParent)
                return;

            var extension = Path.GetExtension(item.Path);
            var newPath = StringUtils.CombinePaths(newParent.Path, item.ShortName + extension);
            Move(item, newPath);
        }

        /// <summary>
        /// Moves the specified item to the different location. Handles moving whole directories and single assets.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="newPath">The new path.</param>
        public void Move(ContentItem item, string newPath)
        {
            if (item == null || string.IsNullOrEmpty(newPath))
                throw new ArgumentNullException();

            if (item.IsFolder && Directory.Exists(newPath))
            {
                // Error
                MessageBox.Show("Cannot move folder. Target location already exists.");
                return;
            }

            // Find target parent
            var newDirPath = Path.GetDirectoryName(newPath);
            var newParent = Find(newDirPath) as ContentFolder;
            if (newParent == null)
            {
                // Error
                MessageBox.Show("Cannot move item. Missing target location.");
                return;
            }

            // Perform renaming
            {
                string oldPath = item.Path;

                // Special case for folders
                if (item.IsFolder)
                {
                    // Cache data
                    var folder = (ContentFolder)item;

                    // Create new folder
                    try
                    {
                        Directory.CreateDirectory(newPath);
                    }
                    catch (Exception ex)
                    {
                        // Error
                        Editor.LogWarning(ex);
                        Editor.LogError(string.Format("Cannot move folder \'{0}\' to \'{1}\'", oldPath, newPath));
                        return;
                    }

                    // Change path
                    item.UpdatePath(newPath);

                    // Rename all child elements
                    for (int i = 0; i < folder.Children.Count; i++)
                        UpdateAssetNewNameTree(folder.Children[i]);

                    // Delete old folder
                    try
                    {
                        Directory.Delete(oldPath, true);
                    }
                    catch (Exception ex)
                    {
                        // Error
                        Editor.LogWarning(ex);
                        Editor.LogWarning(string.Format("Cannot remove folder \'{0}\'", oldPath));
                        return;
                    }
                }
                else
                {
                    if (RenameAsset(item, ref newPath))
                    {
                        MessageBox.Show("Cannot rename item.");
                        return;
                    }
                }

                if (item.ParentFolder != null)
                    item.ParentFolder.Node.SortChildren();
            }

            // Link item
            item.ParentFolder = newParent;

            if (_enableEvents)
                WorkspaceModified?.Invoke();
        }

        /// <summary>
        /// Copies the specified item to the target location. Handles copying whole directories and single assets.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="targetPath">The target item path.</param>
        public void Copy(ContentItem item, string targetPath)
        {
            if (item == null || !item.Exists)
            {
                // Error
                MessageBox.Show("Cannot move item. It's missing.");
                return;
            }

            // Perform copy
            {
                string sourcePath = item.Path;

                // Special case for folders
                if (item.IsFolder)
                {
                    // Cache data
                    var folder = (ContentFolder)item;

                    // Create new folder if missing
                    if (!Directory.Exists(targetPath))
                    {
                        try
                        {
                            Directory.CreateDirectory(targetPath);
                        }
                        catch (Exception ex)
                        {
                            // Error
                            Editor.LogWarning(ex);
                            Editor.LogError(string.Format("Cannot copy folder \'{0}\' to \'{1}\'", sourcePath, targetPath));
                            return;
                        }
                    }

                    // Copy all child elements
                    for (int i = 0; i < folder.Children.Count; i++)
                    {
                        var child = folder.Children[i];
                        var childExtension = Path.GetExtension(child.Path);
                        var childTargetPath = StringUtils.CombinePaths(targetPath, child.ShortName + childExtension);
                        Copy(folder.Children[i], childTargetPath);
                    }
                }
                else
                {
                    // Check if use content pool
                    if (item.IsAsset || item.ItemType == ContentItemType.Scene)
                    {
                        // Rename asset
                        // Note: we use content backend because file may be in use or sth, it's safe
                        if (Editor.ContentEditing.CloneAssetFile(sourcePath, targetPath, Guid.NewGuid()))
                        {
                            // Error
                            Editor.LogError(string.Format("Cannot copy asset \'{0}\' to \'{1}\'", sourcePath, targetPath));
                            return;
                        }
                    }
                    else
                    {
                        // Copy file
                        try
                        {
                            File.Copy(sourcePath, targetPath, true);
                        }
                        catch (Exception ex)
                        {
                            // Error
                            Editor.LogWarning(ex);
                            Editor.LogError(string.Format("Cannot copy asset \'{0}\' to \'{1}\'", sourcePath, targetPath));
                            return;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Deletes the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        public void Delete(ContentItem item)
        {
            if (item == null)
                throw new ArgumentNullException();

            // Fire events
            if (_enableEvents)
                ItemRemoved?.Invoke(item);
            item.OnDelete();
            _itemsDeleted++;

            var path = item.Path;

            // Special case for folders
            if (item is ContentFolder folder)
            {
                // TODO: maybe don't remove folders recursive but at once?

                // Delete all children
                if (folder.Children.Count > 0)
                {
                    var children = folder.Children.ToArray();
                    for (int i = 0; i < children.Length; i++)
                    {
                        Delete(children[0]);
                    }
                }

                // Remove directory
                if (Directory.Exists(path))
                {
                    try
                    {
                        Directory.Delete(path, true);
                    }
                    catch (Exception ex)
                    {
                        // Error
                        Editor.LogWarning(ex);
                        Editor.LogWarning(string.Format("Cannot remove folder \'{0}\'", path));
                        return;
                    }
                }

                // Unlink from the parent
                item.ParentFolder = null;

                // Delete tree node
                folder.Node.Dispose();
            }
            else
            {
                // Check if it's an asset
                if (item.IsAsset)
                {
                    // Delete asset by using content pool
                    FlaxEngine.Content.DeleteAsset(path);
                }
                else
                {
                    // Delete file
                    if (File.Exists(path))
                        File.Delete(path);
                }

                // Unlink from the parent
                item.ParentFolder = null;

                // Delete item
                item.Dispose();
            }

            if (_enableEvents)
                WorkspaceModified?.Invoke();
        }

        private void LoadFolder(ContentTreeNode node, bool checkSubDirs)
        {
            if (node == null)
                return;

            // Temporary data
            var folder = node.Folder;
            var path = folder.Path;

            // Check for missing files/folders (skip it during fast tree setup)
            if (!_isDuringFastSetup)
            {
                for (int i = 0; i < folder.Children.Count; i++)
                {
                    var child = folder.Children[i];
                    if (!child.Exists)
                    {
                        // Send info
                        Editor.Log(string.Format($"Content item \'{child.Path}\' has been removed"));

                        // Destroy it
                        Delete(child);

                        i--;
                    }
                }
            }

            // Find files
            var files = Directory.GetFiles(path, "*.*", SearchOption.TopDirectoryOnly);
            if (node.CanHaveAssets)
            {
                LoadAssets(node, files);
            }
            if (node.CanHaveScripts)
            {
                LoadScripts(node, files);
            }

            // Get child directories
            var childFolders = Directory.GetDirectories(path);

            // Load child folders
            bool sortChildren = false;
            for (int i = 0; i < childFolders.Length; i++)
            {
                var childPath = StringUtils.NormalizePath(childFolders[i]);

                // Check if node already has that element (skip during init when we want to walk project dir very fast)
                ContentFolder childFolderNode = _isDuringFastSetup ? null : node.Folder.FindChild(childPath) as ContentFolder;
                if (childFolderNode == null)
                {
                    // Create node
                    ContentTreeNode n = new ContentTreeNode(node, childPath);
                    if (!_isDuringFastSetup)
                        sortChildren = true;

                    // Load child folder
                    LoadFolder(n, true);

                    // Fire event
                    if (_enableEvents)
                    {
                        ItemAdded?.Invoke(n.Folder);
                        WorkspaceModified?.Invoke();
                    }
                    _itemsCreated++;
                }
                else if (checkSubDirs)
                {
                    // Update child folder
                    LoadFolder(childFolderNode.Node, true);
                }
            }
            if (sortChildren)
                node.SortChildren();
        }

        private void LoadScripts(ContentTreeNode parent, string[] files)
        {
            for (int i = 0; i < files.Length; i++)
            {
                var path = StringUtils.NormalizePath(files[i]);

                // Check if node already has that element (skip during init when we want to walk project dir very fast)
                if (_isDuringFastSetup || !parent.Folder.ContainsChild(path))
                {
                    // Create file item
                    ContentItem item;
                    if (path.EndsWith(".cs"))
                        item = new CSharpScriptItem(path);
                    else if (path.EndsWith(".cpp") || path.EndsWith(".h"))
                        item = new CppScriptItem(path);
                    else if (path.EndsWith(".shader") || path.EndsWith(".hlsl"))
                        item = new ShaderSourceItem(path);
                    else
                        item = new FileItem(path);

                    // Link
                    item.ParentFolder = parent.Folder;

                    // Fire event
                    if (_enableEvents)
                    {
                        ItemAdded?.Invoke(item);
                        WorkspaceModified?.Invoke();
                        if (!path.EndsWith(".Gen.cs"))
                        {
                            if (item is ScriptItem)
                                ScriptsBuilder.MarkWorkspaceDirty();
                            if (item is ScriptItem || item is ShaderSourceItem)
                                Editor.CodeEditing.SelectedEditor.OnFileAdded(path);
                        }
                    }
                    _itemsCreated++;
                }
            }
        }

        private void LoadAssets(ContentTreeNode parent, string[] files)
        {
            for (int i = 0; i < files.Length; i++)
            {
                var path = StringUtils.NormalizePath(files[i]);

                // Check if node already has that element (skip during init when we want to walk project dir very fast)
                if (_isDuringFastSetup || !parent.Folder.ContainsChild(path))
                {
                    // Create file item
                    ContentItem item = null;
                    if (FlaxEngine.Content.GetAssetInfo(path, out var assetInfo))
                    {
                        var proxy = GetAssetProxy(assetInfo.TypeName, path);
                        item = proxy?.ConstructItem(path, assetInfo.TypeName, ref assetInfo.ID);
                    }
                    if (item == null)
                        item = new FileItem(path);

                    // Link
                    item.ParentFolder = parent.Folder;

                    // Fire event
                    if (_enableEvents)
                    {
                        ItemAdded?.Invoke(item);
                        WorkspaceModified?.Invoke();
                    }
                    _itemsCreated++;
                }
            }
        }

        private void LoadProjects(ProjectInfo project)
        {
            var workspace = GetProjectWorkspace(project);
            if (workspace == null)
            {
                workspace = new ProjectTreeNode(project);
                Projects.Add(workspace);

                var contentFolder = Path.Combine(project.ProjectFolderPath, "Content");
                if (Directory.Exists(contentFolder))
                {
                    workspace.Content = new MainContentTreeNode(workspace, ContentFolderType.Content, contentFolder);
                    workspace.Content.Folder.ParentFolder = workspace.Folder;
                }

                var sourceFolder = Path.Combine(project.ProjectFolderPath, "Source");
                if (Directory.Exists(sourceFolder))
                {
                    workspace.Source = new MainContentTreeNode(workspace, ContentFolderType.Source, sourceFolder);
                    workspace.Source.Folder.ParentFolder = workspace.Folder;
                }
            }

            foreach (var reference in project.References)
            {
                LoadProjects(reference.Project);
            }
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            FlaxEngine.Content.AssetDisposing += OnContentAssetDisposing;

            // Setup content proxies
            Proxy.Add(new TextureProxy());
            Proxy.Add(new ModelProxy());
            Proxy.Add(new SkinnedModelProxy());
            Proxy.Add(new MaterialProxy());
            Proxy.Add(new MaterialInstanceProxy());
            Proxy.Add(new MaterialFunctionProxy());
            Proxy.Add(new SpriteAtlasProxy());
            Proxy.Add(new CubeTextureProxy());
            Proxy.Add(new PreviewsCacheProxy());
            Proxy.Add(new FontProxy());
            Proxy.Add(new ShaderProxy());
            Proxy.Add(new ShaderSourceProxy());
            Proxy.Add(new ParticleEmitterProxy());
            Proxy.Add(new ParticleEmitterFunctionProxy());
            Proxy.Add(new ParticleSystemProxy());
            Proxy.Add(new SceneAnimationProxy());
            Proxy.Add(new CSharpScriptProxy());
            Proxy.Add(new CppAssetProxy());
            Proxy.Add(new CppStaticClassProxy());
            Proxy.Add(new CppScriptProxy());
            Proxy.Add(new SceneProxy());
            Proxy.Add(new PrefabProxy());
            Proxy.Add(new IESProfileProxy());
            Proxy.Add(new CollisionDataProxy());
            Proxy.Add(new AudioClipProxy());
            Proxy.Add(new AnimationGraphProxy());
            Proxy.Add(new AnimationGraphFunctionProxy());
            Proxy.Add(new AnimationProxy());
            Proxy.Add(new SkeletonMaskProxy());
            Proxy.Add(new GameplayGlobalsProxy());
            Proxy.Add(new VisualScriptProxy());
            Proxy.Add(new LocalizedStringTableProxy());
            Proxy.Add(new FileProxy());
            Proxy.Add(new SpawnableJsonAssetProxy<PhysicalMaterial>());

            // Settings
            Proxy.Add(new SettingsProxy(typeof(GameSettings), Editor.Instance.Icons.GameSettings128));
            Proxy.Add(new SettingsProxy(typeof(TimeSettings), Editor.Instance.Icons.TimeSettings128));
            Proxy.Add(new SettingsProxy(typeof(LayersAndTagsSettings), Editor.Instance.Icons.LayersTagsSettings128));
            Proxy.Add(new SettingsProxy(typeof(PhysicsSettings), Editor.Instance.Icons.PhysicsSettings128));
            Proxy.Add(new SettingsProxy(typeof(GraphicsSettings), Editor.Instance.Icons.GraphicsSettings128));
            Proxy.Add(new SettingsProxy(typeof(NavigationSettings), Editor.Instance.Icons.NavigationSettings128));
            Proxy.Add(new SettingsProxy(typeof(LocalizationSettings), Editor.Instance.Icons.LocalizationSettings128));
            Proxy.Add(new SettingsProxy(typeof(AudioSettings), Editor.Instance.Icons.AudioSettings128));
            Proxy.Add(new SettingsProxy(typeof(BuildSettings), Editor.Instance.Icons.BuildSettings128));
            Proxy.Add(new SettingsProxy(typeof(InputSettings), Editor.Instance.Icons.InputSettings128));
            Proxy.Add(new SettingsProxy(typeof(StreamingSettings), Editor.Instance.Icons.BuildSettings128));
            Proxy.Add(new SettingsProxy(typeof(WindowsPlatformSettings), Editor.Instance.Icons.WindowsSettings128));
            Proxy.Add(new SettingsProxy(typeof(UWPPlatformSettings), Editor.Instance.Icons.UWPSettings128));
            Proxy.Add(new SettingsProxy(typeof(LinuxPlatformSettings), Editor.Instance.Icons.LinuxSettings128));
            Proxy.Add(new SettingsProxy(typeof(AndroidPlatformSettings), Editor.Instance.Icons.AndroidSettings128));

            var typePS4PlatformSettings = TypeUtils.GetManagedType(GameSettings.PS4PlatformSettingsTypename);
            if (typePS4PlatformSettings != null)
                Proxy.Add(new SettingsProxy(typePS4PlatformSettings, Editor.Instance.Icons.PlaystationSettings128));

            var typeXboxScarlettPlatformSettings = TypeUtils.GetManagedType(GameSettings.XboxScarlettPlatformSettingsTypename);
            if (typeXboxScarlettPlatformSettings != null)
                Proxy.Add(new SettingsProxy(typeXboxScarlettPlatformSettings, Editor.Instance.Icons.XBoxScarletIcon128));

            var typeSwitchPlatformSettings = TypeUtils.GetManagedType(GameSettings.SwitchPlatformSettingsTypename);
            if (typeSwitchPlatformSettings != null)
                Proxy.Add(new SettingsProxy(typeSwitchPlatformSettings, Editor.Instance.Icons.SwitchSettings128));

            // Last add generic json (won't override other json proxies)
            Proxy.Add(new GenericJsonAssetProxy());

            // Create content folders nodes
            Engine = new ProjectTreeNode(Editor.EngineProject)
            {
                Content = new MainContentTreeNode(Engine, ContentFolderType.Content, Globals.EngineContentFolder),
            };
            if (Editor.GameProject != Editor.EngineProject)
            {
                Game = new ProjectTreeNode(Editor.GameProject)
                {
                    Content = new MainContentTreeNode(Game, ContentFolderType.Content, Globals.ProjectContentFolder),
                    Source = new MainContentTreeNode(Game, ContentFolderType.Source, Globals.ProjectSourceFolder),
                };
                // TODO: why it's required? the code above should work for linking the nodes hierarchy
                Game.Content.Folder.ParentFolder = Game.Folder;
                Game.Source.Folder.ParentFolder = Game.Folder;
                Projects.Add(Game);
            }
            Engine.Content.Folder.ParentFolder = Engine.Folder;
            Projects.Add(Engine);
            if (Editor.GameProject != Editor.EngineProject)
            {
                LoadProjects(Game.Project);
            }

            // Load all folders
            // TODO: we should create async task for gathering content and whole workspace contents if it takes too long
            // TODO: create progress bar in content window and after end we should enable events and update it
            _isDuringFastSetup = true;
            foreach (var project in Projects)
            {
                if (project.Content != null)
                    LoadFolder(project.Content, true);
                if (project.Source != null)
                    LoadFolder(project.Source, true);
            }
            _isDuringFastSetup = false;

            // Enable events
            _enableEvents = true;
            Editor.ContentImporting.ImportFileEnd += ContentImporting_ImportFileDone;

            Editor.Log("Project database created. Items count: " + _itemsCreated);
        }

        private void ContentImporting_ImportFileDone(IFileEntryAction obj, bool failed)
        {
            if (failed)
                return;

            // Check if already has that element
            var item = Find(obj.ResultUrl);
            if (item is BinaryAssetItem binaryAssetItem)
            {
                // Get asset info from the registry (content layer will update cache it just after import)
                if (FlaxEngine.Content.GetAssetInfo(binaryAssetItem.Path, out var assetInfo))
                {
                    // If asset type id has been changed we HAVE TO close all windows that use it
                    // For eg. change texture to sprite atlas on reimport
                    if (binaryAssetItem.TypeName != assetInfo.TypeName)
                    {
                        // Asset type has been changed!
                        Editor.LogWarning(string.Format("Asset \'{0}\' changed type from {1} to {2}", item.Path, binaryAssetItem.TypeName, assetInfo.TypeName));
                        Editor.Windows.CloseAllEditors(item);

                        // Remove this item from the database and some related data
                        var toRefresh = binaryAssetItem.ParentFolder;
                        binaryAssetItem.Dispose();
                        toRefresh.Children.Remove(binaryAssetItem);
                        if (!binaryAssetItem.HasDefaultThumbnail)
                        {
                            // Delete old thumbnail and remove it from the cache
                            Editor.Instance.Thumbnails.DeletePreview(binaryAssetItem);
                        }

                        // Refresh the parent folder to find the new asset (it should have different type or some other format)
                        RefreshFolder(toRefresh, false);
                    }
                    else
                    {
                        // Refresh element data that could change during importing
                        binaryAssetItem.OnReimport(ref assetInfo.ID);
                    }
                }

                // Refresh content view (not the best design because window could also track this event but it gives better performance)
                Editor.Windows.ContentWin?.RefreshView();
            }
        }

        internal void OnDirectoryEvent(MainContentTreeNode node, FileSystemEventArgs e)
        {
            // Ensure to be ready for external events
            if (_isDuringFastSetup)
                return;

            // TODO: maybe we could make it faster! since we have a path so it would be easy to just create or delete given file. but remember about subdirectories

            // Switch type
            switch (e.ChangeType)
            {
            case WatcherChangeTypes.Created:
            case WatcherChangeTypes.Deleted:
            {
                lock (_dirtyNodes)
                {
                    _dirtyNodes.Add(node);
                }
                break;
            }
            }
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            // Update all dirty content tree nodes
            lock (_dirtyNodes)
            {
                foreach (var node in _dirtyNodes)
                {
                    LoadFolder(node, true);

                    if (_enableEvents)
                        WorkspaceModified?.Invoke();
                }
                _dirtyNodes.Clear();
            }
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            FlaxEngine.Content.AssetDisposing -= OnContentAssetDisposing;

            // Disable events
            _enableEvents = false;

            // Cleanup
            Proxy.ForEach(x => x.Dispose());
            if (Game != null)
            {
                Game.Dispose();
                Game = null;
            }
            if (Engine != null)
            {
                Engine.Dispose();
                Engine = null;
            }
            Proxy.Clear();
        }
    }
}
