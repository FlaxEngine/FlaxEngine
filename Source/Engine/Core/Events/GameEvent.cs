using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Reflection;

namespace FlaxEngine
{
    /// <summary>
    /// Game Action Delegate
    /// </summary>
    public delegate void GameAction();
    /// <summary>
    /// Generic Game Action Delegate
    /// </summary>
    /// <typeparam name="T0">The Target Type</typeparam>
    /// <param name="value">Value to set for the target type</param>
    public delegate void GameAction<T0>(T0 value);

    /// <summary>
    /// Game Event, similiar to System.Action, but has GUI representation in the editor, while retaining the possibility to use with scripting
    /// </summary>
    public class GameEventBase
    {
        /// <summary>
        /// Main Method, stores all the needed info for this Method/Poperty/Member
        /// </summary>
        [Serializable]
        public struct SerializedMethod
        {
            /// <summary>
            /// Whether this is Actor or Script
            /// </summary>
            public enum SerializedActorType{
                /// <summary>
                /// Actor
                /// </summary>
                Actor,
                /// <summary>
                /// Script
                /// </summary>
                Script
            }
            /// <summary>
            /// Whether this is Method, Poperty or Field
            /// </summary>
            public enum SerializedActorValueType{
                /// <summary>
                /// Invalid
                /// </summary>
                None,
                /// <summary>
                /// Method
                /// </summary>
                Method, 
                /// <summary>
                /// Property
                /// </summary>
                Property,
                /// <summary>
                /// Filed
                /// </summary>
                Field
            }
            /// <summary>
            /// Holds all posibilities MethodINfo, FIledInfo, even PropertyInfo
            /// </summary>
            public struct SerInfos
            {
                /// <summary>
                /// MethodInfo if we are for Method
                /// </summary>
                public MethodInfo minfo;
                /// <summary>
                /// MethodInfo if we are for Method
                /// </summary>
                public FieldInfo finfo;
                /// <summary>
                /// MethodInfo if we are for Method
                /// </summary>
                public PropertyInfo pinfo;

                /// <summary>
                /// Gets the value type that we are
                /// </summary>
                /// <returns>The value type we are</returns>
                public SerializedActorValueType GetValueType()
                {
                    if (minfo != null)
                        return SerializedActorValueType.Method;
                    else if (finfo != null)
                        return SerializedActorValueType.Field;
                    else if (pinfo != null)
                        return SerializedActorValueType.Property;
                    else
                        return SerializedActorValueType.None;
                }
                /// <summary>
                /// Gets the name
                /// </summary>
                /// <returns>The name</returns>
                public string GetValueName()
                {
                    switch (GetValueType())
                    {
                        case SerializedActorValueType.Method:
                            return minfo.Name;
                        case SerializedActorValueType.Field:
                            return finfo.Name;
                        case SerializedActorValueType.Property:
                            return pinfo.Name;
                        default:
                            return null;
                    }
                }
                /// <summary>
                /// Gets the target type of this Member/Propery/Method(in case of Method its first attribute)
                /// </summary>
                /// <returns></returns>
                public Type GetParOrValueType()
                {
                    switch (GetValueType())
                    {
                        case SerializedActorValueType.Method:
                            {
                                List<ParameterInfo> pinfos = minfo?.GetParameters()?.ToList();
                                if (pinfos == null || pinfos.Count == 0)
                                    return null;
                                return pinfos[0].ParameterType;
                            }                            
                        case SerializedActorValueType.Field:
                            return finfo?.FieldType;
                        case SerializedActorValueType.Property:
                            return pinfo?.PropertyType;
                        default:
                            return null;
                    }
                }
            }
            /// <summary>
            /// Applies SerInfo to synchornize itself with it
            /// </summary>
            /// <param name="info"></param>
            public void ApplySerInfo(SerInfos info)
            {
                ValueName = info.GetValueName();
                valueType = info.GetValueType();
                if(SavedValue?.GetType() != info.GetParOrValueType())
                {
                    Type theType = info.GetParOrValueType();
                    if (theType == null || !theType.IsValueType)
                        SavedValue = null;
                    else
                        SavedValue = Activator.CreateInstance(theType);
                }
                if(info.GetParOrValueType() != allowedDynamicType)
                {
                    IsDynamic = false;
                }
            }
            /// <summary>
            /// Gets the SerInfo for this Object
            /// </summary>
            /// <returns></returns>
            public SerInfos GetSerInfos()
            {
                SerInfos infos = new SerInfos();

                Type finalType = GetInvoker()?.GetType();

                string valName = ValueName;

                switch (valueType)
                {
                    case SerializedActorValueType.Method:
                        {
                            List< MethodInfo> methodInfos = finalType?.GetMethods().Where(mi=>mi.Name.Equals(valName)).ToList();
                            if (methodInfos.Count > 1)
                                methodInfos = methodInfos.Where((mi) => mi.GetParameters().Count() < 2).ToList();

                            infos.minfo = methodInfos.FirstOrDefault();
                            break;
                        }
                    case SerializedActorValueType.Field:
                        {
                            infos.finfo = finalType?.GetFields().SingleOrDefault(mi => mi.Name.Equals(valName));
                            break;
                        }
                    case SerializedActorValueType.Property:
                        {
                            infos.pinfo = finalType?.GetProperties().SingleOrDefault(mi => mi.Name.Equals(valName));
                            break;
                        }
                }
                return infos;
            }
            [Serialize]
            object savedValue;
            [Serialize]
            System.Guid savedValueIfObject;
            [NoSerialize]
            FlaxEngine.Object savedValueCache;
            /// <summary>
            /// The saved value, int, string, whatever
            /// </summary>
            [NoSerialize]
            public object SavedValue
            {
                get
                {
                    if (savedValueIfObject != null && savedValueIfObject != Guid.Empty)
                    {
                        if(savedValueCache == null)
                        {
                            savedValueCache = FlaxEngine.Object.TryFind<FlaxEngine.Object>(ref savedValueIfObject);
                        }                            
                        return savedValueCache;
                    }
                    else
                    {
                        return savedValue;
                    }
                }
                set
                {
                    savedValue = null;
                    savedValueIfObject = Guid.Empty;

                    if (value is FlaxEngine.Object){
                        savedValueIfObject = ((FlaxEngine.Object)value).ID;
                    }
                    else
                    {
                        savedValue = value;
                    }
                }
            }
            /// <summary>
            /// If the saved value is dynamic
            /// </summary>
            public bool IsDynamic;
            /// <summary>
            /// The name of the value like MemberName/ProperyName/MethodName
            /// </summary>
            public string ValueName;

