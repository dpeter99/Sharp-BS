using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using Serilog;
using SharpBS.Model;
using SharpBS.Plugins;

namespace SharpBS
{
    public class BSProject
    {
        public string Name { get; private set; }

        public List<IPlugin> plugins = new();
        
        public BSProject(string path)
        {
            var conf = LoadConfig(path);
            this.Name = conf.Name;

            foreach (var plugin in conf.Plugins)
            {
               var p = PluginProvider.GetPlugin(plugin);

               if (p == null)
               { 
                   Log.Error("Could not find {Plugin} plugin", plugin);
                   continue;
               }
               
               plugins.Add(p);
            }
        }
        
        //Load config
        public ProjectConfig LoadConfig(string projectFile)
        {
            projectFile = Path.GetFullPath(projectFile);
            
            FileInfo file = new FileInfo(projectFile);
            if (!file.Exists)
                throw new Exception("[PROJECT] I miss you.\n There is no json file at: " + projectFile);

            var jsonString = File.ReadAllText(file.ToString());

            var proj = JsonSerializer.Deserialize<ProjectConfig>(jsonString);
            var config = proj ?? throw new Exception("WTF?");
            
            return config;
        }
    }
}