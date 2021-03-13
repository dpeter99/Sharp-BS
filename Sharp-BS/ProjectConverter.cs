using Jint;
using Jint.Native;
using Jint.Native.Object;
using Jint.Native.String;
using Jint.Runtime.Descriptors;
using Jint.Runtime.Interop;
using Sharp_BS.Model;

namespace Sharp_BS
{
    public class ProjectConverter : Jint.Runtime.Interop.IObjectConverter
    {
        public bool TryConvert(Engine engine, object value, out JsValue result)
        {
            if (value is Project project)
            {
                result = new ObjectInstance(engine);
                ObjectInstance res = (result as ObjectInstance)!;

                res.DefineOwnProperty(new JsString("name"),new PropertyDescriptor(new JsString(project.Name),true,false,false));

                return true;
            }
            result = null;
            return false;
        }

        //public object Target { get; }
    }
}