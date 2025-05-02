// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine.Json;

namespace FlaxEngine
{
    partial class JsonAsset
    {
        private object _instance;

        /// <summary>
        /// Gets the instance of the serialized object from the json asset data. Cached internally.
        /// </summary>
        public object Instance => _instance ?? (_instance = CreateInstance());

        /// <summary>
        /// Gets the instance of the serialized object from the json data. Cached internally.
        /// </summary>
        /// <returns>The asset instance object or null.</returns>
        public T GetInstance<T>()
        {
            if (Instance is T instance)
                return instance;
            return default;
        }

        /// <summary>
        /// Creates a new instance of the serialized object from the json asset data.
        /// </summary>
        /// <remarks>Use <see cref="Instance"/> to get cached object.</remarks>
        /// <returns>The new object or null if failed.</returns>
        public T CreateInstance<T>()
        {
            return (T)CreateInstance();
        }

        /// <summary>
        /// Creates a new instance of the serialized object from the json asset data.
        /// </summary>
        /// <remarks>Use <see cref="Instance"/> to get cached object.</remarks>
        /// <returns>The new object or null if failed.</returns>
        public object CreateInstance()
        {
            if (WaitForLoaded())
                return null;

            var dataTypeName = DataTypeName;
            if (string.IsNullOrEmpty(dataTypeName))
            {
                Debug.LogError(string.Format("Missing typename of data in Json asset '{0}'.", Path), this);
                return null;
            }
            var assemblies = Utils.GetAssemblies();
            for (int i = 0; i < assemblies.Length; i++)
            {
                var assembly = assemblies[i];
                if (assembly != null)
                {
                    var type = assembly.GetType(dataTypeName);
                    if (type != null)
                    {
                        object obj = null;
                        try
                        {
                            // Create instance
                            obj = Activator.CreateInstance(type);

                            // Deserialize object
                            var data = Data;
                            JsonSerializer.Deserialize(obj, data);
                        }
                        catch (Exception ex)
                        {
                            Debug.LogException(ex, this);
                        }
                        return obj;
                    }
                }
            }

            Debug.LogError(string.Format("Missing type '{0}' to create Json Asset instance.", dataTypeName), this);
            return null;
        }

        /// <summary>
        /// Sets the instance of the asset (for both C# and C++). Doesn't save asset to the file (runtime only).
        /// </summary>
        /// <param name="instance">The new instance.</param>
        public void SetInstance(object instance)
        {
            _instance = instance;
            string str = instance != null ? JsonSerializer.Serialize(instance) : null;
            Data = str;
        }
    }
}
