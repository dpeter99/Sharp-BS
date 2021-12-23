using System;
using System.IO;
using System.Text.Json;
using Microsoft.Extensions.Hosting;
using Serilog;
using Serilog.Core;
using SharpBS.Model;
using SharpBS.Plugins;
using SharpBS.Utils;

namespace SharpBS
{
    public class BSHost : Singleton<BSHost>
    {
        public BSProject Project { get; private set; }
        public string? _projectPath { get; private set; }
        
        public BSHost()
        {
            Log.Logger = new LoggerConfiguration()
                .WriteTo.Console()
                .CreateLogger();
            
            Log.Information("Starting BS!");

            new JSEngine();
        }

        public void OpenProject(string path)
        {
            _projectPath = path;
            Log.Information("Opening project at: {Path}", _projectPath);

            Project = BSProject.fromFile(_projectPath);

            Log.Information("Project: {Name}", Project.Name);
        }
        
        //Parse args


        //Construct Project


        //Find Plugins

        //Run steps
        
        
        
    }
}