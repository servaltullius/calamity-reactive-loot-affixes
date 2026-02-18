using System.Text.Json;
using Mutagen.Bethesda.Skyrim;

namespace CalamityAffixes.Generator.Spec;

public static class AffixSpecLoader
{
    private const string ValidationContractRelativePath = "tools/affix_validation_contract.json";
    private static readonly char[] InvalidModKeyFileNameChars = ['<', '>', ':', '"', '/', '\\', '|', '?', '*'];
    private static readonly ValidationContract Contract = LoadValidationContract();
    private static readonly HashSet<string> SupportedTriggers = Contract.SupportedTriggers;
    private static readonly HashSet<string> SupportedActionTypes = Contract.SupportedActionTypes;

    public static AffixSpec Load(string path)
    {
        var json = File.ReadAllText(path);
        var options = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true,
        };

        var spec = JsonSerializer.Deserialize<AffixSpec>(json, options);
        if (spec is null)
        {
            throw new InvalidDataException($"Failed to parse spec JSON: {path}");
        }

        Validate(spec);
        return spec;
    }

    private static void Validate(AffixSpec spec)
    {
        ValidateModKey(spec.ModKey);

        if (!spec.ModKey.EndsWith(".esp", StringComparison.OrdinalIgnoreCase) &&
            !spec.ModKey.EndsWith(".esm", StringComparison.OrdinalIgnoreCase) &&
            !spec.ModKey.EndsWith(".esl", StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidDataException($"modKey must end with .esp/.esm/.esl (got: {spec.ModKey})");
        }

        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var seenMagicEffects = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var seenSpells = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var kw in spec.Keywords.Tags)
        {
            if (!seen.Add(kw.EditorId))
            {
                throw new InvalidDataException($"Duplicate keyword editorId: {kw.EditorId}");
            }
        }

        foreach (var affix in spec.Keywords.Affixes)
        {
            if (!seen.Add(affix.EditorId))
            {
                throw new InvalidDataException($"Duplicate keyword editorId: {affix.EditorId}");
            }

            ValidateRuntimeShape(affix);

            var records = affix.Records;
            if (records?.MagicEffect is { } magicEffect)
            {
                if (!seenMagicEffects.Add(magicEffect.EditorId))
                {
                    throw new InvalidDataException($"Duplicate MagicEffect editorId: {magicEffect.EditorId}");
                }

                if (!Enum.TryParse<ActorValue>(magicEffect.ActorValue, ignoreCase: true, out _))
                {
                    throw new InvalidDataException($"Unknown ActorValue: {magicEffect.ActorValue} (MagicEffect: {magicEffect.EditorId})");
                }
            }

            if (records?.Spell is { } spell)
            {
                if (!seenSpells.Add(spell.EditorId))
                {
                    throw new InvalidDataException($"Duplicate Spell editorId: {spell.EditorId}");
                }

                if (spell.Delivery is not ("Self" or "TargetActor"))
                {
                    throw new InvalidDataException($"Unknown Spell delivery: {spell.Delivery} (Spell: {spell.EditorId})");
                }

                if (string.IsNullOrWhiteSpace(spell.Effect.MagicEffectEditorId))
                {
                    throw new InvalidDataException($"Spell {spell.EditorId} requires effect.magicEffectEditorId");
                }
            }
        }
    }

    private static void ValidateRuntimeShape(AffixDefinition affix)
    {
        var runtime = JsonSerializer.SerializeToElement(affix.Runtime);
        if (runtime.ValueKind != JsonValueKind.Object)
        {
            throw new InvalidDataException($"{affix.Id}: runtime must be an object.");
        }

        if (!TryGetRequiredString(runtime, "trigger", out var trigger) || !SupportedTriggers.Contains(trigger))
        {
            throw new InvalidDataException($"{affix.Id}: runtime.trigger must be one of [{string.Join(", ", SupportedTriggers)}].");
        }

        if (TryGetOptionalNumber(runtime, "procChancePercent", affix.Id, out var procChancePercent))
        {
            if (procChancePercent is < 0.0 or > 100.0)
            {
                throw new InvalidDataException($"{affix.Id}: runtime.procChancePercent must be in range 0..100 (got: {procChancePercent}).");
            }
        }

        ValidateOptionalNonNegativeNumber(runtime, "icdSeconds", affix.Id);
        ValidateOptionalNonNegativeNumber(runtime, "perTargetICDSeconds", affix.Id);

        if (!runtime.TryGetProperty("action", out var action) || action.ValueKind != JsonValueKind.Object)
        {
            throw new InvalidDataException($"{affix.Id}: runtime.action must be an object.");
        }

        if (!TryGetRequiredString(action, "type", out var actionType) || !SupportedActionTypes.Contains(actionType))
        {
            throw new InvalidDataException($"{affix.Id}: runtime.action.type must be one of [{string.Join(", ", SupportedActionTypes)}].");
        }
    }

    private static bool TryGetRequiredString(JsonElement parent, string key, out string value)
    {
        value = string.Empty;
        if (!parent.TryGetProperty(key, out var property) || property.ValueKind != JsonValueKind.String)
        {
            return false;
        }

        var raw = property.GetString();
        if (string.IsNullOrWhiteSpace(raw))
        {
            return false;
        }

        value = raw;
        return true;
    }

    private static bool TryGetOptionalNumber(JsonElement parent, string key, string affixId, out double value)
    {
        value = 0.0;
        if (!parent.TryGetProperty(key, out var property))
        {
            return false;
        }

        if (property.ValueKind != JsonValueKind.Number || !property.TryGetDouble(out value))
        {
            throw new InvalidDataException($"{affixId}: runtime.{key} must be a number.");
        }

        return true;
    }

    private static void ValidateOptionalNonNegativeNumber(JsonElement parent, string key, string affixId)
    {
        if (!TryGetOptionalNumber(parent, key, affixId, out var value))
        {
            return;
        }

        if (value < 0.0)
        {
            throw new InvalidDataException($"{affixId}: runtime.{key} must be >= 0 (got: {value}).");
        }
    }

    private static ValidationContract LoadValidationContract()
    {
        var path = FindValidationContractPath();
        if (string.IsNullOrWhiteSpace(path) || !File.Exists(path))
        {
            return GetFallbackContract();
        }

        try
        {
            using var doc = JsonDocument.Parse(File.ReadAllText(path));
            var root = doc.RootElement;

            var triggers = ReadStringSet(root, "supportedTriggers");
            var actionTypes = ReadStringSet(root, "supportedActionTypes");
            return new ValidationContract(triggers, actionTypes);
        }
        catch
        {
            return GetFallbackContract();
        }
    }

    private static string? FindValidationContractPath()
    {
        var starts = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        void AddStart(string? candidate)
        {
            if (!string.IsNullOrWhiteSpace(candidate))
            {
                starts.Add(Path.GetFullPath(candidate));
            }
        }

        AddStart(Directory.GetCurrentDirectory());
        AddStart(AppContext.BaseDirectory);
        AddStart(Path.GetDirectoryName(typeof(AffixSpecLoader).Assembly.Location));

        foreach (var start in starts)
        {
            var current = start;
            while (!string.IsNullOrWhiteSpace(current))
            {
                var candidate = Path.Combine(current, ValidationContractRelativePath);
                if (File.Exists(candidate))
                {
                    return candidate;
                }

                var parent = Directory.GetParent(current);
                if (parent is null)
                {
                    break;
                }
                current = parent.FullName;
            }
        }

        return null;
    }

    private static HashSet<string> ReadStringSet(JsonElement root, string key)
    {
        if (!root.TryGetProperty(key, out var property) || property.ValueKind != JsonValueKind.Array)
        {
            throw new InvalidDataException($"Validation contract missing array: {key}");
        }

        var output = new HashSet<string>(StringComparer.Ordinal);
        foreach (var item in property.EnumerateArray())
        {
            if (item.ValueKind != JsonValueKind.String)
            {
                throw new InvalidDataException($"Validation contract '{key}' entries must be strings.");
            }

            var value = item.GetString();
            if (string.IsNullOrWhiteSpace(value))
            {
                throw new InvalidDataException($"Validation contract '{key}' contains an empty value.");
            }
            output.Add(value);
        }

        if (output.Count == 0)
        {
            throw new InvalidDataException($"Validation contract '{key}' must not be empty.");
        }

        return output;
    }

    private static ValidationContract GetFallbackContract()
    {
        return new ValidationContract(
            new HashSet<string>(StringComparer.Ordinal)
            {
                "Hit",
                "IncomingHit",
                "DotApply",
                "Kill",
            },
            new HashSet<string>(StringComparer.Ordinal)
            {
                "DebugNotify",
                "CastSpell",
                "CastSpellAdaptiveElement",
                "CastOnCrit",
                "ConvertDamage",
                "MindOverMatter",
                "Archmage",
                "CorpseExplosion",
                "SummonCorpseExplosion",
                "SpawnTrap",
            });
    }

    private sealed record ValidationContract(HashSet<string> SupportedTriggers, HashSet<string> SupportedActionTypes);

    private static void ValidateModKey(string modKey)
    {
        if (string.IsNullOrWhiteSpace(modKey))
        {
            throw new InvalidDataException("modKey must be a non-empty file name.");
        }

        if (Path.IsPathRooted(modKey))
        {
            throw new InvalidDataException($"modKey must not be rooted: {modKey}");
        }

        if (!string.Equals(modKey, Path.GetFileName(modKey), StringComparison.Ordinal))
        {
            throw new InvalidDataException($"modKey must be a file name only (no path segments): {modKey}");
        }

        if (modKey.IndexOfAny(InvalidModKeyFileNameChars) >= 0)
        {
            throw new InvalidDataException($"modKey contains invalid file name characters: {modKey}");
        }

        if (modKey.EndsWith(' ') || modKey.EndsWith('.'))
        {
            throw new InvalidDataException($"modKey must not end with space or dot: {modKey}");
        }
    }
}
