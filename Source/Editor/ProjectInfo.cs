// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Collections.Generic;
using FlaxEngine;
using Newtonsoft.Json;

namespace FlaxEditor
{
    /// <summary>
    /// 
    /// </summary>
    public class FlaxVersionConverter : JsonConverter
    {
        // Original implementation is based on Newtonsoft.Json VersionConverter
        /// <summary>
        /// Writes the JSON representation of the object.
        /// </summary>
        /// <param name="writer">The <see cref="JsonWriter"/> to write to.</param>
        /// <param name="value">The value.</param>
        /// <param name="serializer">The calling serializer.</param>
        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            if (value == null)
                writer.WriteNull();
            else if (value is Version)
                writer.WriteValue(value.ToString());
            else
                throw new JsonSerializationException("Expected Version object value");
        }

        /// <summary>
        /// Reads the JSON representation of the object.
        /// </summary>
        /// <param name="reader">The <see cref="JsonReader"/> to read from.</param>
        /// <param name="objectType">Type of the object.</param>
        /// <param name="existingValue">The existing property value of the JSON that is being converted.</param>
        /// <param name="serializer">The calling serializer.</param>
        /// <returns>The object value.</returns>
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            if (reader.TokenType == JsonToken.Null)
                return null;

            if (reader.TokenType == JsonToken.StartObject)
            {
                try
                {
                    reader.Read();
                    var values = new Dictionary<string, int>();
                    while (reader.TokenType == JsonToken.PropertyName)
                    {
                        var key = reader.Value as string;
                        reader.Read();
                        var val = (long)reader.Value;
                        reader.Read();
                        values.Add(key, (int)val);
                    }

                    values.TryGetValue("Major", out var major);
                    values.TryGetValue("Minor", out var minor);
                    if (!values.TryGetValue("Build", out var build))
                        build = -1;
                    if (!values.TryGetValue("Revision", out var revision))
                        revision = -1;

                    if (build <= 0)
                        return new Version(major, minor);
                    if (revision <= 0)
                        return new Version(major, minor, build);
                    return new Version(major, minor, build, revision);
                }
                catch (Exception ex)
                {
                    throw new Exception(String.Format("Error parsing version string: {0}", reader.Value), ex);
                }
            }
            if (reader.TokenType == JsonToken.String)
            {
                try
                {
                    return new Version((string)reader.Value!);
                }
                catch (Exception ex)
                {
                    throw new Exception(String.Format("Error parsing version string: {0}", reader.Value), ex);
                }
            }
            throw new Exception(String.Format("Unexpected token or value when parsing version. Token: {0}, Value: {1}", reader.TokenType, reader.Value));
        }

        /// <summary>
        /// Determines whether this instance can convert the specified object type.
        /// </summary>
        /// <param name="objectType">Type of the object.</param>
        /// <returns><c>true</c> if this instance can convert the specified object type; otherwise, <c>false</c>.</returns>
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(Version);
        }
    }

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
                var project = JsonConvert.DeserializeObject<ProjectInfo>(contents, new JsonSerializerSettings() { Converters = new[] { new FlaxVersionConverter() } });
                project.ProjectPath = path;
                project.ProjectFolderPath = StringUtils.NormalizePath(Path.GetDirectoryName(path));

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
