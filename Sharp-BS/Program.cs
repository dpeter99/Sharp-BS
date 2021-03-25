using System;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Text.Json;
using Sharp_BS.Model;

namespace Sharp_BS
{
    class Program
    {
        [SuppressMessage("ReSharper.DPA", "DPA0003: Excessive memory allocations in LOH", MessageId = "type: System.Byte[]")]
        static void Main(string[] args)
        {
            //Console.WriteLine("Hello World!");

            BSHost host = new BSHost(@"P:/csharp/Sharp-BS/Sharp-BS/Sample/project.json");
            host.LoadModules();
            host.Build();
            
            
        }
    }
}