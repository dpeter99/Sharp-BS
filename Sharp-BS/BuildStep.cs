using System;
using System.Collections.Generic;

namespace Sharp_BS
{
    public class BuildStep
    {
        private readonly Action<string[]> _action;

        public List<string> extensions;
        
        public BuildStep(Action<string[]> action, List<string> extensions)
        {
            _action = action;
            this.extensions = extensions;
        }

        public void Run(string[] param)
        {
            _action.Invoke(param);
        }
    }
}