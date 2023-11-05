using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using FlaxEngine.GUI;
namespace FlaxEngine
{
    /// <summary>
    /// UI Raycaster
    /// </summary>
    public class UIRaycaster
    {
        /// <summary>
        /// raycast for element
        /// </summary>
        /// <param name="from"></param>
        /// <param name="point"></param>
        /// <param name="hit"></param>
        /// <param name="ignoreHidedn"></param>
        /// <returns></returns>
        public static bool CastRay(ContainerControl from, ref Float2 point, ref Control hit,bool ignoreHidedn = false)
        {
            bool b = false;
            foreach (var item in from.Children)
            {
                if (ignoreHidedn && !item.Visible)
                    continue;
                var localpoint = item.PointFromParent(ref point);
                if (item is ContainerControl c)
                {
                    if (item.ContainsPoint(ref localpoint))
                    {
                        hit = item;
                        CastRay(c, ref localpoint, ref hit, ignoreHidedn);
                        b = true;
                        continue;
                    }
                }
                if (item.ContainsPoint(ref localpoint))
                {
                    hit = item;
                    b = true;
                }
            }
            return b;
        }
        /// <summary>
        /// raycast for elements
        /// </summary>
        /// <param name="from"></param>
        /// <param name="point"></param>
        /// <param name="hit"></param>
        /// <param name="ignoreHidedn"></param>
        /// <returns></returns>
        public static bool CastRay(ContainerControl from, ref Float2 point, ref List<Control> hit, bool ignoreHidedn = false)
        {
            bool b = false;
            foreach (var item in from.Children)
            {
                if (ignoreHidedn && !item.Visible)
                    continue;
                if (item is ContainerControl c)
                {
                    if (item.ContainsPoint(ref point))
                    {
                        hit.Add(item);
                        if (CastRay(c, ref point, ref hit, ignoreHidedn))
                        {
                            b = true;
                            hit.Add(item);
                        }
                    }
                }
                else
                {
                    if (item.ContainsPoint(ref point))
                    {
                        hit.Add(item);
                        b = true;
                    }
                }
            }
            return b;
        }
    }
}
