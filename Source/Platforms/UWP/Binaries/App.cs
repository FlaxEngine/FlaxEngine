using System;
using Windows.ApplicationModel.Core;
using Windows.UI.Core;
using FlaxEngine;

namespace {0}
{{
    public class App : IFrameworkView, IFrameworkViewSource
    {{
        private Game _game;

        [MTAThread]
        static void Main(string[] args)
        {{
            var app = new App();
            CoreApplication.Run(app);
        }}
        
        public App()
        {{
            InitUWP.SetupDisplay();
			
            _game = new Game();
        }}

        public virtual void Initialize(CoreApplicationView applicationView)
        {{
            _game.Initialize(applicationView);
        }}
        
        public void SetWindow(CoreWindow window)
        {{
            _game.SetWindow(window);
        }}

        public void Load(string entryPoint)
        {{
        }}

        public void Run()
        {{
            _game.Run();
        }}

        public void Uninitialize()
        {{
        }}

        public IFrameworkView CreateView()
        {{
            return this;
        }}
    }}
}}
