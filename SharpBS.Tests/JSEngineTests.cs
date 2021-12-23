using NUnit.Framework;
using SharpBS.Plugins;

namespace SharpBS.Tests;

[TestFixture]
public class JSEngineTests
{
    [SetUp]
    public void Setup()
    {
        var eng = new JSEngine();
    }
    
    [Test]
    public void JSEngineLoadPlugin()
    {
        JSEngine.Instance.LoadJs("../../../../Examples/.bs/plugins/cc.js", null);
    }
}