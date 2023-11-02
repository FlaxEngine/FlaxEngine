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

namespace FlaxEngine.Content
{
    /// <summary>
    /// [todo] add summary here
    /// </summary>
    public class WidgetData : RawDataAsset
    {
        internal struct WidgetDataSerializer
        {
            internal Control control;
            internal List<WidgetDataSerializer> controlTypes;

            public WidgetDataSerializer()
            {
                controlTypes = new();
            }

            internal void WriteControl(ref List<byte> rawStringData)
            {
                rawStringData.AddRange(Encoding.ASCII.GetBytes("{\n"));
                rawStringData.AddRange(Encoding.ASCII.GetBytes(control.GetType().FullName + "\n"));
                rawStringData.AddRange(Encoding.ASCII.GetBytes(control.Location.X.ToString() + "\n"));
                rawStringData.AddRange(Encoding.ASCII.GetBytes(control.Location.Y.ToString() + "\n"));
                rawStringData.AddRange(Encoding.ASCII.GetBytes(control.Size.X.ToString() + "\n"));
                rawStringData.AddRange(Encoding.ASCII.GetBytes(control.Size.Y.ToString() + "\n"));
                rawStringData.AddRange(Encoding.ASCII.GetBytes(control.AnchorMax.X.ToString() + "\n"));
                rawStringData.AddRange(Encoding.ASCII.GetBytes(control.AnchorMin.Y.ToString() + "\n"));
                rawStringData.AddRange(Encoding.ASCII.GetBytes(control.Rotation.ToString() + "\n"));
                rawStringData.AddRange(Encoding.ASCII.GetBytes("}\n"));
            }
        }
        internal ulong WidgetCount;
        WidgetDataSerializer Serializer;
        List<byte> rawStringData;
        WidgetData() : base()
        {

        }
        void Serialize(UICanvas canvas)
        {
            WidgetCount = 0;
            rawStringData.Clear();
            Serializer.controlTypes.Clear();
            Serializer.control = canvas._guiRoot;
            ToControlType(Serializer, canvas._guiRoot.Children);
        }
        /// <summary>
        /// Deserializes the assets and creates new instance of <see cref="UICanvas"/>
        /// </summary>
        public UICanvas Deserialize()
        {
            //[to do] deseralize and create UI
            return null;
        }
        private void ToControlType(WidgetDataSerializer serializer, List<Control> controls)
        {
            for (int i = 0; i < controls.Count; i++)
            {
                if (controls[i] is ContainerControl control)
                {
                    serializer.controlTypes.Add(new WidgetDataSerializer() { control = control});
                    ToControlType(serializer.controlTypes[i], control.Children);
                    WidgetCount++;
                }
            }
        }
        /// <summary>
        /// Saves this asset to the file. Supported only in Editor.
        /// </summary>
        /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
        /// <returns>True if failed, otherwise false.</returns>
        [Unmanaged]
        public new bool Save(string path = null) // overload the save
        {
            Serializer.WriteControl(ref rawStringData);
            Data = rawStringData.ToArray();
            return Internal_Save(__unmanagedPtr, path);
        }
    }
    /// <inheritdoc/>
    public class Widget : AssetItem
    {
        /// <inheritdoc/>
        public Widget(string path, string typeName, ref Guid id) : base(path, typeName, ref id)
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
