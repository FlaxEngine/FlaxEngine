// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using Newtonsoft.Json;

namespace Flax.Build
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
        /// The minimum version supported by this project.
        /// </summary>
        public Version MinEngineVersion;

        /// <summary>
        /// The user-friendly nickname of the engine installation to use when opening the project. Can be used to open game project with a custom engine distributed for team members. This value must be the same in engine and game projects to be paired.
        /// </summary>
        public string EngineNickname;

        /// <summary>
        /// True if project is using C#-only and no native toolsets is required to build and use scripts.
        /// </summary>
        public bool IsCSharpOnlyProject
        {
            get
            {
                if (!_isCSharpOnlyProject.HasValue)
                {
                    var isCSharpOnlyProject = IsTargetCSharpOnly(GameTarget) && IsTargetCSharpOnly(EditorTarget);
                    if (isCSharpOnlyProject)
                    {
                        isCSharpOnlyProject &= References.All(x => x.Project == null || x.Project.Name == "Flax" || x.Project.IsCSharpOnlyProject);
                    }
                    _isCSharpOnlyProject = isCSharpOnlyProject;
                }
                return _isCSharpOnlyProject.Value;
            }
        }

        private bool? _isCSharpOnlyProject;

        private bool IsTargetCSharpOnly(string name)
        {
            if (string.IsNullOrWhiteSpace(name))
                return true;
            var rules = Builder.GenerateRulesAssembly();
            var target = rules.GetTarget(name);
            return target == null || target.Modules.TrueForAll(x => !rules.GetModule(x).BuildNativeCode);
        }

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
            var contents = JsonConvert.SerializeObject(this);
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
            path = Utilities.RemovePathRelativeParts(path);
            if (_projectsCache == null)
                _projectsCache = new List<ProjectInfo>();
            for (int i = 0; i < _projectsCache.Count; i++)
            {
                if (_projectsCache[i].ProjectPath == path)
                    return _projectsCache[i];
            }

            try
            {
                // Load
                Log.Verbose("Loading project file from \"" + path + "\"...");
                var contents = File.ReadAllText(path);
                var project = JsonConvert.DeserializeObject<ProjectInfo>(contents);
                project.ProjectPath = path;
                project.ProjectFolderPath = Path.GetDirectoryName(path);

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
                        referencePath = Path.Combine(Globals.EngineRoot, reference.Name.Substring(14));
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
                    referencePath = Utilities.RemovePathRelativeParts(referencePath);

                    // Load referenced project
                    reference.Project = Load(referencePath);
                }

                // Project loaded
                Log.Verbose($"Loaded project {project.Name}, version {project.Version}");
                _projectsCache.Add(project);
                return project;
            }
            catch
            {
                // Failed to load project
                Log.Error("Failed to load project \"" + path + "\".");
                throw;
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return $"{Name} ({ProjectPath})";
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            var info = obj as ProjectInfo;
            return info != null &&
                   Name == info.Name &&
                   ProjectPath == info.ProjectPath &&
                   ProjectFolderPath == info.ProjectFolderPath &&
                   EqualityComparer<Version>.Default.Equals(Version, info.Version) &&
                   Company == info.Company &&
                   Copyright == info.Copyright &&
                   EqualityComparer<Reference[]>.Default.Equals(References, info.References);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            var hashCode = -833167044;
            hashCode = hashCode * -1521134295 + EqualityComparer<string>.Default.GetHashCode(Name);
            hashCode = hashCode * -1521134295 + EqualityComparer<string>.Default.GetHashCode(ProjectPath);
            hashCode = hashCode * -1521134295 + EqualityComparer<string>.Default.GetHashCode(ProjectFolderPath);
            hashCode = hashCode * -1521134295 + EqualityComparer<Version>.Default.GetHashCode(Version);
            hashCode = hashCode * -1521134295 + EqualityComparer<string>.Default.GetHashCode(Company);
            hashCode = hashCode * -1521134295 + EqualityComparer<string>.Default.GetHashCode(Copyright);
            hashCode = hashCode * -1521134295 + EqualityComparer<Reference[]>.Default.GetHashCode(References);
            return hashCode;
        }
    }
}
