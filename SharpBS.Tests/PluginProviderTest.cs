using System.Linq;
using NUnit.Framework;
using SharpBS.Model;
using SharpBS.Plugins;

namespace SharpBS.Tests;

[Plugin("TestPlugin")]
public class TestPlugin: IPlugin
{
    
}

[TestFixture]
public class PluginProviderTest
{

    [Test]
    public void TestPluginIsFound()
    {
        //var p = PluginProvider.GetPlugin("TestPlugin");
        var found = PluginProvider._typesWithMyAttribute.Exists((tuple => tuple.Type == typeof(TestPlugin)));
        
        Assert.IsTrue(found);
    }

    [Test]
    public void GetInstanceOfTestPlugin()
    {
        var p = PluginProvider.GetPlugin("TestPlugin");
        
        Assert.IsInstanceOf<TestPlugin>(p);
    }

}