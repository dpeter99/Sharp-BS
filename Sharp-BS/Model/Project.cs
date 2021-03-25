using System.Collections.Generic;
using System.Dynamic;
using System.IO;
using System.Text;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;
using Microsoft.ClearScript;
using Sharp_BS.JSInterop;

namespace Sharp_BS.Model
{
    
    public class Project : ICustomConverter
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

        public PropertyBag MarshalTo()
        {
            var expando = new PropertyBag();
            expando["name"] = this.Name;
            expando["author"] = this.Author;

            expando["build"] = Build.MarshalTo();
            
            return expando;
        }

        public void MarshalBack(PropertyBag parameters)
        {
            var p = parameters as IDictionary<string, object>;
            
            this.Name = p["name"].ToString();
            this.Author = p["author"].ToString();
            
            Build.MarshalBack((PropertyBag) p["build"]);
            
            return;
        }
    }
}