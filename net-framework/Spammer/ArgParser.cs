using GameCarrier.Clients;
using GameCarrier.Common;
using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace GameCarrier.Examples
{
    internal class SpammerArgParser
    {
        public bool OptHelp;
        public bool OptVerbose;
        public int BaseMsgSizeInKb;
        public int RelaxTime;
        public int SleepTime;

        public Queue<GcClientMode> Modes { get; }
        public Queue<ConnCredentials> Uris { get; }
        public Queue<int> UriCounts { get; }

        private Queue<string> argsTail;

        public SpammerArgParser()
        {
            OptHelp = false;
            OptVerbose = false;
            BaseMsgSizeInKb = 128;
            RelaxTime = 3000;
            Modes = new Queue<GcClientMode>();
            Uris = new Queue<ConnCredentials>();
            UriCounts = new Queue<int>();
        }

        private static void SetInt(string name, string value, ref int variable)
        {
            if (!int.TryParse(value, out variable))
            {
                throw new BadArguments($"Non integer value for {name}.");
            }

            if (variable <= 0)
            {
                throw new BadArguments($"Value for {name} might be strictly positive.");
            }
        }

        static private ConnectionProtocol LookupProto(string protoStr)
        {
            if (protoStr == "")
            {
                return ConnectionProtocol.WebSocketSecure;
            }

            if (protoStr == "wss://")
            {
                return ConnectionProtocol.WebSocketSecure;
            }

            if (protoStr == "ws://")
            {
                return ConnectionProtocol.WebSocket;
            }

            throw new BadArguments($"Invalid proto {protoStr}");
        }

        private void ParseUri(string uri)
        {
            string pattern = @"^(wss?://)?([^:/]+)(:([0-9]+))?/?(.*)$";
            var m = Regex.Match(uri, pattern);
            if (!m.Success)
            {
                throw new BadArguments($"Invalid URI: {uri}");
            }

            var protoStr = m.Groups[1].Value;
            var host = m.Groups[2].Value;
            var portStr = m.Groups[4].Value;
            var app = m.Groups[5].Value;

            var proto = LookupProto(protoStr);
            var port = portStr != "" ? int.Parse(portStr) : 7681;
            Uris.Enqueue(new ConnCredentials(proto, host, port, app));
        }

        private void SetUriCount(int count)
        {
            if (UriCounts.Count >= Uris.Count)
            {
                throw new BadArguments("Unexpacted URI count.");
            }

            while (UriCounts.Count < Uris.Count - 1)
            {
                UriCounts.Enqueue(1);
            }

            UriCounts.Enqueue(count);
        }

        bool TryIntArg(string arg, string name, ref int variable)
        {
            if (name == null)
            {
                return false;
            }

            if (!arg.StartsWith(name))
            {
                return false;
            }

            if (arg.Length == name.Length)
            {
                if (argsTail.Count == 0)
                {
                    throw new BadArguments($"No value for {name} argument.");
                }

                SetInt(name, argsTail.Dequeue(), ref variable);
                return true;
            }

            if (arg[name.Length] != '=')
            {
                throw new BadArguments($"Invalid option {arg}.");
            }

            int eqPos = name.Length + 1;
            SetInt(name, arg.Substring(eqPos), ref variable);
            return true;
        }

        void ReadLongOption(string arg)
        {
            switch (arg)
            {
                case "--help":
                    OptHelp = true;
                    return;
                case "--verbose":
                    OptVerbose = true;
                    return;
                case "--passive":
                    Modes.Enqueue(GcClientMode.Passive);
                    return;
                case "--active":
                    Modes.Enqueue(GcClientMode.Active);
                    return;
                case "--hybrid":
                    Modes.Enqueue(GcClientMode.Hybrid);
                    return;
            }

            bool intArg = false
                || TryIntArg(arg, "--kbytes", ref BaseMsgSizeInKb)
                || TryIntArg(arg, "--sleep", ref SleepTime)
                || TryIntArg(arg, "--relax", ref RelaxTime)
                ;
            if (intArg)
            {
                return;
            }

            throw new BadArguments($"Invalid long option {arg}");
        }

        void ReadShortValue(string name, string arg, ref int variable)
        {
            var value = arg != "" ? arg : argsTail.Dequeue();
            SetInt(name, value, ref variable);
        }

        void ReadShortOption(string arg)
        {
            for (int i = 1; i < arg.Length; ++i)
            {
                char ch = arg[i];
                switch (ch)
                {
                    case 'v':
                        OptVerbose = true;
                        break;
                    case 'p':
                        Modes.Enqueue(GcClientMode.Passive);
                        break;
                    case 'a':
                        Modes.Enqueue(GcClientMode.Active);
                        break;
                    case 'h':
                        Modes.Enqueue(GcClientMode.Hybrid);
                        break;
                    case 'k':
                        ReadShortValue($"-{ch}", arg.Substring(i + 1), ref BaseMsgSizeInKb);
                        return;
                    case 'r':
                        ReadShortValue($"-{ch}", arg.Substring(i + 1), ref RelaxTime);
                        return;
                    case 's':
                        ReadShortValue($"-{ch}", arg.Substring(i + 1), ref SleepTime);
                        return;
                    default:
                        throw new BadArguments($"Invalid short option {arg[i]}");
                }
            }
        }

        void ParseArgument(string arg)
        {
            if (arg.StartsWith("--"))
            {
                ReadLongOption(arg);
                return;
            }

            if (arg.StartsWith("-"))
            {
                ReadShortOption(arg);
                return;
            }

            if (int.TryParse(arg, out int count))
            {
                SetUriCount(count);
            }
            else
            {
                ParseUri(arg);
            }
        }

        public static void Usage()
        {
            Console.WriteLine("Usage: Spammer.exe [options] [uri1 [xN1] [uri2 [xN2] ... ]]");
            Console.WriteLine();
            Console.WriteLine("Options:");
            Console.WriteLine("  -v, --verbose          Verbose level of logging.");
            Console.WriteLine(" --help                  Print help message and exit.");
            Console.WriteLine(" -k, --kbytes  <int>     Base message len in kb.");
            Console.WriteLine(" -s, --sleep   <int>     Sleep time in ms between gc_service().");
            Console.WriteLine(" -r, --relax   <int>     Relax time in ms.");
            Console.WriteLine(" -p, --passive           Add clients run in a passive mode.");
            Console.WriteLine(" -a, --active            Add clients run in an active mode.");
            Console.WriteLine(" -h, --hybrid            Add clients run in a hybrid mode.");
        }

        public int? Parse(string[] args)
        {
            try
            {
                argsTail = new Queue<string>(args);
                while (argsTail.Count > 0)
                {
                    var arg = argsTail.Dequeue();
                    ParseArgument(arg);
                }

                if (Modes.Count == 0)
                {
                    throw new BadArguments("Mode is not set.");
                }

                while (UriCounts.Count < Uris.Count)
                {
                    UriCounts.Enqueue(1);
                }

                if (OptHelp)
                {
                    Usage();
                    return 0;
                }

                return null;
            }
            catch (BadArguments ex)
            {
                Console.WriteLine(ex.Message);
                Console.WriteLine();
                Usage();
                return 1;
            }
        }

        public void Dump()
        {
            Console.WriteLine($"Help      : {OptHelp}");
            Console.WriteLine($"Verbose   : {OptVerbose}");
            Console.WriteLine($"BaseMsgSz : {BaseMsgSizeInKb}");
            Console.WriteLine($"RelaxTime : {RelaxTime}");
            Console.WriteLine($"SleepTime : {SleepTime}");

            Console.WriteLine("Modes:");
            foreach (var mode in Modes)
            {
                Console.WriteLine($"  {mode}");
            }

            Console.WriteLine("URIs:");
            Queue<int> uriCounts = new Queue<int>(UriCounts);
            foreach (var uri in Uris)
            {
                var count = uriCounts.Dequeue();
                Console.WriteLine($"  count: {count}, proto: {uri.Proto}, host: {uri.Host}, port: {uri.Port}, app: {uri.App}");
            }
        }
    }
}
