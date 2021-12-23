using System;
using System.IO;

namespace SharpBS.Utils;

public static class FileUtils
{
    public static FileInfo GetFile(string path)
    {
        path = Path.GetFullPath(path);
            
        FileInfo file = new FileInfo(path);
        if (!file.Exists)
            throw new Exception("[PROJECT] I miss you.\n There is no json file at: " + path);

        return file;
    }
}