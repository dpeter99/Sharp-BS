using System;

namespace Util.Singleton
{
    class SingletonMultipleInstanceException: Exception
    {
        public SingletonMultipleInstanceException(Type type) 
            : base($"Singleton type: {type.Name} was instanciated more than once")
        {

        }
    }
}
