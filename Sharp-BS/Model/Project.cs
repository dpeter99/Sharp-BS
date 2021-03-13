using System.IO;
using System.Text;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;

namespace Sharp_BS.Model
{
    
    public class Project
    {
        [JsonPropertyName("name")]
        public string Name { get; set; }
        
        [JsonPropertyName("author")]
        public string Author { get; set; }

        [JsonConverter(typeof(SourceSetConverter))]
        [JsonPropertyName("source")]
        public Sources Sources { get; set; }
        
        [JsonConverter(typeof(BuildConverter))]
        [JsonPropertyName("build")]
        public Build Build { get; set; }
    }
}