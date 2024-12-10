// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Json;
using FlaxEngine.Utilities;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Content
{
    sealed class VisualScriptParameterInfo : IScriptMemberInfo
    {
        private readonly VisualScriptType _type;
        private readonly VisjectGraphParameter _parameter;

        internal VisualScriptParameterInfo(VisualScriptType type, VisjectGraphParameter parameter)
        {
            _type = type;
            _parameter = parameter;
        }

        /// <inheritdoc />
        public string Name => _parameter.Name;

        /// <inheritdoc />
        public bool IsPublic => _parameter.IsPublic;

        /// <inheritdoc />
        public bool IsStatic => false;

        /// <inheritdoc />
        public bool IsVirtual => false;

        /// <inheritdoc />
        public bool IsAbstract => false;

        /// <inheritdoc />
        public bool IsGeneric => false;

        /// <inheritdoc />
        public bool IsField => true;

        /// <inheritdoc />
        public bool IsProperty => false;

        /// <inheritdoc />
        public bool IsMethod => false;

        /// <inheritdoc />
        public bool IsEvent => false;

        /// <inheritdoc />
        public bool HasGet => true;

        /// <inheritdoc />
        public bool HasSet => true;

        /// <inheritdoc />
        public int ParametersCount => 0;

        /// <inheritdoc />
        public ScriptType DeclaringType => new ScriptType(_type);

        /// <inheritdoc />
        public ScriptType ValueType
        {
            get
            {
                var type = TypeUtils.GetType(_parameter.TypeTypeName);
                return type ? type : new ScriptType(_parameter.Type);
            }
        }

        /// <inheritdoc />
        public int MetadataToken => 0;

        /// <inheritdoc />
        public bool HasAttribute(Type attributeType, bool inherit)
        {
            return Surface.SurfaceMeta.HasAttribute(_parameter, attributeType);
        }

        /// <inheritdoc />
        public object[] GetAttributes(bool inherit)
        {
            return Surface.SurfaceMeta.GetAttributes(_parameter);
        }

        /// <inheritdoc />
        public ScriptMemberInfo.Parameter[] GetParameters()
        {
            return null;
        }

        /// <inheritdoc />
        public object GetValue(object obj)
        {
            if (!_type.Asset)
                throw new TargetException("Missing Visual Script asset.");
            return _type.Asset.GetScriptInstanceParameterValue(_parameter.Name, (Object)obj);
        }

        /// <inheritdoc />
        public void SetValue(object obj, object value)
        {
            if (!_type.Asset)
                throw new TargetException("Missing Visual Script asset.");
            _type.Asset.SetScriptInstanceParameterValue(_parameter.Name, (Object)obj, value);
        }

        /// <inheritdoc />
        public object Invoke(object obj, object[] parameters)
        {
            throw new NotSupportedException();
        }
    }

    sealed class VisualScriptMethodInfo : IScriptMemberInfo
    {
        [Flags]
        private enum Flags
        {
            None = 0,
            Static = 1,
            Virtual = 2,
            Override = 4,
        }

        private readonly VisualScriptType _type;
        private readonly int _index;
        private readonly string _name;
        private readonly byte _flags;
        private readonly ScriptType _returnType;
        private readonly ScriptMemberInfo.Parameter[] _parameters;
        private object[] _attributes;

        internal VisualScriptMethodInfo(VisualScriptType type, int index)
        {
            _type = type;
            _index = index;
            type.Asset.GetMethodSignature(index, out _name, out _flags, out var returnTypeName, out var paramNames, out var paramTypeNames, out var paramOuts);
            _returnType = TypeUtils.GetType(returnTypeName);
            if (paramNames != null && paramNames.Length != 0)
            {
                _parameters = new ScriptMemberInfo.Parameter[paramNames.Length];
                for (int i = 0; i < _parameters.Length; i++)
                {
                    _parameters[i] = new ScriptMemberInfo.Parameter
                    {
                        Name = paramNames[i],
                        Type = TypeUtils.GetType(paramTypeNames[i]),
                        IsOut = paramOuts[i],
                    };
                }
            }
            else
            {
                _parameters = Utils.GetEmptyArray<ScriptMemberInfo.Parameter>();
            }
        }

        /// <inheritdoc />
        public string Name => _name;

        /// <inheritdoc />
        public bool IsPublic => true;

        /// <inheritdoc />
        public bool IsStatic => (_flags & (byte)Flags.Static) != 0;

        /// <inheritdoc />
        public bool IsVirtual => (_flags & (byte)Flags.Virtual) != 0;

        /// <inheritdoc />
        public bool IsAbstract => false;

        /// <inheritdoc />
        public bool IsGeneric => false;

        /// <inheritdoc />
        public bool IsField => false;

        /// <inheritdoc />
        public bool IsProperty => false;

        /// <inheritdoc />
        public bool IsMethod => true;

        /// <inheritdoc />
        public bool IsEvent => false;

        /// <inheritdoc />
        public bool HasGet => false;

        /// <inheritdoc />
        public bool HasSet => false;

        /// <inheritdoc />
        public int ParametersCount => _parameters.Length;

        /// <inheritdoc />
        public ScriptType DeclaringType => (_flags & (byte)Flags.Override) != 0 ? _type.BaseType : new ScriptType(_type);

        /// <inheritdoc />
        public ScriptType ValueType => _returnType;

        public int MetadataToken => 0;

        /// <inheritdoc />
        public bool HasAttribute(Type attributeType, bool inherit)
        {
            return GetAttributes(inherit).Any(x => x.GetType() == attributeType);
        }

        /// <inheritdoc />
        public object[] GetAttributes(bool inherit)
        {
            if (_attributes == null)
            {
                var data = _type.Asset.GetMethodMetaData(_index, Surface.SurfaceMeta.AttributeMetaTypeID);
                var dataOld = _type.Asset.GetMethodMetaData(_index, Surface.SurfaceMeta.OldAttributeMetaTypeID);
                _attributes = Surface.SurfaceMeta.GetAttributes(data, dataOld);
            }
            return _attributes;
        }

        /// <inheritdoc />
        public ScriptMemberInfo.Parameter[] GetParameters()
        {
            return _parameters;
        }

        /// <inheritdoc />
        public object GetValue(object obj)
        {
            throw new NotSupportedException();
        }

        /// <inheritdoc />
        public void SetValue(object obj, object value)
        {
            throw new NotSupportedException();
        }

        /// <inheritdoc />
        public object Invoke(object obj, object[] parameters)
        {
            if (!_type.Asset)
                throw new TargetException("Missing Visual Script asset.");
            return _type.Asset.InvokeMethod(_index, obj, parameters);
        }
    }

    /// <summary>
    /// The implementation of the <see cref="IScriptType"/>
    /// </summary>
    /// <seealso cref="FlaxEditor.Scripting.IScriptType" />
    [HideInEditor]
    public sealed class VisualScriptType : IScriptType
    {
        private VisualScript _asset;
        private ScriptMemberInfo[] _parameters;
        private ScriptMemberInfo[] _methods;
        private object[] _attributes;
        private List<Action<ScriptType>> _disposing;

        /// <summary>
        /// Gets the Visual Script asset that contains this type.
        /// </summary>
        public VisualScript Asset => _asset;

        internal VisualScriptType(VisualScript asset)
        {
            _asset = asset;
        }

        private void CacheData()
        {
            if (_parameters != null)
                return;
            if (_asset.WaitForLoaded())
                return;
            FlaxEngine.Content.AssetReloading += OnAssetReloading;

            // Cache Visual Script parameters info
            var parameters = _asset.Parameters;
            if (parameters.Length != 0)
            {
                _parameters = new ScriptMemberInfo[parameters.Length];
                for (int i = 0; i < parameters.Length; i++)
                    _parameters[i] = new ScriptMemberInfo(new VisualScriptParameterInfo(this, parameters[i]));
            }
            else
                _parameters = Utils.GetEmptyArray<ScriptMemberInfo>();

            // Cache Visual Script methods info
            var methodsCount = _asset.GetMethodsCount();
            if (methodsCount != 0)
            {
                _methods = new ScriptMemberInfo[methodsCount];
                for (int i = 0; i < methodsCount; i++)
                    _methods[i] = new ScriptMemberInfo(new VisualScriptMethodInfo(this, i));
            }
            else
                _methods = Utils.GetEmptyArray<ScriptMemberInfo>();

            // Cache Visual Script attributes
            {
                var data = _asset.GetMetaData(Surface.SurfaceMeta.AttributeMetaTypeID);
                var dataOld = _asset.GetMetaData(Surface.SurfaceMeta.OldAttributeMetaTypeID);
                _attributes = Surface.SurfaceMeta.GetAttributes(data, dataOld);
            }
        }

        private void OnAssetReloading(Asset asset)
        {
            if (asset == _asset)
            {
                _parameters = null;
                _methods = null;
                FlaxEngine.Content.AssetReloading -= OnAssetReloading;
            }
        }

        internal void Dispose()
        {
            if (_disposing != null)
            {
                foreach (var e in _disposing)
                    e(new ScriptType(this));
                _disposing.Clear();
                _disposing = null;
            }
            if (_parameters != null)
            {
                OnAssetReloading(_asset);
            }
            _asset = null;
        }

        /// <inheritdoc />
        public string Name => Path.GetFileNameWithoutExtension(_asset.Path);

        /// <inheritdoc />
        public string Namespace => string.Empty;

        /// <inheritdoc />
        public string TypeName => JsonSerializer.GetStringID(_asset.ID);

        /// <inheritdoc />
        public bool IsPublic => true;

        /// <inheritdoc />
        public bool IsAbstract => _asset && !_asset.WaitForLoaded() && (_asset.Meta.Flags & VisualScript.Flags.Abstract) != 0;

        /// <inheritdoc />
        public bool IsSealed => _asset && !_asset.WaitForLoaded() && (_asset.Meta.Flags & VisualScript.Flags.Sealed) != 0;

        /// <inheritdoc />
        public bool IsEnum => false;

        /// <inheritdoc />
        public bool IsClass => true;

        /// <inheritdoc />
        public bool IsInterface => false;

        /// <inheritdoc />
        public bool IsArray => false;

        /// <inheritdoc />
        public bool IsValueType => false;

        /// <inheritdoc />
        public bool IsGenericType => false;

        /// <inheritdoc />
        public bool IsReference => false;

        /// <inheritdoc />
        public bool IsPointer => false;

        /// <inheritdoc />
        public bool IsStatic => false;

        /// <inheritdoc />
        public bool CanCreateInstance => !IsAbstract;

        /// <inheritdoc />
        public ScriptType BaseType => _asset && !_asset.WaitForLoaded() ? TypeUtils.GetType(_asset.Meta.BaseTypename) : ScriptType.Null;

        /// <inheritdoc />
        public ContentItem ContentItem => _asset ? Editor.Instance.ContentDatabase.FindAsset(_asset.ID) : null;

        /// <inheritdoc />
        public object CreateInstance()
        {
            return Object.New(TypeName);
        }

        /// <inheritdoc />
        public bool HasInterface(ScriptType c)
        {
            return BaseType.HasInterface(c);
        }

        /// <inheritdoc />
        public bool HasAttribute(Type attributeType, bool inherit)
        {
            if (inherit && BaseType.HasAttribute(attributeType, true))
                return true;
            CacheData();
            return _attributes.Any(x => x.GetType() == attributeType);
        }

        /// <inheritdoc />
        public object[] GetAttributes(bool inherit)
        {
            CacheData();
            if (inherit)
            {
                var thisAttributes = _attributes;
                var baseAttributes = BaseType.GetAttributes(true);
                if (thisAttributes.Length != 0)
                {
                    if (baseAttributes.Length != 0)
                    {
                        var resultAttributes = new object[thisAttributes.Length + baseAttributes.Length];
                        Array.Copy(thisAttributes, 0, resultAttributes, 0, thisAttributes.Length);
                        Array.Copy(baseAttributes, 0, resultAttributes, thisAttributes.Length, baseAttributes.Length);
                        return resultAttributes;
                    }
                    return thisAttributes;
                }
                return baseAttributes;
            }
            return _attributes;
        }

        /// <inheritdoc />
        public ScriptMemberInfo[] GetMembers(string name, MemberTypes type, BindingFlags bindingAttr)
        {
            var members = new List<ScriptMemberInfo>();
            if ((bindingAttr & BindingFlags.DeclaredOnly) == 0)
            {
                var baseType = BaseType;
                if (baseType)
                    members.AddRange(baseType.GetMembers(name, type, bindingAttr));
            }
            CacheData();
            if ((type & MemberTypes.Field) != 0)
            {
                foreach (var parameter in _parameters)
                {
                    if (parameter.Filter(name, bindingAttr))
                        members.Add(parameter);
                }
            }
            if ((type & MemberTypes.Method) != 0)
            {
                foreach (var method in _methods)
                {
                    if (method.Filter(name, bindingAttr))
                        members.Add(method);
                }
            }
            return members.ToArray();
        }

        /// <inheritdoc />
        public ScriptMemberInfo[] GetMembers(BindingFlags bindingAttr)
        {
            var members = new List<ScriptMemberInfo>();
            if ((bindingAttr & BindingFlags.DeclaredOnly) == 0)
            {
                var baseType = BaseType;
                if (baseType)
                    members.AddRange(baseType.GetMembers(bindingAttr));
            }
            CacheData();
            foreach (var parameter in _parameters)
            {
                if (parameter.Filter(bindingAttr))
                    members.Add(parameter);
            }
            foreach (var method in _methods)
            {
                if (method.Filter(bindingAttr))
                    members.Add(method);
            }
            return members.ToArray();
        }

        /// <inheritdoc />
        public ScriptMemberInfo[] GetFields(BindingFlags bindingAttr)
        {
            CacheData();
            var baseType = BaseType;
            if (baseType)
            {
                var baseFields = baseType.GetFields(bindingAttr);
                var newArray = new ScriptMemberInfo[_parameters.Length + baseFields.Length];
                Array.Copy(_parameters, newArray, _parameters.Length);
                Array.Copy(baseFields, 0, newArray, _parameters.Length, baseFields.Length);
                return newArray;
            }
            return _parameters;
        }

        /// <inheritdoc />
        public ScriptMemberInfo[] GetProperties(BindingFlags bindingAttr)
        {
            var baseType = BaseType;
            if (baseType)
                return baseType.GetProperties(bindingAttr);
            return Utils.GetEmptyArray<ScriptMemberInfo>();
        }

        /// <inheritdoc />
        public ScriptMemberInfo[] GetMethods(BindingFlags bindingAttr)
        {
            CacheData();
            var baseType = BaseType;
            if (baseType)
            {
                var baseMethods = baseType.GetMethods(bindingAttr);
                var newArray = new ScriptMemberInfo[_methods.Length + baseMethods.Length];
                Array.Copy(_methods, newArray, _methods.Length);
                Array.Copy(baseMethods, 0, newArray, _methods.Length, baseMethods.Length);
                return newArray;
            }
            return _methods;
        }

        /// <inheritdoc />
        public void TrackLifetime(Action<ScriptType> disposing)
        {
            if (_disposing == null)
                _disposing = new List<Action<ScriptType>>();
            _disposing.Add(disposing);
        }
    }

    /// <summary>
    /// Implementation of <see cref="BinaryAssetItem"/> for <see cref="VisualScript"/> assets.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetItem" />
    public class VisualScriptItem : BinaryAssetItem
    {
        /// <summary>
        /// The cached list of all Visual Script assets found in the workspace (engine, game and plugin projects). Updated on workspace modifications.
        /// </summary>
        public static readonly List<VisualScriptItem> VisualScripts = new List<VisualScriptItem>();

        private VisualScriptType _scriptType;

        /// <summary>
        /// The Visual Script type. Can be null if failed to load asset.
        /// </summary>
        public ScriptType ScriptType
        {
            get
            {
                if (_scriptType == null)
                {
                    var asset = FlaxEngine.Content.LoadAsync<VisualScript>(ID);
                    if (asset)
                        _scriptType = new VisualScriptType(asset);
                }
                return new ScriptType(_scriptType);
            }
        }

        /// <inheritdoc />
        public VisualScriptItem(string path, ref Guid id, string typeName, Type type)
        : base(path, ref id, typeName, type, ContentItemSearchFilter.Script)
        {
            VisualScripts.Add(this);
            Editor.Instance.CodeEditing.ClearTypes();
        }

        /// <inheritdoc />
        public override bool OnEditorDrag(object context)
        {
            return new ScriptType(typeof(Actor)).IsAssignableFrom(ScriptType) && ScriptType.CanCreateInstance;
        }

        /// <inheritdoc />
        public override Actor OnEditorDrop(object context)
        {
            return (Actor)ScriptType.CreateInstance();
        }

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => Editor.Instance.Icons.VisualScript128;

        /// <inheritdoc />
        protected override bool DrawShadow => false;

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            if (_scriptType != null)
            {
                Editor.Instance.CodeEditing.ClearTypes();
                _scriptType.Dispose();
                _scriptType = null;
            }
            VisualScripts.Remove(this);

            base.OnDestroy();
        }
    }
}
