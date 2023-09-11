using FlaxEditor.Content;
using FlaxEditor.Content.GUI;
using FlaxEngine;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
namespace FlaxEditor.Content
{
    /// <summary>
    /// class with is controling visability of items in content window
    /// </summary>
    internal class ContentFilter
    {
        #region Filters Config
        /// <summary>
        /// all suported files by engine Content Folder
        /// </summary>
        public static readonly List<string> SuportedFileExtencionsInContentFolder = new List<string>()
        {
            ".flax",
            ".json",
            ".scene",
            ".prefab",
        };
        /// <summary>
        /// all suported files by engine Source Folder
        /// </summary>
        public static readonly List<string> SuportedFileExtencionsInSourceFolder = new List<string>()
        {
            ".shader",
            ".cs",
            ".h",
            ".cpp",
        };
        /// <summary>
        /// ignores folders in source folder (top layer),
        /// for example obj folder is default c# project folder 
        /// </summary>
        internal static readonly List<string> HideFoldersInSourceFolder = new List<string>()
        {
            "obj",                  // default c# project folder
            "Properties",           // c# project stuff ?
        };
        /// <summary>
        /// ignores files in source folder (top layer),
        /// </summary>
        internal static readonly List<string> HideFilesInSourceFolder = new List<string>() //dont edit
        {
            "Game.csproj",                  //solucion file
            "Game.Gen.cs",                  //auto-generated not be edited
            "GameEditorTarget.Build.cs",    
            "GameTarget.Build.cs",          
        };
        #endregion
        internal static ContentItem gameSettings;
        internal static List<ContentItem> buildFiles;

        internal static ContentTreeNode settings;
        internal static ContentTreeNode shaders;
        internal static ProjectTreeNode engine;
        internal static List<ProjectTreeNode> plugins = new();
        internal static List<ContentItem> FilterFolder(ContentFolder folder)
        {
            return FilterFolder(folder, SuportedFileExtencionsInContentFolder.ToArray(), SuportedFileExtencionsInSourceFolder.ToArray());
        }
        internal static List<ContentItem> FilterFolder(ContentFolder folder, string[] FileExtencionsInContentFolder, string[] ExtencionsInSourceFolder)
        {
            if (folder.CanHaveAssets)
            {
                for (int i = 0; i < folder.Children.Count; i++)
                {
                    bool Visible = false;
                    for (int j = 0; j < FileExtencionsInContentFolder.Length; j++)
                    {
                        if ((folder.Children[i].Path.EndsWith(FileExtencionsInContentFolder[j]) || folder.Children[i].IsFolder))
                        {
                            if (folder.Children[i].Visible)
                            {
                                Visible = true;
                            }
                            break;
                        }
                    }
                    folder.Children[i].Visible = Visible;
                }
            }
            else if (folder.CanHaveScripts)
            {
                for (int i = 0; i < folder.Children.Count; i++)
                {
                    bool Visible = false;
                    for (int j = 0; j < ExtencionsInSourceFolder.Length; j++)
                    {
                        if ((folder.Children[i].Path.EndsWith(ExtencionsInSourceFolder[j]) || folder.Children[i].IsFolder))
                        {
                            if (folder.Children[i].Visible)
                            {
                                Visible = true;
                            }
                            break;
                        }
                    }
                    folder.Children[i].Visible = Visible;
                }
            }
            return folder.Children;
        }
        internal static ProjectTreeNode Filter(ProjectTreeNode tree)
        {
            var content = tree.Children[0] as ContentTreeNode;
            var source = tree.Children[1] as ContentTreeNode;
            //filter content folder (top layer)
            buildFiles = new();
            for (int i = 0; i < content.Folder.Children.Count; i++)
            {
                if (content.Folder.Children[i].FileName == "GameSettings.json")
                {
                    gameSettings = content.Folder.Children[i];
                    content.Folder.Children[i].Visible = false;
                    break;
                }
            }

            int hindenCount = 0;

            for (int i = content.Children.Count - 1; i >= 0; i--)//we are starting from back it faster
            {
                var node = content.Children[i] as ContentTreeNode;
                if (node.Folder.FileName == "Settings")
                {
                    settings = node;
                    hindenCount++;
                    node.Visible = false;
                    node.Folder.Visible = false;
                }
                if (node.Folder.FileName == "Shaders")
                {
                    shaders = node;
                    hindenCount++;
                    node.Visible = false;
                    node.Folder.Visible = false;

                }
                if (hindenCount == 2)
                    break;
            }

            
            //-----------------------------------------------------------------------------------------------------

            //filter source folder (top layer)
            hindenCount = 0;
            for (int i = 0; i < source.Folder.Children.Count; i++)
            {
                for (int j = 0; j < HideFilesInSourceFolder.Count; j++)
                {
                    if (source.Folder.Children[i].FileName == HideFilesInSourceFolder[j])
                    {
                        source.Folder.Children[i].Visible = false;
                        hindenCount++;
                        if(i > 1)
                        {
                            buildFiles.Add(source.Folder.Children[i]);
                        }
                        if (HideFilesInSourceFolder.Count == hindenCount) goto HideFilesInSourceFolderComplited;
                        break;
                    }
                }
            }
        HideFilesInSourceFolderComplited:
            hindenCount = 0;
            for (int i = source.Children.Count - 1; i >= 0; i--)
            {
                var node = source.Children[i] as ContentTreeNode;
                for (int j = 0; j < HideFoldersInSourceFolder.Count; j++)
                {
                    if (node.Folder.FileName == HideFoldersInSourceFolder[j])
                    {
                        node.Visible = false;
                        node.Folder.Visible = false;
                        hindenCount++;
                        if (HideFoldersInSourceFolder.Count == hindenCount) goto HideFoldersInSourceFolderComplited;
                        break;
                    }
                }
            }
        HideFoldersInSourceFolderComplited:
            //content
            return tree;
        }
        internal static void UpdateFilterVisability(ContentSettingsDropdown dropdown)
        {
            engine.Visible = false;
            engine.Folder.Visible = false;

            foreach (var item in plugins)
            {
                item.Visible = false;
                item.Folder.Visible = false;
            }
            foreach (var item in buildFiles)
            {
                item.Visible = false;
            }
            gameSettings.Visible = false;
            settings.Visible = false;
            settings.Folder.Visible = false;

            shaders.Visible = false;
            shaders.Folder.Visible = false;

            for (int i = 0; i < dropdown.Selection.Count; i++)
            {
                if (dropdown.Selection[i] == 0) //Show Engine Content
                {
                    engine.Visible = true;
                    engine.Folder.Visible = true;
                }

                if (dropdown.Selection[i] == 1)//Show Plugin Content
                {
                    foreach (var item in plugins)
                    {
                        item.Visible = true;
                        item.Folder.Visible = true;
                    }
                }

                if (dropdown.Selection[i] == 2)//Show Build Files
                {
                    foreach (var item in buildFiles)
                    {
                        item.Visible = true;
                    }
                }

                if (dropdown.Selection[i] == 3)//Show Game Settings
                {
                    gameSettings.Visible = true;
                    settings.Visible = true;
                    settings.Folder.Visible = true;
                }

                if (dropdown.Selection[i] == 4)//"Show Shader Source"
                {
                    shaders.Visible = true;
                    shaders.Folder.Visible = true;
                }
            }
            engine.ParentTree.PerformLayout();
            Editor.Instance.Windows.ContentWin.RefreshView();
        }
    }
}
