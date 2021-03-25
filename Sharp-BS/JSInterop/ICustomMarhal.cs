using System;
using System.Dynamic;
using Microsoft.ClearScript;

namespace Sharp_BS.JSInterop
{
    public interface ICustomConverter
    {
        PropertyBag MarshalTo();
        void MarshalBack(PropertyBag parameters);
    }

    [AttributeUsage(AttributeTargets.Class)]
    public class JSMarshalAttribute: Attribute
    {
        public readonly ICustomConverter _converter;

        public JSMarshalAttribute(ICustomConverter converter)
        {
            _converter = converter;
        }
    }
}