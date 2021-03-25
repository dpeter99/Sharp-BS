using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using Microsoft.ClearScript;
using Microsoft.ClearScript.V8;
using Sharp_BS.JSInterop;
using Sharp_BS.Model;
using Util.Singleton;

namespace Sharp_BS
{
    public class BSHost : Singleton<BSHost>
    {
        public string ProjectPath;
        
        public Project Project { get; set; }
        
        public Registry<BuildStep> BuildSteps = new();
        public Registry<FileExtensionPlugin> Extensions = new();
        public Registry<string> BuildVariables = new();

        //private Jint.Engine jsEngine;
        private Microsoft.ClearScript.V8.V8ScriptEngine jsEngine;
        private HostFunctions _hostFunctions;

        public BSHost(string projectFile)
        {
            FileInfo file = new FileInfo(projectFile);
            if (!file.Exists)
                throw new Exception("[PROJECT] I miss you");

            var jsonString = File.ReadAllText(file.ToString());

            var proj = JsonSerializer.Deserialize<Project>(jsonString);
            Project = proj ?? throw new Exception("WTF?");
            
            ProjectPath = file.Directory?.FullName!;
            BuildVariables.Register("root",ProjectPath);


            V8ScriptEngineFlags flags = V8ScriptEngineFlags.None;
            //flags = V8ScriptEngineFlags.EnableDebugging | V8ScriptEngineFlags.AwaitDebuggerAndPauseOnStart;
            
            jsEngine = new V8ScriptEngine(flags);


            _hostFunctions = new HostFunctions();
            jsEngine.AddHostObject("host", _hostFunctions);

            jsEngine.AddHostObject("Log", new Action<object>(PluginLog));
            jsEngine.AddHostObject("BSHost", this);
            jsEngine.AddHostObject("Exec", new Action<object>(PluginLog));

        }

        public void RegisterBuildStep(string name,string extensions, Action<string[]> action)
        {
            var exts = extensions.Split(' ').ToList();
            
            BuildSteps.Register(name, new BuildStep(exts, action));
        }
        
        public void RegisterBuildStep(string name,string extensions, object action)
        {
            RegisterBuildStep(name,extensions,(Action<string[]>)_hostFunctions.proc(1,action));
        }
        
        
        public void RegisterFileExtension(string extension, Action<Project> action)
        {
            Extensions.Register(extension, new FileExtensionPlugin(extension,action));
        }
        
        public void RegisterFileExtension(string extension, object action)
        {
            //RegisterFileExtension(extension,(Action<Project>)_hostFunctions.proc(1,action));
            RegisterFileExtension(extension, JSInteropRunner.MakeAction(action,_hostFunctions));
        }

        
        public void Build()
        {
            //Expand the file tree
            {
                foreach (var set in Project.Sources.Sets.Values)
                {
                    set.ExpandFileList();

                    //var extensions = set.Files.Select(f => f.Extension).Distinct();

                    var files = string.Join(" ", set.Files.Select(f => f.ToString()));

                    BuildVariables.Register(set.Name, files);
                }

                //Run file extension functions
                
                var extensions = Project.Sources.Sets.Values.SelectMany(s => s.Files).Select(f => f.Extension).Distinct();

                foreach (var extension in extensions)
                {
                    var ext = Extensions.Get(extension);
                    if (ext is not null)
                    {
                        ext.action.Invoke(Project);
                    }
                        
                }
                
            }

            foreach (var step in Project.Build.steps)
            {
                var args = new List<string>();
                var extensions = new List<string>();
                foreach (var valueArg in step.args)
                {
                    args.Add(TemplateString(valueArg));

                    var tags = GetTemplateTags(valueArg);

                    var sets = Project.Sources.Sets.Values.Where(i => tags.Any(t=>t == i.Name));

                    foreach (var s in sets)
                    {
                        extensions.AddRange(s.Files.Select(f => f.Extension).Distinct());
                    }
                }
                extensions = extensions.Distinct().ToList();

                var stepToRun = BuildSteps.Items.FirstOrDefault(i => i.Value.extensions.All(extensions.Contains));
                
                stepToRun.Value.Run(args.ToArray());
            }
            
        }
        
        
        public static string TemplateString(string path)
        {
            var pathBuilder = new StringBuilder(path);

            Regex regex = new Regex(@"\$([-\w]+)");
            var m = regex.Match(path);
            foreach (Capture capture in m.Captures)
            {
                pathBuilder.Remove(capture.Index, capture.Length);

                var key = capture.Value.Replace("$", "");

                var replace = BSHost.Instance.BuildVariables.Get(key);
                if (replace is not null)
                {
                    pathBuilder.Insert(capture.Index, replace);
                }
            }

            var pathDone = pathBuilder.ToString();
            return pathDone;
        }
        
        public List<string> GetTemplateTags(string path)
        {
            var stuff = new List<string>(); 

            Regex regex = new Regex(@"\$([-\w]+)");
            var m = regex.Match(path);
            foreach (Capture capture in m.Captures)
            {
                var key = capture.Value.Replace("$", "");
                stuff.Add(key);
            }
            
            return stuff;
        }
        
        
        public void LoadModules()
        {
            //var builtin = Directory.EnumerateFiles($"{ProjectPath}/.bs/plugins/**/*.js");
            var localPlugins = new DirectoryInfo($"{ProjectPath}/.bs/plugins/").EnumerateFiles("plugin.js",SearchOption.AllDirectories);
            //var localPlugins = Directory.EnumerateFiles($"{ProjectPath}/.bs/plugins/","**/plugin.js", SearchOption.AllDirectories);

            foreach (var pluginFile in localPlugins)
            {
                var jsCode = File.ReadAllText(pluginFile.ToString());
                //jsEngine.BreakPoints.Add(new BreakPoint(30,0));
                jsEngine.Execute(jsCode);
                
            }
        }

        public void PluginLog(object s)
        {
            Console.WriteLine(s);
        }
    }

    public class FileExtensionPlugin
    {
        public string extension;
        public Action<Project> action;

        public FileExtensionPlugin(string extension, Action<Project> action)
        {
            this.extension = extension;
            this.action = action;
        }
    }
}