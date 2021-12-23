using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace SharpBS.Model
{
    public class ProjectConfig
    {
        [JsonPropertyName("name")]
        public string Name { get; set; }

        [JsonPropertyName("plugins")] 
        public List<string> Plugins { get; set; } = new();
        
        [JsonPropertyName("steps")]
        public List<object> Steps { get; set; }
        
        [JsonPropertyName("files")]
        public Dictionary<string,List<string>> Files { get; set; }
        
        [JsonPropertyName("variables")]
        public Dictionary<string,string> Variables { get; set; }
    }
}