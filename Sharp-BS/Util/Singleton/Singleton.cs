namespace Util.Singleton
{
    public class Singleton<T> where T : Singleton<T>
    {
        static T? _instance;
        public static T Instance
        {
            get
            {
                if (_instance == null)
                    throw new SingletonNotInstantiated(typeof(T));
                return _instance;
            }

            internal set
            {
                if (_instance != null)
                    throw new SingletonMultipleInstanceException(typeof(T));
                _instance = value;
            }
        }

        public Singleton()
        {
            Instance = (T)this;
        }
    }
}
