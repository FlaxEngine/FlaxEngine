using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using FlaxEngine.GUI;
using System.Reflection;
using System.Text;
using FlaxEditor.Content;
using Newtonsoft.Json;
namespace FlaxEngine
{
    /// <summary>
    /// [todo] add summary here
    /// </summary>
    public class UIBlueprintAsset
    {
        /// <summary>
        /// 
        /// </summary>
        public UIBlueprintAsset()
        {
            Root = new ContainerControl();
        }
        /// <summary>
        /// root control
        /// </summary>
        public Control Root;
    }
#if FLAX_EDITOR
    /// <inheritdoc/>
    public class UIBlueprintAssetItem : JsonAssetItem
    {
        /// <inheritdoc/>
        public UIBlueprintAssetItem(string path, Guid id, string typeName) : base(path, id, typeName){}
        /// <inheritdoc/>
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Widget;
    }
    internal class UIElementRegistry
    {
        //custom game Widgets
        internal static List<Type> GameWidgets = new List<Type>();
        //custom Editor Widgets
        internal static List<Type> EditorWidgets = new List<Type>();
        //shared standard UI
        internal static List<Type> SharedWidgets = new List<Type>();
        internal static void Init()
        {
            SharedWidgets.AddRange(GetClasses("FlaxEngine.GUI"));
            GameWidgets.AddRange(GetClasses("FlaxEngine.Widgets"));
            EditorWidgets.AddRange(GetClasses("FlaxEditor.Widgets"));
            EditorWidgets.AddRange(GetClasses("FlaxEditor.GUI"));
        }
        internal static IEnumerable<Type> GetClasses(string nameSpace)
        {
            Assembly asm = Assembly.GetExecutingAssembly();
            return asm.GetTypes()
                .Where(type => type.Namespace == nameSpace)
                .Select(type => type);
        }
        internal static UIControl CreateWidgetInstance(Type type)
        {
            return (UIControl)Activator.CreateInstance(type);
        }
    }
#endif
}

