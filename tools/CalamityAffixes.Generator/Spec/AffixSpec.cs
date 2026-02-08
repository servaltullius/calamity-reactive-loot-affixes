using System.Text.Json.Serialization;

namespace CalamityAffixes.Generator.Spec;

public sealed class AffixSpec
{
    [JsonPropertyName("version")]
    public required int Version { get; init; }

    [JsonPropertyName("modKey")]
    public required string ModKey { get; init; }

    [JsonPropertyName("eslFlag")]
    public required bool EslFlag { get; init; }

    [JsonPropertyName("loot")]
    public LootSpec? Loot { get; init; }

    [JsonPropertyName("keywords")]
    public required KeywordSpec Keywords { get; init; }
}

public sealed class LootSpec
{
    [JsonPropertyName("chancePercent")]
    public double ChancePercent { get; init; }

    // Chance (0-100) to grant a random runeword fragment when an affix is applied to a new item instance.
    [JsonPropertyName("runewordFragmentChancePercent")]
    public double RunewordFragmentChancePercent { get; init; } = 1.0;

    [JsonPropertyName("renameItem")]
    public bool RenameItem { get; init; }

    [JsonPropertyName("sharedPool")]
    public bool SharedPool { get; init; }

    [JsonPropertyName("debugLog")]
    public bool DebugLog { get; init; }

    // Safety cap for trap-style procs (SpawnTrap) across all affixes.
    // 0 = unlimited (not recommended).
    [JsonPropertyName("trapGlobalMaxActive")]
    public int TrapGlobalMaxActive { get; init; } = 64;

    [JsonPropertyName("nameFormat")]
    public string? NameFormat { get; init; }
}

public sealed class KeywordSpec
{
    [JsonPropertyName("tags")]
    public required List<KeywordDefinition> Tags { get; init; }

    [JsonPropertyName("affixes")]
    public required List<AffixDefinition> Affixes { get; init; }

    [JsonPropertyName("kidRules")]
    public required List<KidRuleEntry> KidRules { get; init; }

    [JsonPropertyName("spidRules")]
    public required List<Dictionary<string, object?>> SpidRules { get; init; }
}

public sealed class KeywordDefinition
{
    [JsonPropertyName("editorId")]
    public required string EditorId { get; init; }

    [JsonPropertyName("name")]
    public required string Name { get; init; }
}

public sealed class AffixDefinition
{
    [JsonPropertyName("id")]
    public required string Id { get; init; }

    [JsonPropertyName("editorId")]
    public required string EditorId { get; init; }

    [JsonPropertyName("name")]
    public required string Name { get; init; }

    [JsonPropertyName("records")]
    public AffixRecordSpec? Records { get; init; }

    [JsonPropertyName("kid")]
    public required KidRule Kid { get; init; }

    [JsonPropertyName("runtime")]
    public required Dictionary<string, object?> Runtime { get; init; }
}

public sealed class AffixRecordSpec
{
    [JsonPropertyName("magicEffect")]
    public MagicEffectRecordSpec? MagicEffect { get; init; }

    [JsonPropertyName("spell")]
    public SpellRecordSpec? Spell { get; init; }
}

public sealed class MagicEffectRecordSpec
{
    [JsonPropertyName("editorId")]
    public required string EditorId { get; init; }

    [JsonPropertyName("name")]
    public string? Name { get; init; }

    [JsonPropertyName("actorValue")]
    public required string ActorValue { get; init; }

    [JsonPropertyName("resistValue")]
    public string? ResistValue { get; init; }

    [JsonPropertyName("magicSkill")]
    public string? MagicSkill { get; init; }

    [JsonPropertyName("hostile")]
    public bool Hostile { get; init; }

    [JsonPropertyName("recover")]
    public bool Recover { get; init; }
}

public sealed class SpellRecordSpec
{
    [JsonPropertyName("editorId")]
    public required string EditorId { get; init; }

    [JsonPropertyName("name")]
    public string? Name { get; init; }

    [JsonPropertyName("delivery")]
    public required string Delivery { get; init; }

    [JsonPropertyName("effect")]
    public required SpellEffectRecordSpec Effect { get; init; }
}

public sealed class SpellEffectRecordSpec
{
    [JsonPropertyName("magicEffectEditorId")]
    public required string MagicEffectEditorId { get; init; }

    [JsonPropertyName("magnitude")]
    public double Magnitude { get; init; }

    [JsonPropertyName("duration")]
    public int Duration { get; init; }

    [JsonPropertyName("area")]
    public int Area { get; init; }
}

public sealed class KidRule
{
    [JsonPropertyName("type")]
    public required string Type { get; init; }

    [JsonPropertyName("strings")]
    public required string Strings { get; init; }

    [JsonPropertyName("formFilters")]
    public required string FormFilters { get; init; }

    [JsonPropertyName("traits")]
    public required string Traits { get; init; }

    [JsonPropertyName("chance")]
    public required double Chance { get; init; }
}

public sealed class KidRuleEntry
{
    [JsonPropertyName("comment")]
    public string? Comment { get; init; }

    [JsonPropertyName("keywordEditorId")]
    public required string KeywordEditorId { get; init; }

    [JsonPropertyName("type")]
    public required string Type { get; init; }

    [JsonPropertyName("strings")]
    public required string Strings { get; init; }

    [JsonPropertyName("formFilters")]
    public required string FormFilters { get; init; }

    [JsonPropertyName("traits")]
    public required string Traits { get; init; }

    [JsonPropertyName("chance")]
    public required double Chance { get; init; }
}
