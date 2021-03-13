using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json.Serialization;

namespace Sharp_BS.Model
{
    public class SourceSet
    {
        public SourceSet(string name)
        {
            Name = name;
            Patterns = new List<string>();
        }

        public string Name { get; set; }
        public List<string> Patterns { get; set; }


        [JsonIgnore]
        public List<FileInfo> Files = new List<FileInfo>();

        public void ExpandFileList()
        {
            foreach (var pattern in Patterns)
            {
                bool negative = false;
                
                var path = pattern;
                if (pattern.StartsWith("-"))
                {
                    path = Path.GetFullPath(pattern[1..]);
                }

                var path_done = BSHost.TemplateString(path);

                var end = path_done.LastIndexOf('/');
                var folder = Path.GetDirectoryName(path_done);

                var fileList = Directory.EnumerateFiles(folder, folder.Substring(end)).Select(f=>new FileInfo(f)).ToList();

                if (!negative)
                {
                    Files.AddRange(fileList);
                }
                else
                {
                    
                }
                
                
            }
        }
        
    }
}