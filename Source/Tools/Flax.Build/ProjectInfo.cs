// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Flax.Build
{
    public class FlaxVersionConverter : JsonConverter<Version>
    {
        /// <summary>
        /// Writes the JSON representation of the object.
        /// </summary>
        /// <param name="writer">The <see cref="Utf8JsonWriter"/> to write to.</param>
        /// <param name="value">The value.</param>
        /// <param name="options">The calling serializer.</param>
        public override void Write(Utf8JsonWriter writer, Version value, JsonSerializerOptions options)
        {
            writer.WriteStringValue(value.ToString());
        }

        /// <summary>
        /// Reads the JSON representation of the object.
        /// </summary>
        /// <param name="reader">The <see cref="Utf8JsonReader"/> to read from.</param>
        /// <param name="typeToConvert">Type of the object.</param>
        /// <param name="options">The serializer options.</param>
        /// <returns>The object value.</returns>
        public override Version? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            if (reader.TokenType == JsonTokenType.Null)
                return null;

            if (reader.TokenType == JsonTokenType.StartObject)
            {
                try
                {
                    reader.Read();
                    var values = new Dictionary<string, int>();
                    while (reader.TokenType == JsonTokenType.PropertyName)
                    {
                        var key = reader.GetString();
                        reader.Read();
                        var val = reader.GetInt32();
                        reader.Read();
                        values.Add(key, val);
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
                    throw new Exception(string.Format("Error parsing version string: {0}", reader.GetString()), ex);
                }
            }

            if (reader.TokenType == JsonTokenType.String)
            {
                try
                {
                    return new Version((string)reader.GetString()!);
                }
                catch (Exception ex)
                {
                    throw new Exception(string.Format("Error parsing version string: {0}", reader.GetString()), ex);
                }
            }
            throw new Exception(string.Format("Unexpected token or value when parsing version. Token: {0}, Value: {1}", reader.TokenType, reader.GetString()));
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

    public class ConfigurationDictionaryConverter : System.Text.Json.Serialization.JsonConverter<Dictionary<string, string>>
    {
        public override Dictionary<string, string>? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            var dictionary = new Dictionary<string, string>();
            while (reader.Read())
            {
                if (reader.TokenType == JsonTokenType.EndObject)
                    return dictionary;
                if (reader.TokenType != JsonTokenType.PropertyName)
                    throw new Exception();

                string key = reader.GetString();
                reader.Read();

                string value;
                if (reader.TokenType == JsonTokenType.True)
                    value = "true";
                else if (reader.TokenType == JsonTokenType.False)
                    value = "false";
                else
                    value = reader.GetString();
                dictionary.Add(key, value);
            }
            throw new Exception();
        }

        public override void Write(Utf8JsonWriter writer, Dictionary<string, string> dictionary, JsonSerializerOptions options)
        {
            writer.WriteStartObject();
            foreach ((string key, string value) in dictionary)
            {
                var propertyName = key.ToString();
                writer.WritePropertyName(options.PropertyNamingPolicy?.ConvertName(propertyName) ?? propertyName);
                writer.WriteStringValue(value);
            }
            writer.WriteEndObject();
        }
    }

    [JsonSourceGenerationOptions(IncludeFields = true)]
    [JsonSerializable(typeof(ProjectInfo))]
    internal partial class ProjectInfoSourceGenerationContext : JsonSerializerContext
    {
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
            [JsonIgnore]
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
        [JsonIgnore]
        public string ProjectPath;

        /// <summary>
        /// The project root folder path.
        /// </summary>
        [JsonIgnore]
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
        /// The custom build configuration entries loaded from project file.
        /// </summary>
        [System.Text.Json.Serialization.JsonConverter(typeof(ConfigurationDictionaryConverter))]
        public Dictionary<string, string> Configuration;

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
            return target == null || target.Modules.TrueForAll(moduleName =>
            {
                var module = rules.GetModule(moduleName);
                return module != null && !module.BuildNativeCode;
            });
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
            var contents = JsonSerializer.Serialize<ProjectInfo>(this, new JsonSerializerOptions() { Converters = { new FlaxVersionConverter() }, TypeInfoResolver = ProjectInfoSourceGenerationContext.Default });
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
                var project = JsonSerializer.Deserialize<ProjectInfo>(contents.AsSpan(),
                                                                      new JsonSerializerOptions() { Converters = { new FlaxVersionConverter() }, IncludeFields = true, TypeInfoResolver = ProjectInfoSourceGenerationContext.Default });
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
