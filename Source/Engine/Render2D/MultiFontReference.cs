

using System.Collections.Generic;
using System.Linq;

namespace FlaxEngine
{
    /// <summary>
    /// Reference to multiple font references
    /// </summary>
    public class MultiFontReference : List<FontReference>
    {
        public MultiFontReference()
        {
            _cachedFont = null;
        }

        public MultiFontReference(IEnumerable<FontReference> other)
        {
            AddRange(other);
            _cachedFont = null;
        }

        public MultiFontReference(MultiFontReference other)
        {
            AddRange(other);
            _cachedFont = other._cachedFont;
        }

        public MultiFontReference(MultiFontReference other, float size)
        {
            AddRange(other.Select(x => new FontReference(x) { Size = size }));
            _cachedFont = null;
        }

        public MultiFontReference(MultiFont other)
        {
            AddRange(other.Fonts.Select(x => new FontReference(x)));
            _cachedFont = other;
        }

        public MultiFontReference(FontAsset[] assets, float size)
        {
            AddRange(assets.Select(x => new FontReference(x, size)));
            _cachedFont = null;
        }

        [EditorOrder(0), Tooltip("The font asset to use as characters source.")]
        public MultiFont GetMultiFont()
        {
            if (_cachedFont)
                return _cachedFont;
            var fontList = this.Where(x => x.Font).Select(x => x.GetFont()).ToArray();
            _cachedFont = MultiFont.Create(fontList);
            return _cachedFont;
        }

        public bool Verify()
        {
            foreach (var i in this)
            {
                if (!i.Font)
                {
                    return false;
                }
            }

            return true;
        }

        [NoSerialize]
        private MultiFont _cachedFont;
    }
}
