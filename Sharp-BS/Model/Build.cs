using System.Collections;
using System.Collections.Generic;
using System.Dynamic;
using System.Linq;
using Microsoft.ClearScript;
using Sharp_BS.JSInterop;

namespace Sharp_BS.Model
{
    public class Build : ICustomConverter
    {
        public List<Step> steps = new();

        public PropertyBag MarshalTo()
        {
            var expando = new PropertyBag();
            
            var stepsExpando = new PropertyBag();
            foreach (var step in steps)
            {
                stepsExpando[step.name] = step.args.ToArray();
            }

            expando["steps"] = stepsExpando;


            return expando;
        }

        public void MarshalBack(PropertyBag parameters)
        {
            var nSteps = parameters["steps"] as PropertyBag;

            var stepList = new List<Step>();
            
            foreach (var step in nSteps)
            {
                var s = new Step(step.Key);
                s.args = ((IList) step.Value).Cast<string>().ToList();
                
                stepList.Add(s);
            }

            steps = stepList;
        }
    }
}