using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace Flax.Stats
{
    internal partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        public void SetSourceCode(List<CodeFrame> codeFrames, List<CodeFrame> apiCodeFrames)
        {
            var set = chart1.Series["TotalLines"];
            set.ChartType = SeriesChartType.Line;
            var firstApiFrame = apiCodeFrames.Count > 0 ? apiCodeFrames[0].Date : DateTime.MaxValue;

            for (int i = 0; i < codeFrames.Count; i++)
            {
                var frame = codeFrames[i];
                var root = frame.Root;
				long total = 0;
				
                // All frames before 2016-08-05 have Source folder named to Celelej in a project dir
                if (frame.Date < new DateTime(2016, 8, 5))
                {
                    var sourceDir = root["Celelej"];
                    total = sourceDir.TotalLinesOfCode;
                }
                else
                {
                    var sourceDir = root["Source"];

                    // Check if frame is from Source
                    if (sourceDir != null)
                    {
                        total = sourceDir.TotalLinesOfCode + root["Development"].TotalLinesOfCode +
                                root["Tools"].TotalLinesOfCode;
                    }

                    // Check if there was SourceAPI frame at that frame
                    /*if (frame.Date >= firstApiFrame)
                    {
                        var apiFrame = apiCodeFrames.Find(e => Math.Abs((e.Date - frame.Date).TotalMinutes) < 2);
                        if (apiFrame != null)
                            total += apiFrame.Root.TotalLinesOfCode;
                    }*/
                }

                set.Points.AddXY(codeFrames[i].Date, total);
            }
        }
    }
}
