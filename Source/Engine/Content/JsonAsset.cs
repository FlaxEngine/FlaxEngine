// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine.Json;

namespace FlaxEngine
{
    partial class JsonAsset
    {
        /// <summary>
        /// Creates the serialized object instance from the json asset data.
        /// </summary>
        /// <returns>The created object or null.</returns>
        public T CreateInstance<T>()
        {
            return (T)CreateInstance();
        }

        /// <summary>
        /// Creates the serialized object instance from the json asset data.
        /// </summary>
        /// <returns>The created object or null.</returns>
        public object CreateInstance()
        {
            if (WaitForLoaded())
                return null;

            var assemblies = AppDomain.CurrentDomain.GetAssemblies();
            var dataTypeName = DataTypeName;

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
                            Debug.LogException(ex);
                        }

                        return obj;
                    }
                }
            }

            return null;
        }
    }
}
