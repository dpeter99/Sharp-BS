using System;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Sharp_BS.Model
{
    public class BuildConverter : JsonConverter<Build>
    {
        public override Build? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            Build set = new();
            
            using (JsonDocument document = JsonDocument.ParseValue(ref reader))
            {
                if (document.RootElement.ValueKind != JsonValueKind.Object)
                {
                    throw new Exception("[JSON] Ohh no!");
                }

                var root = document.RootElement;

                foreach (var property in root.EnumerateObject())
                {
                    var patterns = new Step(property.Name);

                    if (property.Value.ValueKind != JsonValueKind.Array)
                        throw new Exception("[JSON] More nooo!");

                    foreach (var p in property.Value.EnumerateArray())
                    {
                        patterns.args.Add(p.GetString() ?? string.Empty);
                    }

                    set.steps.Add(patterns);
                }
            }

            return set;
        }

        public override void Write(Utf8JsonWriter writer, Build value, JsonSerializerOptions options)
        {
            throw new NotImplementedException();
        }
    }
}