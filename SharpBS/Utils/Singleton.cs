using System;

namespace SharpBS.Utils;

public class Singleton<T> where T : Singleton<T>, new()
{
    private static bool autoInit = false;
    
    private static T? _instance;
    public static T Instance
    {
        get
        {
            if (_instance == null)
            {
                throw new SingletonNotInitialized(typeof(T));
            }

            if (autoInit)
            {
                _instance = new T();
            }
            
            return _instance;
        }
    }

    public Singleton()
    {
        if (_instance == null)
        {
            _instance = this as T;
        }
        else
        {
            throw new SingletonMultipleInit(typeof(T));
        }
        
    }
}

public class SingletonNotInitialized : Exception
{
    public SingletonNotInitialized(Type type): base(type.ToString()+" was initialized multiple times")
    {
        
    }
}

public class SingletonMultipleInit : Exception
{
    public SingletonMultipleInit(Type type): base(type.ToString()+" was initialized multiple times")
    {
        
    }
}