            /// <summary>
            /// Our type
            /// </summary>
            public SerializedActorType type;
            /// <summary>
            /// Our Value type
            /// </summary>
            public SerializedActorValueType valueType;

            /// <summary>
            /// Makes the Values name fancy, like in case of Method we add the nice ()
            /// </summary>
            /// <returns></returns>
            public string GetDisplayValueName()
            {
                switch (valueType)
                {
                    case SerializedActorValueType.Method:
                        return GetInvoker().GetType().Name + "." + ValueName + "()";
                    case SerializedActorValueType.Field:
                    case SerializedActorValueType.Property:
                        return GetInvoker().GetType().Name  + "." + ValueName;
                    default:
                        return "Not set";
                }
            }

            /// <summary>
            /// We get the invoker, Actor in case of Actor, Script in case if a script
            /// </summary>
            /// <returns>The invoker</returns>
            public object GetInvoker()
            {
                if (type == SerializedActorType.Actor)
                    return Actor;
                else if (type == SerializedActorType.Script)
                    return script;
                else
                    return null;
            }

            /// <summary>
            /// Hold the target actor
            /// </summary>
            public Actor Actor;
            /// <summary>
            /// Holds the target script, if any
            /// </summary>
            public Script script;

            /// <summary>
            /// We get the actor, if we are actor its simple if script its his parent
            /// </summary>
            /// <returns></returns>
            public Actor GetActor()
            {
                if (type == SerializedActorType.Actor)
                    return Actor;
                else if (type == SerializedActorType.Script)
                    return script.Actor;
                else
                    return null;
            }
            /// <summary>
            /// Returns dynamic value if it should or the saved value if it shouldnt, note that there is a bug with serialization where int becomes long so we need to convert back
            /// </summary>
            /// <param name="dynamic">the dynamic vlaue to use in case we should</param>
            /// <param name="targetType">the target type</param>
            /// <returns></returns>
            public object GetSavedValueOrDynamic(object dynamic, Type targetType)
            {
                if (IsDynamic)
                    return dynamic;
                else
                {
                    if (SavedValue == null)
                        return null;
                    if (targetType != null && SavedValue.GetType() == typeof(long) && targetType != typeof(long))
                        SavedValue = (int)(long)SavedValue;
                    return SavedValue;
                }                    
            }
            /// <inheritdoc/>
            public override bool Equals(object obj)
            {
                if(obj is SerializedMethod otherSerMethod)
                {
                    bool valEq = SavedValue == otherSerMethod.SavedValue && ValueName == otherSerMethod.ValueName;
                    bool typesQr = type == otherSerMethod.type && valueType == otherSerMethod.valueType;
                    bool invokersEq = Actor == otherSerMethod.Actor && script == otherSerMethod.script;
                    bool dynamicEq = IsDynamic == otherSerMethod.IsDynamic;
                    return valEq && typesQr && invokersEq && dynamicEq;
                }
                else
                    return false;
            }

            /// <inheritdoc/>
            public override int GetHashCode()
            {
                int hash = 17;
                if(SavedValue != null)
                    hash = hash * 23 + SavedValue.GetHashCode();
                if(ValueName != null)
                    hash = hash * 23 + ValueName.GetHashCode();
                if(Actor != null)
                    hash = hash * 23 + Actor.GetHashCode();
                if(script != null)
                    hash = hash * 23 + script.GetHashCode();
                hash = hash * 23 + IsDynamic.GetHashCode();
                return hash;
            }
            /// <summary>
            /// The dynamic type that is allowed
            /// </summary>
            [NoSerialize]
            public Type allowedDynamicType;

