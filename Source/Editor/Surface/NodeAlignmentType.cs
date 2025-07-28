// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Surface
{
	/// <summary>
	/// Node Alignment type.
	/// </summary>
	[HideInEditor]
	public enum NodeAlignmentType
	{
		/// <summary>
		/// Align nodes vertically to top, matching top-most node.
		/// </summary>
		Top,

		/// <summary>
		/// Align nodes vertically to middle, using average of all nodes.
		/// </summary>
		Middle,

		/// <summary>
		/// Align nodes vertically to bottom, matching bottom-most node.
		/// </summary>
		Bottom,

		/// <summary>
		/// Align nodes horizontally to left, matching left-most node.
		/// </summary>
		Left,

		/// <summary>
		/// Align nodes horizontally to center, using average of all nodes.
		/// </summary>
		Center,

		/// <summary>
		/// Align nodes horizontally to right, matching right-most node.
		/// </summary>
		Right,
	}
}
