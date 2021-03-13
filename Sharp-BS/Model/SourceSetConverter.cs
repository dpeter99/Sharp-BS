using System;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Sharp_BS.Model
{
    public class SourceSetConverter : JsonConverter<Sources>
    {
        public override Sources? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            Sources set = new Sources();
            
            using (JsonDocument document = JsonDocument.ParseValue(ref reader))
            {
                if (document.RootElement.ValueKind != JsonValueKind.Object)
                {
                    throw new Exception("[JSON] Ohh no!");
                }

                var root = document.RootElement;

                foreach (var property in root.EnumerateObject())
                {
                    var patterns = new SourceSet(property.Name);

                    if (property.Value.ValueKind != JsonValueKind.Array)
                        throw new Exception("[JSON] More nooo!");

                    foreach (var p in property.Value.EnumerateArray())
                    {
                        patterns.Patterns.Add(p.GetString() ?? string.Empty);
                    }

                    set.Sets.TryAdd(property.Name, patterns);
                }
            }

            return set;
        }

        public override void Write(Utf8JsonWriter writer, Sources value, JsonSerializerOptions options)
        {
            throw new NotImplementedException();
        }
    }
}