using System;
using System.Collections.Generic;
using System.Collections.Immutable;

namespace Sharp_BS
{
    public class Registry<T>
    {
        public ImmutableDictionary<string, T> Items => _items.ToImmutableDictionary();

        private Dictionary<string, T> _items = new();

        public void Register(string id, T i)
        {
            _items.Add(key:id,i);
        }

        public T? Get(string key)
        {
            if (_items.ContainsKey(key))
            {
                return _items[key];
            }

            return default(T);
        }
        
    }
}