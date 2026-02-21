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

    // Chance (0-100) to grant a random runeword fragment on eligible loot-roll events.
    [JsonPropertyName("runewordFragmentChancePercent")]
    public double RunewordFragmentChancePercent { get; init; } = 5.0;

    [JsonPropertyName("reforgeOrbChancePercent")]
    public double ReforgeOrbChancePercent { get; init; } = 3.0;

    // Currency drop policy is now hybrid-only.
    // Runtime enforces 'hybrid' even if legacy values are supplied for backward compatibility.
    [JsonPropertyName("currencyDropMode")]
    public string CurrencyDropMode { get; init; } = "hybrid";

    // Optional explicit LVLI targets used by hybrid distribution path.
    // Format: "ModName.esm|00ABCDEF".
    // If omitted/empty, generator uses curated defaults.
    [JsonPropertyName("currencyLeveledListTargets")]
    public List<string>? CurrencyLeveledListTargets { get; init; }

    // If true, merge auto-discovered mod death-item lists from --masters into currency targets.
    // Discovery includes both leveled lists named DeathItem* and lists referenced by mod NPC DeathItem links.
    [JsonPropertyName("currencyLeveledListAutoDiscoverDeathItems")]
    public bool CurrencyLeveledListAutoDiscoverDeathItems { get; init; } = true;

    [JsonPropertyName("lootSourceChanceMultCorpse")]
    public double LootSourceChanceMultCorpse { get; init; } = 0.8;

    [JsonPropertyName("lootSourceChanceMultContainer")]
    public double LootSourceChanceMultContainer { get; init; } = 1.0;

    [JsonPropertyName("lootSourceChanceMultBossContainer")]
    public double LootSourceChanceMultBossContainer { get; init; } = 1.35;

    [JsonPropertyName("lootSourceChanceMultWorld")]
    public double LootSourceChanceMultWorld { get; init; } = 1.0;

    [JsonPropertyName("renameItem")]
    public bool RenameItem { get; init; }

    [JsonPropertyName("nameMarkerPosition")]
    public string? NameMarkerPosition { get; init; }

    [JsonPropertyName("sharedPool")]
    public bool SharedPool { get; init; }

    [JsonPropertyName("debugLog")]
    public bool DebugLog { get; init; }

    // If true, DotApply trigger handling auto-disables when CAFF_TAG_DOT coverage is detected as too broad.
    [JsonPropertyName("dotTagSafetyAutoDisable")]
    public bool DotTagSafetyAutoDisable { get; init; } = false;

    // Unique tagged MGEF threshold for DotApply safety warning/auto-disable (0 = disabled).
    [JsonPropertyName("dotTagSafetyUniqueEffectThreshold")]
    public int DotTagSafetyUniqueEffectThreshold { get; init; } = 96;

    // Safety cap for trap-style procs (SpawnTrap) across all affixes.
    // 0 = unlimited (not recommended).
    [JsonPropertyName("trapGlobalMaxActive")]
    public int TrapGlobalMaxActive { get; init; } = 64;

    // Tick-level cast budget for SpawnTrap processing (0 = unlimited).
    [JsonPropertyName("trapCastBudgetPerTick")]
    public int TrapCastBudgetPerTick { get; init; } = 8;

    // Trigger proc budget in fixed window (0 = unlimited).
    [JsonPropertyName("triggerProcBudgetPerWindow")]
    public int TriggerProcBudgetPerWindow { get; init; } = 12;

    // Window size for Trigger proc budget in milliseconds (0 = unlimited).
    [JsonPropertyName("triggerProcBudgetWindowMs")]
    public int TriggerProcBudgetWindowMs { get; init; } = 100;

    // If true, removes stale legacy affix mappings from items that are no longer loot-eligible.
    [JsonPropertyName("cleanupInvalidLegacyAffixes")]
    public bool CleanupInvalidLegacyAffixes { get; init; } = true;

    // If true, strips tracked suffix slots from mapped item instances during runtime sanitization.
    [JsonPropertyName("stripTrackedSuffixSlots")]
    public bool StripTrackedSuffixSlots { get; init; } = true;

    // Case-insensitive EditorID substrings that disqualify armor-like consumable reward boxes from loot affixes.
    [JsonPropertyName("armorEditorIdDenyContains")]
    public List<string>? ArmorEditorIdDenyContains { get; init; }

    [JsonPropertyName("bossContainerEditorIdAllowContains")]
    public List<string>? BossContainerEditorIdAllowContains { get; init; }

    [JsonPropertyName("bossContainerEditorIdDenyContains")]
    public List<string>? BossContainerEditorIdDenyContains { get; init; }

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

    [JsonPropertyName("nameEn")]
    public string? NameEn { get; init; }

    [JsonPropertyName("nameKo")]
    public string? NameKo { get; init; }

    [JsonPropertyName("records")]
    public AffixRecordSpec? Records { get; init; }

    [JsonPropertyName("kid")]
    public required KidRule Kid { get; init; }

    [JsonPropertyName("slot")]
    public string? Slot { get; init; }

    [JsonPropertyName("family")]
    public string? Family { get; init; }

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

    [JsonPropertyName("archetype")]
    public string? Archetype { get; init; }
}

public sealed class SpellRecordSpec
{
    [JsonPropertyName("editorId")]
    public required string EditorId { get; init; }

    [JsonPropertyName("name")]
    public string? Name { get; init; }

    [JsonPropertyName("delivery")]
    public required string Delivery { get; init; }

    [JsonPropertyName("spellType")]
    public string? SpellType { get; init; }

    [JsonPropertyName("castType")]
    public string? CastType { get; init; }

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
