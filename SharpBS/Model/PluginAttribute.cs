using System;

namespace SharpBS.Model
{
    [AttributeUsage(AttributeTargets.Class)]
    public class PluginAttribute : Attribute
    {
        public string Name { get; }

        public PluginAttribute(string name)
        {
            Name = name;
        }
    }
}