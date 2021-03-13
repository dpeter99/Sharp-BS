using System.Collections.Generic;

namespace Sharp_BS.Model
{
    public class Step
    {
        public string PropertyName { get; }

        public List<string> Args = new List<string>();
        
        public Step(string propertyName)
        {
            PropertyName = propertyName;
        }
    }
}