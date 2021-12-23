using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Microsoft.ClearScript;
using Microsoft.ClearScript.JavaScript;
using Microsoft.ClearScript.V8;
using SharpBS.Utils;

namespace SharpBS.Plugins
{
    public class JSEngine : Singleton<JSEngine>
    {
        private readonly V8ScriptEngine jsEngine;
        private HostFunctions _hostFunctions;

        public JSEngine()
        {
            V8ScriptEngineFlags flags = V8ScriptEngineFlags.None;
            //flags = V8ScriptEngineFlags.EnableDebugging | V8ScriptEngineFlags.AwaitDebuggerAndPauseOnStart;
            
            jsEngine = new V8ScriptEngine(flags);


            _hostFunctions = new HostFunctions();
            jsEngine.AddHostObject("host", _hostFunctions);

            jsEngine.AddHostObject("Log", new Action<object>(PluginLog));
            jsEngine.AddHostObject("BSHost", jsEngine);
            jsEngine.AddHostObject("Exec", new Action<object>(PluginLog));
            
            

        }

        public ScriptObject? LoadJs(string path, IPlugin plugin)
        {
            var f = FileUtils.GetFile(path);
            var jsCode = File.ReadAllText(f.FullName);

            
            
            var doc = new DocumentInfo(new Uri(path))
            {
                //SourceMapUri = new Uri(path),
                Category = ModuleCategory.Standard,
                ContextCallback = (i) => GetPluginContext(plugin)
            };
            
            var compiled = jsEngine.Compile(doc, jsCode);
            ScriptObject? res = jsEngine.Evaluate(compiled) as ScriptObject;
            
            //jsEngine.

            //Console.WriteLine(res);

            return res;
        }

        public static IDictionary<string,object> GetPluginContext(IPlugin plugin)
        {
            var props = new Dictionary<string, object>();
            props.Add("name", "Test");

            return props;
        }
        
        public void PluginLog(object s)
        {
            Console.WriteLine(s);
        }
    }
    
}