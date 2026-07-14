using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;

namespace CalamityAffixes.Generator.Tests;

public sealed class MagicEffectContractTests
{
    [Fact]
    public void BuildKeywordPlugin_WhenSharedMagicEffectChangesCastContract_FailsHard()
    {
        var sharedMagicEffect = new MagicEffectRecordSpec
        {
            EditorId = "CAFF_MGEF_SHARED_CONFLICT",
            ActorValue = "Health",
        };
        var constantSpell = new SpellRecordSpec
        {
            EditorId = "CAFF_SPEL_SHARED_CONSTANT",
            Delivery = "Self",
            SpellType = "Ability",
            CastType = "ConstantEffect",
            Effect = Effect(sharedMagicEffect.EditorId),
        };
        var fireAndForgetSpell = new SpellRecordSpec
        {
            EditorId = "CAFF_SPEL_SHARED_FIRE_AND_FORGET",
            Delivery = "Self",
            SpellType = "Spell",
            CastType = "FireAndForget",
            Effect = Effect(sharedMagicEffect.EditorId),
        };

        var spec = new AffixSpec
        {
            Version = 1,
            ModKey = "CalamityAffixes.esp",
            EslFlag = true,
            Keywords = new KeywordSpec
            {
                Tags = [],
                Affixes =
                [
                    Affix(
                        "constant",
                        new AffixRecordSpec
                        {
                            MagicEffect = sharedMagicEffect,
                            Spell = constantSpell,
                        }),
                    Affix(
                        "fire_and_forget",
                        new AffixRecordSpec
                        {
                            Spell = fireAndForgetSpell,
                        }),
                ],
                KidRules = [],
                SpidRules = [],
            },
        };

        var ex = Assert.Throws<InvalidDataException>(() => KeywordPluginBuilder.Build(spec));
        Assert.Contains("conflicting spell contracts", ex.Message, StringComparison.OrdinalIgnoreCase);
        Assert.Contains(constantSpell.EditorId, ex.Message, StringComparison.Ordinal);
        Assert.Contains(fireAndForgetSpell.EditorId, ex.Message, StringComparison.Ordinal);
    }

    private static SpellEffectRecordSpec Effect(string magicEffectEditorId)
    {
        return new SpellEffectRecordSpec
        {
            MagicEffectEditorId = magicEffectEditorId,
            Magnitude = 1.0,
            Duration = 0,
            Area = 0,
        };
    }

    private static AffixDefinition Affix(string id, AffixRecordSpec records)
    {
        return new AffixDefinition
        {
            Id = id,
            EditorId = $"CAFF_AFFIX_{id.ToUpperInvariant()}",
            Name = id,
            Records = records,
            Kid = new KidRule
            {
                Type = "None",
                Strings = "NONE",
                FormFilters = "NONE",
                Traits = "-E",
                Chance = 0.0,
            },
            Runtime = new RuntimeSpec
            {
                Trigger = "Hit",
                Action = new Dictionary<string, object?> { ["type"] = "DebugNotify" },
            },
        };
    }
}
