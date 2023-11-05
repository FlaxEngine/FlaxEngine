using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using FlaxEditor.Viewport;
using FlaxEngine.GUI;
using System.Reflection;
using FlaxEditor.Content;
using FlaxEditor.Surface.Elements;

namespace FlaxEngine
{
    /// <summary>
    /// [todo] add summary here
    /// </summary>
    public class Widget
    {
        public Vector2[] SupportedResolutions =
        {
        new Vector2(1280, 720),
        new Vector2(1920, 1080),
    };

        public string DefaultLanguage = "en";
    }
    /// <inheritdoc/>
    public class WidgetItem : JsonAssetItem
    {
        /// <inheritdoc/>
        public WidgetItem(string path, Guid id, string typeName) : base(path, id, typeName)
        {
        }

        

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
}
