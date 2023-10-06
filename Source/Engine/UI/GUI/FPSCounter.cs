// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Frame Per Second Counter
    /// </summary>
    public class FPSCounter : Control
    {
        private byte _snapshotCount = 10;
        private int[] _snapshots;
        private int isAtSnapshots;
        private int minFPS = int.MaxValue;
        private int maxFPS;

        private int fps;
        private int accumFrames;
        private float lastUpdate;

        /// <summary>
        /// shows the avrage fps
        /// </summary>
        [EditorOrder(10), Tooltip("Shows the avrage fps.")]
        public bool ShowAvrage;
        /// <summary>
        /// how meny frames to snapshot and calculate the AvrageFPS more frames better accuracy
        /// </summary>
        [EditorOrder(20), Tooltip("How meny frames to snapshot and calculate the AvrageFPS more frames better accuracy.")]
        public byte SnapshotCount 
        {
            get => _snapshotCount;
            set 
            {
                _snapshotCount = value;
                _snapshots = new int[_snapshotCount];
            }
        }
        /// <summary>
        /// shows min and max fps
        /// </summary>
        [EditorOrder(30), Tooltip("Shows min and max fps.")]
        public bool ShowMinMax;

        /// <summary>
        /// crates new Frame Per Second Counter
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        public FPSCounter(float x, float y) : base(x, y, 64f, 32f)
        {
        }
        /// <summary>
        /// crates new Frame Per Second Counter
        /// </summary>
        public FPSCounter() : base(0, 0, 64f, 32f)
        {
        }
        /// <inheritdoc/>
        public override void Draw()
        {
            base.Draw();

            accumFrames++;
            float timeNow = Time.TimeSinceStartup;
            if (timeNow - lastUpdate >= 1.0f)
            {
                fps = accumFrames;
                lastUpdate = timeNow;
                accumFrames = 0;
            }

            Color color = GetFpsColor(fps);
            
            string text = string.Format("FPS: {0}", fps);
            Font font = Style.Current.FontMedium;
            Render2D.DrawText(font, text, new Rectangle(Float2.One, Size), Color.Black, TextAlignment.Near, TextAlignment.Near, TextWrapping.NoWrap, 1f, 1f);
            Render2D.DrawText(font, text, new Rectangle(Float2.Zero, Size), color, TextAlignment.Near, TextAlignment.Near, TextWrapping.NoWrap, 1f, 1f);
            var CsizeY = new Float2(0, font.Size*1.5f);
            var sizeY = new Float2(0, CsizeY.Y);
            if (ShowAvrage)
            {
                if (Engine.FrameCount % 60 == 0)
                {
                    if (isAtSnapshots >= _snapshotCount)
                    {
                        isAtSnapshots = 0;
                        minFPS = int.MaxValue;
                        maxFPS = 0;
                    }
                    else
                    {
                        _snapshots[isAtSnapshots] = fps;
                        isAtSnapshots++;
                    }
                }
                int avrage = 0;
                for (int i = 0; i < _snapshots.Length; i++)
                {
                    avrage += _snapshots[i];
                }
                avrage /= _snapshots.Length;

                color = GetFpsColor(avrage);
                
                text = string.Format("avg: {0}", avrage);
                Render2D.DrawText(font, text, new Rectangle(Float2.One + sizeY, Size), Color.Black, TextAlignment.Near, TextAlignment.Near, TextWrapping.NoWrap, 1f, 1f);
                Render2D.DrawText(font, text, new Rectangle(Float2.Zero + sizeY, Size), color, TextAlignment.Near, TextAlignment.Near, TextWrapping.NoWrap, 1f, 1f);
            }
            if (ShowMinMax)
            {
                if(fps < minFPS)
                {
                    minFPS = fps;
                }
                if (fps > maxFPS)
                {
                    maxFPS = fps;
                }
                sizeY += CsizeY;
                color = GetFpsColor(minFPS);
                text = string.Format("min: {0}", minFPS);
                Render2D.DrawText(font, text, new Rectangle(Float2.One  + sizeY, Size), Color.Black, TextAlignment.Near, TextAlignment.Near, TextWrapping.NoWrap, 1f, 1f);
                Render2D.DrawText(font, text, new Rectangle(Float2.Zero + sizeY, Size), color, TextAlignment.Near, TextAlignment.Near, TextWrapping.NoWrap, 1f, 1f);
                sizeY += CsizeY;
                color = GetFpsColor(maxFPS);
                text = string.Format("max: {0}", maxFPS);
                Render2D.DrawText(font, text, new Rectangle(Float2.One  + sizeY, Size), Color.Black, TextAlignment.Near, TextAlignment.Near, TextWrapping.NoWrap, 1f, 1f);
                Render2D.DrawText(font, text, new Rectangle(Float2.Zero + sizeY, Size), color, TextAlignment.Near, TextAlignment.Near, TextWrapping.NoWrap, 1f, 1f);
            }
        }
        private static Color GetFpsColor(in int fps)
        {
            if (fps < 13)
                return Color.Red;
            else if (fps < 22)
                return Color.Yellow;
            else
                return Color.Green;
        }
    }
}
