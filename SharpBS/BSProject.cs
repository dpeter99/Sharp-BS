using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using Serilog;
using SharpBS.Model;
using SharpBS.Plugins;
using SharpBS.Utils;

namespace SharpBS
{
    public class BSProject
    {
        public string Name { get; private set; }

        public string ProjectRoot { get; private set; }
        
        public List<IPlugin> plugins = new();
        

        private BSProject(ProjectConfig conf, string path)
        {
            this.Name = conf.Name;

            this.ProjectRoot = path;
            
            if (conf.Plugins.Count > 0)
            {
                foreach (var plugin in conf.Plugins)
                {
                    var p = PluginProvider.GetPlugin(plugin, this);

                    if (p == null)
                    {
                        Log.Error("Could not find {Plugin} plugin", plugin);
                        continue;
                    }

                    plugins.Add(p);
                }
            }
        }

        public static BSProject fromFile(string path)
        {
            var file = FileUtils.GetFile(path);

            var jsonString = File.ReadAllText(file.ToString());

            
            var projectRoot = file.Directory?.FullName ?? "";

            var fromJsonString = FromJsonString(jsonString, projectRoot);
            return fromJsonString;
        }
        
        public static BSProject FromJsonString(string Json, string path)
        {
            var conf = LoadConfig(Json);
            return new BSProject(conf, path);
        }
        
        //Load config
        public static ProjectConfig LoadConfig(string jsonString)
        {
            var proj = JsonSerializer.Deserialize<ProjectConfig>(jsonString);
            var config = proj ?? throw new Exception("WTF?");
            
            return config;
        }
    }
}