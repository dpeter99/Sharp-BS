using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Dynamic;
using System.Threading.Tasks;
using Microsoft.ClearScript;
using Sharp_BS.Model;

namespace Sharp_BS.JSInterop
{
    public static class JSInteropRunner
    {
        private static ObjectTracker marshaledObjs = new ObjectTracker();

        public static void AddObject(string name, object o)
        {
            
        }

        public static PropertyBag MarshalTo(object parameter)
        {
            PropertyBag m_obj;
            
            if (parameter is ICustomConverter marhal)
            {
                //Do the custom marshaling
                m_obj = marhal.MarshalTo();
            }
            else
            {
                m_obj = new PropertyBag();
            }

            m_obj["UUID"] = marshaledObjs.TrackObject(parameter);
            
            return m_obj;
        }
        
        public static void MarshalBack(PropertyBag parameters)
        {
           
            
            Guid id = (Guid) parameters["UUID"];

            var o = marshaledObjs.GetObject(id);

            if (o is ICustomConverter marshal)
            {
                marshal.MarshalBack(parameters);
            }
        }

        public static Action<Project> MakeAction(object JSAction, HostFunctions _hostFunctions)
        {
            var action = (Action<object>) _hostFunctions.proc(1, JSAction);

            return (p) =>
            {
                //Marshal p
                var m_objs = MarshalTo(p);

                //Run function
                action.DynamicInvoke(m_objs);

                //Bring back p
                MarshalBack(m_objs);

            };
        }
        
        
    }
}