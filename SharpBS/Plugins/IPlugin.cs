using System.Text.Json;

namespace SharpBS.Plugins
{
    public interface IPlugin
    {
        string Name { get; }
        Step GetStep(string name, JsonElement config);
    }
}