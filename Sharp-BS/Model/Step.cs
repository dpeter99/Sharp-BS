using System.Collections.Generic;
using Microsoft.ClearScript;
using Sharp_BS.JSInterop;

namespace Sharp_BS.Model
{
    public class Step
    {
        public string name { get; set; }

        public List<string> args = new List<string>();
        
        public Step(string name)
        {
            this.name = name;
        }

/*
        public PropertyBag MarshalTo()
        {
            var p = new PropertyBag();
            
        }

        public void MarshalBack(PropertyBag parameters)
        {
            throw new System.NotImplementedException();
        }
        */
    }
}