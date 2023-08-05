using System.Reflection;
using System.Runtime.Versioning;
using System.Collections.Generic;
using System.Linq;
using System;
using System.Diagnostics;
using System.IO;

namespace ConfigPatcher
{
    internal class Program
    {
        private static readonly Dictionary<string, Func<string>> paramGetters = new Dictionary<string, Func<string>>()
        {
            { "gc_build_type", ReadGcBuildType },
            { "gc_target_framework", ReadGcTargetFramework },
            { "gc_app_dir", ReadGcAppDir },
            { "gc_git_root", ReadGcGitRoot },
        };

        private static readonly Dictionary<string, string> paramCache = new Dictionary<string, string>() { };

        private static void PrintHelp()
        {
            Console.WriteLine("Usage: ConfigPatcher.exe InFile OutFile");
        }

        private static string ReadAttribute<T>(string propName)
        {
            var assembly = Assembly.GetExecutingAssembly();
            var attributes = assembly.GetCustomAttributes(typeof(T), false);
            object obj = attributes.SingleOrDefault();
            var info = obj?.GetType()?.GetProperty(propName);
            object value = info?.GetValue(obj);
            string str = value?.ToString();
            return str ?? "";
        }

        private static string ReadGcBuildType()
        {
            var exeLocation = Assembly.GetExecutingAssembly().Location;
            string[] pathItems = exeLocation.Split(Path.DirectorySeparatorChar);
            int index = pathItems.Length - 3;
            if (index < 0)
            {
                return "Release";
            }
            return pathItems[index];
        }

        private static string ReadGcTargetFramework()
        {
            var frameworkDisplayName = ReadAttribute<TargetFrameworkAttribute>("FrameworkDisplayName");
            return frameworkDisplayName.Replace(".NET ", "net");
        }

        static string ReadGcGitRoot()
        {
            var processStartInfo = new ProcessStartInfo
            {
                FileName = "git",
                Arguments = "rev-parse --show-toplevel",
                RedirectStandardOutput = true,
                UseShellExecute = false
            };
            var process = Process.Start(processStartInfo);
            if (process == null)
            {
                Console.Error.WriteLine("Cannot Start Git");
                return "";
            }
            var output = process.StandardOutput.ReadToEnd();
            process.WaitForExit();
            return output.Trim();
        }

        static string ReadGcAppDir()
        {
            return ReadValue("gc_git_root") + "/build/bin/apps";
        }

        static string ReadValue(string key)
        {
            if (paramCache.ContainsKey(key))
            {
                return paramCache[key];
            }

            if (!paramGetters.ContainsKey(key))
            {
                Console.Error.WriteLine($"Invalid key {key}");
                return "";
            }

            var getter = paramGetters[key];
            var value = getter();
            paramCache[key] = value;

            if (value == "")
            {
                Console.Error.WriteLine($"No value for key {key}");
            }
            return value;
        }

        private static string ProcessLine(string line)
        {
            string outline = line;
            foreach (var pair in paramGetters)
            {
                var key = pair.Key;
                var macro = "@" + key + "@";

                while (outline.Contains(macro))
                {
                    var value = ReadValue(key);
                    outline = outline.Replace(macro, value);
                }
            }

            return outline;
        }

        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                PrintHelp();
                return;
            }

            if (args.Length != 2)
            {
                Console.Error.WriteLine("Wrong argument count");
                PrintHelp();
                Environment.Exit(1);
            }

            string inFile = args[0];
            string outFile = args[1];

            string outDir = Path.GetDirectoryName(outFile);
            if (outDir != null && !Directory.Exists(outDir))
            {
                Directory.CreateDirectory(outDir);
            }

            using (var outf = new StreamWriter(outFile))
            {
                foreach (string line in File.ReadLines(inFile))
                {
                    string outline = ProcessLine(line);
                    outf.WriteLine(outline);
                }
            }
        }
    }
}
