using System;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.CommandLine.NamingConventionBinder;
using System.Threading.Tasks;
using SharpBS.Utils;


namespace SharpBS
{
    class Program
    {
        static BSHost runner = new();
        
        static void Main(string[] args)
        {
            var root = new RootCommand()
            {
                new Command("build")
                {
                    new Option<bool>(
                        "--graph",
                        "An option whose argument is parsed as a bool"),
                    new Option<string>("path","path to the project file"),
                    new Option<string>("conf")
                }.apply((a)=>
                {
                    a.Handler = CommandHandler.Create(BuildAction);
                })
            };

            root.Invoke(args);
        }

        private static void BuildAction(bool graph, string? path, string conf)
        {
            if (path is null)
                path = "./project.json";
            
            runner.OpenProject(path);
            runner.Build(graph: true);

        }
    }
}