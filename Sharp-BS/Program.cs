using System;
using System.IO;
using System.Text.Json;
using Sharp_BS.Model;

namespace Sharp_BS
{
    class Program
    {
        static void Main(string[] args)
        {
            //Console.WriteLine("Hello World!");

            BSHost host = new BSHost(@"P:/csharp/Sharp-BS/Sharp-BS/Sample/project.json");
            host.LoadModules();
            host.Build();
            
            
        }
    }
}