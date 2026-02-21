using System.Text.Json;
using System.Text.RegularExpressions;

namespace CalamityAffixes.Generator.Tests;

public sealed class RuntimeContractSyncTests
{
    [Fact]
    public void RuntimeContractHeaderAndValidationContract_AreInSync()
    {
        var repoRoot = FindRepoRoot();
        var headerPath = Path.Combine(
            repoRoot,
            "skse",
            "CalamityAffixes",
            "include",
            "CalamityAffixes",
            "RuntimeContract.h");
        var validationContractPath = Path.Combine(repoRoot, "tools", "affix_validation_contract.json");

        Assert.True(File.Exists(headerPath), $"RuntimeContract header not found: {headerPath}");
        Assert.True(File.Exists(validationContractPath), $"Validation contract JSON not found: {validationContractPath}");

        var headerText = File.ReadAllText(headerPath);

        var headerTriggers = ExtractConstantValues(
            headerText,
            @"inline constexpr std::string_view\s+kTrigger[A-Za-z0-9_]+\s*=\s*""([^""]+)"";");
        var headerActionTypes = ExtractConstantValues(
            headerText,
            @"inline constexpr std::string_view\s+kAction[A-Za-z0-9_]+\s*=\s*""([^""]+)"";");

        using var jsonDoc = JsonDocument.Parse(File.ReadAllText(validationContractPath));
        var root = jsonDoc.RootElement;

        var jsonTriggers = root
            .GetProperty("supportedTriggers")
            .EnumerateArray()
            .Select(static element => element.GetString())
            .Where(static value => !string.IsNullOrWhiteSpace(value))
            .Select(static value => value!)
            .ToHashSet(StringComparer.Ordinal);

        var jsonActionTypes = root
            .GetProperty("supportedActionTypes")
            .EnumerateArray()
            .Select(static element => element.GetString())
            .Where(static value => !string.IsNullOrWhiteSpace(value))
            .Select(static value => value!)
            .ToHashSet(StringComparer.Ordinal);

        Assert.True(
            headerTriggers.SetEquals(jsonTriggers),
            BuildMismatchMessage("supportedTriggers", headerTriggers, jsonTriggers));
        Assert.True(
            headerActionTypes.SetEquals(jsonActionTypes),
            BuildMismatchMessage("supportedActionTypes", headerActionTypes, jsonActionTypes));
    }

    private static HashSet<string> ExtractConstantValues(string sourceText, string pattern)
    {
        var values = Regex
            .Matches(sourceText, pattern, RegexOptions.CultureInvariant)
            .Select(static match => match.Groups[1].Value)
            .Where(static value => !string.IsNullOrWhiteSpace(value))
            .ToHashSet(StringComparer.Ordinal);

        Assert.NotEmpty(values);
        return values;
    }

    private static string BuildMismatchMessage(
        string fieldName,
        IReadOnlySet<string> headerValues,
        IReadOnlySet<string> jsonValues)
    {
        static string JoinOrDash(IEnumerable<string> values)
        {
            var ordered = values.OrderBy(static value => value, StringComparer.Ordinal).ToArray();
            return ordered.Length > 0 ? string.Join(", ", ordered) : "-";
        }

        var onlyInHeader = headerValues.Except(jsonValues, StringComparer.Ordinal);
        var onlyInJson = jsonValues.Except(headerValues, StringComparer.Ordinal);

        return $"{fieldName} mismatch. " +
               $"onlyInHeader=[{JoinOrDash(onlyInHeader)}], " +
               $"onlyInJson=[{JoinOrDash(onlyInJson)}].";
    }

    private static string FindRepoRoot()
    {
        var dir = new DirectoryInfo(AppContext.BaseDirectory);
        while (dir is not null)
        {
            if (File.Exists(Path.Combine(dir.FullName, "affixes", "affixes.json")))
            {
                return dir.FullName;
            }

            dir = dir.Parent;
        }

        throw new DirectoryNotFoundException("Failed to locate repo root (affixes/affixes.json not found above test output directory).");
    }
}
