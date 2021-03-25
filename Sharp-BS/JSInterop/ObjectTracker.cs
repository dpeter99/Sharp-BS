using System;
using System.Collections.Generic;
using System.Linq;

namespace Sharp_BS.JSInterop
{
    public class ObjectTracker
    {
        public Dictionary<Guid, object> objects = new Dictionary<Guid, object>();

        public Guid TrackObject(object o)
        {
            if (objects.ContainsValue(o))
            {
                return objects.FirstOrDefault(i => i.Value.Equals(o)).Key;
            }

            var newGuid = Guid.NewGuid();
            objects.Add(newGuid, o);

            return newGuid;
        }
        
        public object? GetObject(Guid k)
        {
            if (objects.ContainsKey(k))
            {
                return objects[k];
            }

            return null;
        }
    }
}