// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using System.IO;
using System.Linq;
using FlaxEditor.Surface;
using FlaxEngine;
using NUnit.Framework;

namespace FlaxEditor.Tests
{
    [TestFixture]
    public class TestSurface
    {
        [Test]
        public void TestArchetypes()
        {
            // Validate all archetypes (reduce mistakes)
            for (int groupIndex = 0; groupIndex < NodeFactory.DefaultGroups.Count; groupIndex++)
            {
                var group = NodeFactory.DefaultGroups[groupIndex];

                // Unique group id
                for (int i = groupIndex + 1; i < NodeFactory.DefaultGroups.Count; i++)
                {
                    Assert.IsFalse(group.GroupID == NodeFactory.DefaultGroups[i].GroupID, string.Format("Invalid group ID: {0} and {1} have the same ID", group.GroupID, NodeFactory.DefaultGroups[i].GroupID));
                }

                var nodes = group.Archetypes.ToArray();
                for (int nodeIndex = 0; nodeIndex < nodes.Length; nodeIndex++)
                {
                    var node = nodes[nodeIndex];

                    // Unique node ids
                    for (int i = nodeIndex + 1; i < nodes.Length; i++)
                    {
                        Assert.IsFalse(node.TypeID == nodes[i].TypeID, string.Format("Invalid node TypeID: {0} and {1} have the same ID", node.Title, nodes[i].Title));
                    }

                    // Unique box ids
                    if (node.Elements != null)
                    {
                        for (int i = 0; i < node.Elements.Length; i++)
                        {
                            if (node.Elements[i].Type == NodeElementType.Input || node.Elements[i].Type == NodeElementType.Output)
                            {
                                for (int j = i + 1; j < node.Elements.Length; j++)
                                {
                                    if (node.Elements[j].Type == NodeElementType.Input || node.Elements[j].Type == NodeElementType.Output)
                                    {
                                        Assert.IsFalse(node.Elements[i].BoxID == node.Elements[j].BoxID, string.Format("Invalid BoxID for node {0} in group {3}, elements: {1} and {2} have the same ID", node.Title, i, j, group.Name));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        [Test]
        public void TestGPUTextureType()
        {
            var meta = new SurfaceMeta();
            Assert.AreEqual(MaterialParameterType.GPUTexture, meta.GetGPUTextureType());

            // Stored as a single byte with the material parameter type (read by ShaderGraphUtilities::GetGPUTextureType)
            meta.SetGPUTextureType(MaterialParameterType.GPUTextureArray);
            Assert.AreEqual(MaterialParameterType.GPUTextureArray, meta.GetGPUTextureType());
            Assert.AreEqual(new[] { (byte)MaterialParameterType.GPUTextureArray }, meta.GetEntry(SurfaceMeta.GPUTextureTypeMetaTypeID).Data);

            // Survives save and load
            var stream = new MemoryStream();
            meta.Save(new BinaryWriter(stream));
            stream.Position = 0;
            var loaded = new SurfaceMeta();
            loaded.Load(new BinaryReader(stream));
            Assert.AreEqual(MaterialParameterType.GPUTextureArray, loaded.GetGPUTextureType());

            meta.SetGPUTextureType(MaterialParameterType.GPUTextureVolume);
            Assert.AreEqual(MaterialParameterType.GPUTextureVolume, meta.GetGPUTextureType());
            meta.SetGPUTextureType(MaterialParameterType.GPUTextureCube);
            Assert.AreEqual(MaterialParameterType.GPUTextureCube, meta.GetGPUTextureType());

            // A 2D texture is the default so it removes the entry
            meta.SetGPUTextureType(MaterialParameterType.GPUTexture);
            Assert.AreEqual(MaterialParameterType.GPUTexture, meta.GetGPUTextureType());
            Assert.IsFalse(meta.Entries.Any(x => x.TypeID == SurfaceMeta.GPUTextureTypeMetaTypeID));

            // Other types aren't GPU texture types
            meta.SetGPUTextureType(MaterialParameterType.Texture);
            Assert.IsFalse(meta.Entries.Any(x => x.TypeID == SurfaceMeta.GPUTextureTypeMetaTypeID));
            meta.AddEntry(SurfaceMeta.GPUTextureTypeMetaTypeID, new[] { (byte)MaterialParameterType.Texture });
            Assert.AreEqual(MaterialParameterType.GPUTexture, meta.GetGPUTextureType());
        }
    }
}
#endif
