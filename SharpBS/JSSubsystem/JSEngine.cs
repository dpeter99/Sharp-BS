using System;
using Microsoft.ClearScript;
using Microsoft.ClearScript.V8;

namespace SharpBS.Plugins
{
    public class JSEngine
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
            jsEngine.AddHostObject("BSHost", this);
            jsEngine.AddHostObject("Exec", new Action<object>(PluginLog));

        }
        
        public void PluginLog(object s)
        {
            Console.WriteLine(s);
        }
    }
}