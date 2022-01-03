using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Text.Json;
using System.Text.RegularExpressions;
using Microsoft.ClearScript.Util.Web;
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

        public List<Step> steps = new();

        public VariableStore vars = new();

        private BSProject(ProjectConfig conf, string path)
        {
            this.Name = conf.Name; 
            this.ProjectRoot = path;
            
            AddNewVar("root",ProjectRoot);
            
            foreach (var variable in conf.Variables)
            {
                AddNewVar(variable.Key, variable.Value);
            }
            
            
            if (conf.Plugins.Count > 0)
            {
                foreach (var plugin in conf.Plugins)
                {
                    var p = PluginProvider.GetPlugin(plugin.plugin, this);

                    if (p == null)
                    {
                        Log.Error("Could not find {Plugin} plugin", plugin.plugin);
                        continue;
                    }

                    plugins.Add(p);
                }
            }

            if (conf.Steps.Count > 0)
            {
                foreach (JsonElement stepData in conf.Steps)
                {
                    string stepName = stepData.GetProperty("task").GetString() ?? "";

                    var step = GetStepByName(stepName, stepData);

                    if (step is null)
                    {
                        Log.Error("Could not find {Step} plugin", stepName);
                        continue;
                    }
                    
                    steps.Add(step);
                }
            }
            
            
        }

        private void AddNewVar(string key,string value)
        {
            vars.Add(key, value);
            Log.Logger.Debug("Found var: Key:\"{Key}\" Value:\"{Value}\"", key, value);
        }

        /// <summary>
        /// Loads a project from a file
        /// </summary>
        /// <param name="path">The path to the project file</param>
        /// <returns>The loaded project</returns>
        public static BSProject FromFile(string path)
        {
            var file = FileUtils.GetFile(path);

            var jsonString = File.ReadAllText(file.ToString());

            
            var projectRoot = file.Directory?.FullName ?? "";

            var fromJsonString = FromJsonString(jsonString, projectRoot);
            return fromJsonString;
        }
        
        /// <summary>
        /// Loads a project from the string representation of the config
        /// </summary>
        /// <param name="Json">The string of the JSON config</param>
        /// <param name="path">The path were this project is located</param>
        /// <returns>The loaded project</returns>
        public static BSProject FromJsonString(string Json, string path)
        {
            var conf = LoadConfig(Json);
            return new BSProject(conf, path);
        }
        
        /// <summary>
        /// Loaded the ProjectConfig from the JSON
        /// </summary>
        /// <param name="jsonString">The json contnet to parse</param>
        /// <returns>The parsed contents of the json as a Project Config</returns>
        /// <exception cref="Exception">Exception if the parsing has failed</exception>
        public static ProjectConfig LoadConfig(string jsonString)
        {
            var proj = JsonSerializer.Deserialize<ProjectConfig>(jsonString);
            var config = proj ?? throw new Exception("WTF?");
            
            return config;
        }


        public IPlugin? GetPluginByName(string name)
        {
            foreach (var plugin in plugins)
            {
                if (plugin.Name == name)
                    return plugin;
            }

            return null;
        }
        
        public Step? GetStepByName(string name, JsonElement config)
        {
            if (name[0] == '@')
            {
                name = name.Remove(0, 2);
                name = name.Replace("}", "");
                var tokens = name.Split("::");

                var plugin = GetPluginByName(tokens[0]);
                if(plugin is not null)
                    return plugin.GetStep(name, config);
            }

            return null;
        }

        
        
        public void BuildGraph()
        {   
            
        }
    }

    public class VariableStore
    {
        public class Key
        {
            private string[] tokens;

            public Key(string name)
            {
                name = name.Replace("${", "").Replace("}", "");
                tokens = name.Split("::");
            }

            public override bool Equals(object? obj)
            {
                if (obj is Key o)
                {
                    return this.ToString() == o.ToString();
                }

                return base.Equals(obj);
            }

            public override int GetHashCode()
            {
                return this.ToString().GetHashCode();
            }

            public override string ToString()
            {
                return tokens.Aggregate((s, s1) => s + s1);
            }
        }

        private VariableStore? parent = null;
        
        Dictionary<Key,string> vars = new ();

        public VariableStore()
        {
            
        }

        public VariableStore(VariableStore parent)
        {
            this.parent = parent;
        }
        
        public void Add(string key, string val)
        {
            vars.Add(new Key(key),val);
        }

        public bool HasKey(string key)
        {
            return vars.ContainsKey(new Key(key));
        }
        
        public bool HasKey(Key key)
        {
            return vars.ContainsKey(key);
        }
        
        public string Get(string key)
        {
            var k = new Key(key);
            return Get(k);
        }

        public string Get(Key key)
        {
            if (vars.ContainsKey(key))
            {
                return vars[key];
            }
            else if (parent is not null && parent.HasKey(key))
                return parent.Get(key);

            throw new Exception("Could not find key: " + key);
        }

        public string ParseString(string text)
        {
            var reg = new Regex(@"(?:\${([a-z_\-:]+)})");

            while (reg.IsMatch(text))
            {
                text = reg.Replace(text, match => Get(match.Value));
            }

            return text;
        }
        
        public VariableStore GetSubStore()
        {
            return new VariableStore(this);
        }
    }
}