using System;

namespace SharpBS.Utils;

public static class Accesors
{
    public static T apply<T>(this T a, Action<T> f)
    {
        f(a);
        return a;
    }
    
    public static R Let<T, R>(this T self, Func<T, R> block) 
    {
        return block(self);
    }
    
}