using System.Text.Json;
using System.Text.Json.Serialization;
using CalamityAffixes.Generator.Spec;

namespace CalamityAffixes.Generator.Writers;

public static class InventoryInjectorJsonWriter
{
    private const string SchemaUrl = "https://raw.githubusercontent.com/Exit-9B/InventoryInjector/main/docs/InventoryInjector.schema.json";

    public static string Render(AffixSpec spec)
    {
        var config = new InventoryInjectorConfig
        {
            Schema = SchemaUrl,
            // IMPORTANT:
            // InventoryInjector's "text" key maps to SkyUI list entry display name.
            // We intentionally do NOT inject "text" for affixes, because that makes it look like the item's name
            // changed. Affix UX is provided via the Prisma UI overlay at runtime.
            Rules = [],
        };

        return JsonSerializer.Serialize(config, new JsonSerializerOptions { WriteIndented = true });
    }

    private sealed class InventoryInjectorConfig
    {
        [JsonPropertyName("$schema")]
        public required string Schema { get; init; }

        [JsonPropertyName("rules")]
        public required List<InventoryInjectorRule> Rules { get; init; }
    }

    private sealed class InventoryInjectorRule
    {
        [JsonPropertyName("match")]
        public required InventoryInjectorMatch Match { get; init; }

        [JsonPropertyName("assign")]
        public required InventoryInjectorAssign Assign { get; init; }
    }

    private sealed class InventoryInjectorMatch
    {
        [JsonPropertyName("formType")]
        public required string FormType { get; init; }

        [JsonPropertyName("keywords")]
        public required string Keywords { get; init; }
    }

    private sealed class InventoryInjectorAssign
    {
        [JsonPropertyName("text")]
        public required string Text { get; init; }
    }
}
