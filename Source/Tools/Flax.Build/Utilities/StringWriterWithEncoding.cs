// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using System.Text;

namespace Flax.Build
{
    /// <summary>
    /// The <see cref="StreamWriter"/> implementation with a custom encoding.
    /// </summary>
    /// <seealso cref="System.IO.StringWriter" />
    public sealed class StringWriterWithEncoding : StringWriter
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="StringWriterWithEncoding"/> class.
        /// </summary>
        public StringWriterWithEncoding()
        : this(Encoding.UTF8)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="StringWriterWithEncoding"/> class.
        /// </summary>
        /// <param name="encoding">The encoding.</param>
        public StringWriterWithEncoding(Encoding encoding)
        {
            Encoding = encoding;
        }

        /// <summary>
        /// Gets the <see cref="T:System.Text.Encoding" /> in which the output is written.
        /// </summary>
        public override Encoding Encoding { get; }
    }
}
