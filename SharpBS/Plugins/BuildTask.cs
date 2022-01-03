namespace SharpBS.Plugins;


/// <summary>
/// Represents a single action that needs to run
/// </summary>
/// It can have other nodes that it needs and nodes that depend on it.
public abstract class BuildTask: INode
{
    public abstract void Execute();
}