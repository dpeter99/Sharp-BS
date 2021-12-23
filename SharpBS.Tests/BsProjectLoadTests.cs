using NUnit.Framework;

namespace SharpBS.Tests;

public class BsProjectLoadTests
{
    
    
    [SetUp]
    public void Setup()
    {
        
    }

    [Test]
    public void ProjectNameIsLoaded()
    {
        var proj = BSProject.FromJsonString("{\"name\": \"Chroma\"}");
        
        Assert.AreEqual(proj.Name, "Chroma");
    }
    
    
    [Test]
    public void TestPluginIsLoaded()
    {
        var proj = BSProject.FromJsonString("{\"name\": \"Chroma\", \"plugins\":[\"TestPlugin\"]}");

        var found = proj.plugins.Exists(p => typeof(TestPlugin).IsInstanceOfType(p));
        
        Assert.IsTrue(found);
    }
}