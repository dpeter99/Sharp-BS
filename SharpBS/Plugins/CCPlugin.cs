using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using Serilog;
using SharpBS.Model;

namespace SharpBS.Plugins;

[Plugin("cc")]
public class CCPlugin: IPlugin
{
    private readonly BSProject _project;
    public string Name { get; } = "cc";

    public CCPlugin(BSProject project)
    {
        _project = project;
    }
    
    public Step? GetStep(string name, JsonElement config)
    {
        if (name == "cc::compile")
            return new CCCompileStep(config, _project);
        return null;
    }
    
}

public class CCCompileStep : Step
{
    
    List<FileInfo> srcFiles = new();
    
    public CCCompileStep(JsonElement config, BSProject project)
    {
        var src_array = config.GetProperty("src");
        foreach (var src in src_array.EnumerateArray())
        {
            var src_text = src.GetString();

            var s = project.vars.ParseString(src_text);
            Log.Logger.Debug(s);
        }
    }
    
    public void BuildGraph()
    {
        
    }
}


/// <summary>
/// Represents a single invocation of the c/cpp compiler
/// </summary>
/// This will output an object file
public class CCCompileTask : BuildTask
{
    private readonly string _file;

    public CCCompileTask(string file)
    {
        _file = file;
    }
    
    public override void Execute()
    {
        Log.Logger.Debug("Compiling files: " + _file);
    }
}

public interface INode
{
    
}