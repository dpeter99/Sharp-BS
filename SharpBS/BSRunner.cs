using System;
using System.IO;
using System.Text.Json;
using Microsoft.Extensions.Hosting;
using Serilog;
using Serilog.Core;
using SharpBS.Model;

namespace SharpBS
{
    public class BSHost
    {
        private string? _projectPath;
        
        public BSHost()
        {
            Log.Logger = new LoggerConfiguration()
                .WriteTo.Console()
                .CreateLogger();
            
            Log.Information("Starting BS!");
        }

        public void OpenProject(string path)
        {
            _projectPath = path;
            Log.Information("Opening project at: {Path}", _projectPath);

            BSProject project = new BSProject(_projectPath);
            
            Log.Information("Project: {Name}", project.Name);
        }
        
        //Parse args


        //Construct Project


        //Find Plugins

        //Run steps
    }
}