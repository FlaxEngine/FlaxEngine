// Copyright (c) Wojciech Figat. All rights reserved.

#if FLAX_TESTS
using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;
using NUnit.Framework;

namespace FlaxEditor.Tests
{
    [TestFixture]
    public class TestSceneGraph
    {
        public class MyNode : SceneGraphNode
        {
            private string _name;

            public MyNode(string name)
            : base(Guid.NewGuid())
            {
                _name = name;
            }

            public MyNode[] LinkChildren
            {
                set
                {
                    foreach (var e in value)
                    {
                        e.ParentNode = this;
                    }
                }
            }

            public override string Name => _name;
            public override SceneNode ParentScene => null;
            public override Transform Transform { get; set; }
            public override bool IsActive => true;
            public override bool IsActiveInHierarchy => true;
            public override int OrderInParent { get; set; }
        }

        private MyNode GetTestTree()
        {
            var root = new MyNode("Root")
            {
                LinkChildren = new[]
                {
                    new MyNode("Level1_0")
                    {
                        LinkChildren = new[]
                        {
                            new MyNode("Level2_0"),
                            new MyNode("Level2_1")
                            {
                                LinkChildren = new[]
                                {
                                    new MyNode("Level3_0"),
                                    new MyNode("Level3_1"),
                                    new MyNode("Level3_2"),
                                }
                            },
                            new MyNode("Level2_2"),
                        }
                    },
                    new MyNode("Level1_1"),
                    new MyNode("Level1_2"),
                    new MyNode("Level1_3"),
                    new MyNode("Level1_4"),
                }
            };

            return root;
        }

        private MyNode GetNode(SceneGraphNode root, string name)
        {
            if (root.Name == name)
                return root as MyNode;
            for (int i = 0; i < root.ChildNodes.Count; i++)
            {
                var node = GetNode(root.ChildNodes[i], name);
                if (node != null)
                    return node;
            }
            return null;
        }

        [Test]
        public void TestBuildAllNodes()
        {
            var root = GetTestTree();

            var testList1 = new List<SceneGraphNode>
            {
                GetNode(root, "Level1_0"),
                GetNode(root, "Level3_0"),
                GetNode(root, "Level3_1"),
                GetNode(root, "Level1_3"),
            };
            var solidList = SceneGraphTools.BuildAllNodes(testList1);
            Assert.IsTrue(solidList != null && solidList.Count == 8);
            Assert.IsTrue(solidList.Contains(GetNode(root, "Level1_0")));
            Assert.IsTrue(solidList.Contains(GetNode(root, "Level1_3")));
            Assert.IsTrue(solidList.Contains(GetNode(root, "Level2_0")));
            Assert.IsTrue(solidList.Contains(GetNode(root, "Level2_1")));
            Assert.IsTrue(solidList.Contains(GetNode(root, "Level2_2")));
            Assert.IsTrue(solidList.Contains(GetNode(root, "Level3_0")));
            Assert.IsTrue(solidList.Contains(GetNode(root, "Level3_1")));
            Assert.IsTrue(solidList.Contains(GetNode(root, "Level3_2")));
        }

        [Test]
        public void TestBuildNodesParents()
        {
            var root = GetTestTree();

            var testList1 = new List<SceneGraphNode>
            {
                GetNode(root, "Level1_0"),
                GetNode(root, "Level3_0"),
                GetNode(root, "Level3_1"),
                GetNode(root, "Level1_3"),
            };
            var solidList = SceneGraphTools.BuildNodesParents(testList1);
            Assert.IsTrue(solidList != null && solidList.Count == 2);
            Assert.IsTrue(solidList.Contains(GetNode(root, "Level1_0")));
            Assert.IsTrue(solidList.Contains(GetNode(root, "Level1_3")));
        }
    }
}
#endif
