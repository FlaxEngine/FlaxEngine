// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Collections.Generic;
using FlaxEngine;
using Newtonsoft.Json;

namespace FlaxEditor
{
    /// <summary>
    /// Contains information about Flax project.
    /// </summary>
    public sealed class ProjectInfo
    {
        private static List<ProjectInfo> _projectsCache;

        /// <summary>
        /// The project reference.
        /// </summary>
        public class Reference
        {
            /// <summary>
            /// The referenced project name.
            /// </summary>
            public string Name;

            /// <summary>
            /// The referenced project.
            /// </summary>
            [NonSerialized]
            public ProjectInfo Project;

            /// <inheritdoc />
            public override string ToString()
            {
                return Name;
            }
        }

        /// <summary>
        /// The project name.
        /// </summary>
        public string Name;

        /// <summary>
        /// The project file path.
        /// </summary>
        [NonSerialized]
        public string ProjectPath;

        /// <summary>
        /// The project root folder path.
        /// </summary>
        [NonSerialized]
        public string ProjectFolderPath;

        /// <summary>
        /// The content assets folder
        /// </summary>
        public string ContentFolderPath;

        /// <summary>
        /// The source files folder path
        /// </summary>
        public string SourceFolderPath;

        /// <summary>
        /// The project version.
        /// </summary>
        public Version Version;

        /// <summary>
        /// The project publisher company.
        /// </summary>
        public string Company = string.Empty;

        /// <summary>
        /// The project copyright note.
        /// </summary>
        public string Copyright = string.Empty;

        /// <summary>
        /// The name of the build target to use for the game building (final, cooked game code).
        /// </summary>
        public string GameTarget;

        /// <summary>
        /// The name of the build target to use for the game in editor building (editor game code).
        /// </summary>
        public string EditorTarget;

        /// <summary>
        /// The project references.
        /// </summary>
        public Reference[] References = new Reference[0];

        /// <summary>
        /// The default scene asset identifier to open on project startup.
        /// </summary>
        public string DefaultScene;

        /// <summary>
        /// The default scene spawn point (position and view direction).
        /// </summary>
        public Ray DefaultSceneSpawn;

        /// <summary>
        /// The minimum version supported by this project.
        /// </summary>
        public Version MinEngineVersion;

        /// <summary>
        /// The user-friendly nickname of the engine installation to use when opening the project. Can be used to open game project with a custom engine distributed for team members. This value must be the same in engine and game projects to be paired.
        /// </summary>
        public string EngineNickname;

        /// <summary>
        /// Gets all projects including this project, it's references and their references (any deep level of references).
        /// </summary>
        /// <returns>The collection of projects.</returns>
        public HashSet<ProjectInfo> GetAllProjects()
        {
            var result = new HashSet<ProjectInfo>();
            GetAllProjects(result);
            return result;
        }

        private void GetAllProjects(HashSet<ProjectInfo> result)
        {
            result.Add(this);
            foreach (var reference in References)
                reference.Project.GetAllProjects(result);
        }

        /// <summary>
        /// Saves the project file.
        /// </summary>
        public void Save()
        {
            var contents = FlaxEngine.Json.JsonSerializer.Serialize(this);
            File.WriteAllText(ProjectPath, contents);
        }

        /// <summary>
        /// Loads the project from the specified file.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>The loaded project.</returns>
        public static ProjectInfo Load(string path)
        {
            // Try to reuse loaded file
            path = StringUtils.RemovePathRelativeParts(path);
            if (_projectsCache == null)
                _projectsCache = new List<ProjectInfo>();
            for (int i = 0; i < _projectsCache.Count; i++)
            {
                if (_projectsCache[i].ProjectPath == path)
                    return _projectsCache[i];
            }

            Profiler.BeginEvent(path);
            try
            {
                // Load
                var contents = File.ReadAllText(path);
                var project = JsonConvert.DeserializeObject<ProjectInfo>(contents);
                project.ProjectPath = path;
                project.ProjectFolderPath = StringUtils.NormalizePath(Path.GetDirectoryName(path));
                project.ContentFolderPath = StringUtils.NormalizePath(project.ProjectFolderPath + "/Content");
                project.SourceFolderPath = StringUtils.NormalizePath(project.ProjectFolderPath + "/Source");

                // Process project data
                if (string.IsNullOrEmpty(project.Name))
                    throw new Exception("Missing project name.");
                if (project.Version == null)
                    project.Version = new Version(1, 0);
                if (project.Version.Revision == 0)
                    project.Version = new Version(project.Version.Major, project.Version.Minor, project.Version.Build);
                if (project.Version.Build == 0 && project.Version.Revision == -1)
                    project.Version = new Version(project.Version.Major, project.Version.Minor);
                foreach (var reference in project.References)
                {
                    string referencePath;
                    if (reference.Name.StartsWith("$(EnginePath)"))
                    {
                        // Relative to engine root
                        referencePath = Path.Combine(Globals.StartupFolder, reference.Name.Substring(14));
                    }
                    else if (reference.Name.StartsWith("$(ProjectPath)"))
                    {
                        // Relative to project root
                        referencePath = Path.Combine(project.ProjectFolderPath, reference.Name.Substring(15));
                    }
                    else if (Path.IsPathRooted(reference.Name))
                    {
                        // Relative to workspace
                        referencePath = Path.Combine(Environment.CurrentDirectory, reference.Name);
                    }
                    else
                    {
                        // Absolute
                        referencePath = reference.Name;
                    }

                    // Load referenced project
                    reference.Project = Load(referencePath);
                }

                // Project loaded
                Editor.Log($"Loaded project {project.Name}, version {project.Version}");
                _projectsCache.Add(project);
                return project;
            }
            catch
            {
                // Failed to load project
                Editor.LogError("Failed to load project \"" + path + "\".");
                throw;
            }
            finally
            {
                Profiler.EndEvent();
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return $"{Name} ({ProjectPath})";
        }
    }
}
