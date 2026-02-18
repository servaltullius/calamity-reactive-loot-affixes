using System.Text.Json;
using Mutagen.Bethesda.Skyrim;

namespace CalamityAffixes.Generator.Spec;

public static class AffixSpecLoader
{
    private static readonly char[] InvalidModKeyFileNameChars = ['<', '>', ':', '"', '/', '\\', '|', '?', '*'];

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
