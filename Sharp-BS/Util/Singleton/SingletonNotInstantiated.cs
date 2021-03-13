using System;

namespace Util.Singleton
{
    class SingletonNotInstantiated: Exception
    {
        public SingletonNotInstantiated(Type type):base($"{type.Name} is not yet Instantiated")
        {

        }
    }
}
