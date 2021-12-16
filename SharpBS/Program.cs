using System;

namespace SharpBS
{
    class Program
    {
        static void Main(string[] args)
        {
            BSHost runner = new BSHost();
            runner.OpenProject("../Examples/example.json");
        }
    }
}