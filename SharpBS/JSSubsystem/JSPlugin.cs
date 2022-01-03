using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using Microsoft.ClearScript;
using SharpBS.Utils;

namespace SharpBS.Plugins
{
    public class JSPlugin : IPlugin
    {
        public string Name { get; } = "Js plugin";

        public List<string> steps = new();
        
        private object callback;

        public JSPlugin(string file)
        {
            var jsObj = JSEngine.Instance.LoadJs(file,this);
            
            ParseJsPluginObject(jsObj);
        }

        private void ParseJsPluginObject(ScriptObject? res)
        {
            if (res is not null)
            {
                if (res.PropertyNames.Contains("steps"))
                {
                    var steps = res["steps"] as ScriptObject;

                    foreach (var stepID in steps.PropertyIndices)
                    {
                        var step = steps[stepID];
                        
                        
                        
                        //Console.WriteLine(step["name"]);
                    }
                }
            }
        }
        
        public void Run(object config)
        {
            
        }

        public Step GetStep(string name, JsonElement config)
        {
            throw new NotImplementedException();
        }
    }
}