            /// <summary>
            /// Invokes the method or sets the value in case of Field or Property
            /// </summary>
            /// <param name="dynamicValue"></param>
            public void Invoke(object dynamicValue = null)
            {
                object invoker = GetInvoker();
                SerInfos info = GetSerInfos();

                switch (info.GetValueType())
                {
                    case SerializedActorValueType.Method:
                        {
                            object[] parameters = new object[] { GetSavedValueOrDynamic(dynamicValue, info.GetParOrValueType()) };
                            if (parameters[0] == null || info.minfo.GetParameters().Count() == 0)
                                parameters = null;
                            try
                            {
                                info.minfo.Invoke(invoker, parameters);
                            }
                            catch (Exception e)
                            {
                                Debug.LogWarning("Failed to invoke method of GameEvent on " + GetActor() + " with ValueName: " + GetDisplayValueName() + ", reason: " + e.Message);
                                Debug.LogWarning(e.StackTrace);
                            }
                            break;
                        }
                    case SerializedActorValueType.Property:
                        {
                            try
                            {
                                info.pinfo.SetValue(invoker, GetSavedValueOrDynamic(dynamicValue, info.GetParOrValueType()));
                            }
                            catch (Exception e)
                            {
                                Debug.LogWarning("Failed to set property of GameEvent on " + GetActor() + " with ValueName: " + GetDisplayValueName() + ", reason: " + e.Message);
                                Debug.LogWarning(e.StackTrace);
                            }
                            break;
                        }
                    case SerializedActorValueType.Field:
                        {
                            try
                            {
                                info.finfo.SetValue(invoker, GetSavedValueOrDynamic(dynamicValue, info.GetParOrValueType()));
                            }
                            catch (Exception e)
                            {
                                Debug.LogWarning("Failed to set field of GameEvent on " + GetActor() + " with ValueName: " + GetDisplayValueName() + ", reason: " + e.Message);
                                Debug.LogWarning(e.StackTrace);
                            }
                            break;
                        }
                    default:
                        {
                            Debug.LogWarning("Could not deserialize GameEvent on  " + GetActor() + " with valueName " + ValueName);
                            break;
                        }
                }
            }
        }
    }
    /// <summary>
    /// Custom list for SerMethods
    /// </summary>
    /// <typeparam name="T0">The target Dyanmic Type</typeparam>
    public class SerMethList<T0> : List<GameEventBase.SerializedMethod> { }

    /// <summary>
    /// Void GameEvent
    /// </summary>
    public class GameEvent : GameEventBase
    {
        List<GameAction> actions = new List<GameAction>();
        /// <summary>
        /// Adds script listener for this action trigger
        /// </summary>
        /// <param name="action">The listener</param>
        public void AddListener(GameAction action)
        {
            actions.Add(action);
        }
        /// <summary>
        /// Clears all subscibed listeners(does not reflect the gui created ones)
        /// </summary>
        public void ClearAllListeners()
        {
            actions.Clear();
        }
        /// <summary>
        /// void to use in SerMethList
        /// </summary>
        public class GameEventVoid { }

        /// <summary>
        /// Gui listeners
        /// </summary>
        public SerMethList<GameEventVoid> actorMethods = new SerMethList<GameEventVoid>();

        /// <summary>
        /// Invokes both GUI methods and Scripted methods
        /// </summary>
        public void Invoke()
        {
            foreach (SerializedMethod actorMethod in actorMethods)
            {
                actorMethod.Invoke();
            }
            foreach (GameAction action in actions)
            {
                action?.Invoke();
            }
        }    
    }
    
    /// <summary>
    /// Generic GameEvent
    /// </summary>
    /// <typeparam name="T0">The Generic type</typeparam>
    public class GameEvent<T0> : GameEventBase
    {
        List<GameAction<T0>> actions = new List<GameAction<T0>>();
        /// <summary>
        /// Adds script listener for this action trigger
        /// </summary>
        /// <param name="action">The listener</param>
        public void AddListener(GameAction<T0> action)
        {
            actions.Add(action);
        }
        /// <summary>
        /// Clears all subscibed listeners(does not reflect the gui created ones)
        /// </summary>
        public void ClearAllListeners()
        {
            actions.Clear();
        }

        /// <summary>
        /// Gui listeners
        /// </summary>
        public SerMethList<T0> actorMethods = new SerMethList<T0>();

        /// <summary>
        /// Invokes both GUI methods and Scripted methods
        /// </summary>
        public void Invoke(T0 val)
        {
            foreach (SerializedMethod actorMethod in actorMethods)
            {
                actorMethod.Invoke(val);
            }
            foreach (GameAction<T0> action in actions)
            {
                action?.Invoke(val);
            }
        }
    }
}
