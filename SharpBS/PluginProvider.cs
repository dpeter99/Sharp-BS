using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using SharpBS.Model;
using SharpBS.Plugins;

[assembly: InternalsVisibleTo("SharpBS.Tests")]
namespace SharpBS
{
    public static class PluginProvider
    {
        internal static List<(Type Type, PluginAttribute Attributes)> _typesWithMyAttribute;
        //private static List<(Type, Attributes)> _typesWithMyAttribute;

        static PluginProvider()
        {
            var querry = 
                from a in AppDomain.CurrentDomain.GetAssemblies().AsParallel()
                from t in a.GetTypes()
                let attributes = t.GetCustomAttributes(typeof(PluginAttribute), true)
                where attributes != null && attributes.Length > 0 && t.IsAssignableTo(typeof(IPlugin))
                select new { Type = t, Attributes = attributes.Cast<PluginAttribute>().First() };
            
            _typesWithMyAttribute = querry.Select(a=> (a.Type,a.Attributes)).ToList();
        }
        
        public static IPlugin? GetPlugin(string name, BSProject project)
        {
            //Check loaded assemblies
            (Type? Type, PluginAttribute? Attributes) plugin = _typesWithMyAttribute.Find(plugin => plugin.Attributes.Name == name);
            if (plugin.Type != null)
            {
                var constr = plugin.Type.GetConstructor(BindingFlags.Public, Type.EmptyTypes);

                var p = Activator.CreateInstance(plugin.Type) as IPlugin;
                return p;
            }
            
            
            //Check for js files in .bs/plugins/
            
            var localPlugins = new DirectoryInfo($"{project.ProjectRoot}/.bs/plugins/")
                .EnumerateFiles(name+".js",SearchOption.AllDirectories)
                .FirstOrDefault();

            if (localPlugins != null)
                return new JSPlugin(localPlugins.FullName);
            
            
            return null;
        }
    }
}