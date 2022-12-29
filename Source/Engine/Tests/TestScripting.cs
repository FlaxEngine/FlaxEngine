// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#if FLAX_TESTS
namespace FlaxEngine
{
    /// <summary>
    /// Test class.
    /// </summary>
    public class TestClassManaged : TestClassNative
    {
        TestClassManaged()
        {
            SimpleField = 2;
        }

        /// <inheritdoc />
        public override int Test(string str)
        {
            return str.Length + base.Test(str);
        }
    }
}
#endif
