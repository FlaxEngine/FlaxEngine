// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;

namespace Flax.Stats
{
    internal static class Program
    {
        private const string ProjectRoot = @"C:\Flax\"; // TODO: could we detect it?

        private struct StatsTarget
        {
            public string SourceFolder;
            public string StatsFolder;

            public StatsTarget(string source, string stats)
            {
                SourceFolder = ProjectRoot + source;
                StatsFolder = ProjectRoot + @"Source\" + stats;

                if (!Directory.Exists(SourceFolder))
                    Directory.CreateDirectory(SourceFolder);
                if (!Directory.Exists(StatsFolder))
                    Directory.CreateDirectory(StatsFolder);
            }
        }

        private static StatsTarget Source = new StatsTarget("Source", @"Development\CodeStats");
        private static StatsTarget SourceAPI = new StatsTarget("Source\\FlaxAPI", @"Development\CodeStatsAPI");

        private static string formatNum(long num)
        {
            return num.ToString("# ### ### ##0");
        }

        private static string getLangStatsText(out long totalLines, params CodeFrame[] frames)
        {
            long linesCpp = 0, linesCSharp = 0, linesHlsl = 0;

            for (int i = 0; i < frames.Length; i++)
            {
                var root = frames[i].Root;

                linesCpp += root.GetTotalLinesOfCode(Languages.Cpp);
                linesCSharp += root.GetTotalLinesOfCode(Languages.CSharp);
                linesHlsl += root.GetTotalLinesOfCode(Languages.Hlsl);
            }

            totalLines = linesCpp + linesCSharp + linesHlsl;

            return string.Format(
                @"C++:  {0}
C#:   {1}
HLSL: {2}",
                formatNum(linesCpp),
                formatNum(linesCSharp),
                formatNum(linesHlsl)
            );
        }

        private static void saveStatsInfo(CodeFrame source, CodeFrame sourceApi)
        {
            long totalLines;

            string sourceStats = getLangStatsText(out totalLines, source);
            string sourceApiStats = getLangStatsText(out totalLines, sourceApi);
            string totalStats = getLangStatsText(out totalLines, source, sourceApi);

            string path = Path.Combine(ProjectRoot, "Source\\Development", "Code Stats Summary.txt");
            File.WriteAllText(path,
                string.Format(
                    @"Source:
{0}

SourceAPI:
{1}

Total:
{2}

Total lines of code: {3}
",
                    sourceStats,
                    sourceApiStats,
                    totalStats,
                    formatNum(totalLines)
                ));
        }

        public static int CompareFrames(CodeFrame x, CodeFrame y)
        {
            return x.Date.CompareTo(y.Date);
        }

        private static void Peek()
        {
            // Peek code frames
            Environment.CurrentDirectory = Source.SourceFolder;
            CodeFrame sourceFrame = CodeFrame.Capture(Source.SourceFolder);
            CodeFrame sourceApiFrame = CodeFrame.Capture(SourceAPI.SourceFolder);

            // Save frames
            var filename = "Stats_" + DateTime.Now.ToFilenameString() + ".dat";
            sourceFrame.Save(Path.Combine(Source.StatsFolder, filename));
            sourceApiFrame.Save(Path.Combine(SourceAPI.StatsFolder, filename));

            // Update stats
            saveStatsInfo(sourceFrame, sourceApiFrame);
        }

        private static void Clear()
        {
        }

        private static void Show()
        {
            // Load all code frames
            const string filesFilter = "Stats_*.dat";
            string[] files = Directory.GetFiles(Source.StatsFolder, filesFilter);
            string[] filesApi = Directory.GetFiles(SourceAPI.StatsFolder, filesFilter);
            //
            List<CodeFrame> sourceCodeFrames = new List<CodeFrame>(files.Length);
            List<CodeFrame> sourceApiCodeFrames = new List<CodeFrame>(filesApi.Length);
            //
            for (int i = 0; i < files.Length; i++)
                sourceCodeFrames.Add(CodeFrame.Load(files[i]));
            for (int i = 0; i < filesApi.Length; i++)
                sourceApiCodeFrames.Add(CodeFrame.Load(filesApi[i]));

            // Sort frames
            sourceCodeFrames.Sort(CompareFrames);
            sourceApiCodeFrames.Sort(CompareFrames);

            // Show window
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Form1 f = new Form1();
            f.SetSourceCode(sourceCodeFrames, sourceApiCodeFrames);
            Application.Run(f);

            // Clean data
            for (int i = 0; i < sourceCodeFrames.Count; i++)
                sourceCodeFrames[i].Dispose();
            for (int i = 0; i < sourceApiCodeFrames.Count; i++)
                sourceApiCodeFrames[i].Dispose();
        }

        /// <summary>
        /// The main entry point for the application
        /// </summary>
        [STAThread]
        private static void Main(string[] args)
        {
            // Parse input arguments
            TaskType task = TaskType.Show;
            foreach (var s in args)
            {
                switch (s)
                {
                    case "peek":
                        task = TaskType.Peek;
                        break;
                    case "clear":
                        task = TaskType.Clear;
                        break;
                    default:
                        MessageBox.Show("Unknown argument: " + s);
                        break;
                }
            }

            // Switch task
            switch (task)
            {
                case TaskType.Peek:
                    Peek();
                    break;
                case TaskType.Clear:
                    Clear();
                    break;
                default:
                    Show();
                    break;
            }
        }
    }
}
