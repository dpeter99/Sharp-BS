using System;
using System.Collections.Generic;
using Sharp_BS.Model;

namespace Sharp_BS
{
    public class BuildStep
    {
        public delegate void StepCallback(string[] prams);
        
        //private readonly Func<string[]> _action;
        private Action<string[]> callback;

        public List<string> extensions;
        
        public BuildStep(List<string> extensions, Action<string[]> c)
        {
            callback = c;
            this.extensions = extensions;
        }

        public void Run(string[] param)
        {
            callback.Invoke(param);
        }
    }
}