using CalamityAffixes.Generator.Spec;
using CalamityAffixes.Generator.Writers;

static void PrintUsage()
{
    Console.WriteLine("CalamityAffixes.Generator");
    Console.WriteLine();
    Console.WriteLine("Usage:");
    Console.WriteLine("  dotnet run --project tools/CalamityAffixes.Generator -- [--spec <path>] [--data <path>]");
    Console.WriteLine();
    Console.WriteLine("Defaults:");
    Console.WriteLine("  --spec affixes/affixes.json");
    Console.WriteLine("  --data Data");
}

string specPath = "affixes/affixes.json";
string dataDir = "Data";

for (var i = 0; i < args.Length; i++)
{
    var arg = args[i];
    switch (arg)
    {
        case "--spec" when i + 1 < args.Length:
            specPath = args[++i];
            break;
        case "--data" when i + 1 < args.Length:
            dataDir = args[++i];
            break;
        case "-h":
        case "--help":
            PrintUsage();
            return;
        default:
            Console.Error.WriteLine($"Unknown or incomplete argument: {arg}");
            PrintUsage();
            Environment.ExitCode = 1;
            return;
    }
}

specPath = Path.GetFullPath(specPath);
dataDir = Path.GetFullPath(dataDir);

var spec = AffixSpecLoader.Load(specPath);
GeneratorRunner.Generate(spec, dataDir);

Console.WriteLine("Generation complete:");
Console.WriteLine($"- Spec: {specPath}");
Console.WriteLine($"- Data: {dataDir}");
Console.WriteLine($"- Wrote: {Path.Combine(dataDir, "CalamityAffixes_KID.ini")}");
Console.WriteLine($"- Wrote: {Path.Combine(dataDir, "CalamityAffixes_DISTR.ini")}");
Console.WriteLine($"- Wrote: {Path.Combine(dataDir, spec.ModKey)}");
