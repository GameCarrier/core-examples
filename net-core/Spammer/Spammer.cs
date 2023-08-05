using GameCarrier.Clients;
using GameCarrier.Common;
using System.Text;
using System.Security.Cryptography;
using System.Runtime.CompilerServices;
using System.ComponentModel;

namespace GameCarrier.Examples
{
    internal class Spammer
    {
        public static SpammerArgParser ArgParser = new();
        public static Random Rnd = new();

        private static readonly Logger log = new("Spammer");
        private static readonly List<SpammerClient> clients = new();

        private class SpammerClient : GcClient
        {
            private static readonly Logger log = new("Client");
            public int PacketsLeft { get; set; }
            public bool Stopped { get; set; }

            public SpammerClient(ConnCredentials uri, int packetsLeft) :
                base(uri.Proto, uri.Host, uri.Port, uri.App)
            {
                PacketsLeft = packetsLeft;
                Stopped = false;
            }

            public static byte[] MakeMessage(int sz)
            {
                byte[] msg = new byte[sz];
                msg[0] = (byte)'M';
                msg[1] = (byte)':';
                msg[2] = (byte)' ';
                for (int i=3; i<sz - 32; ++i)
                {
                    msg[i] = (byte)(32 + Spammer.Rnd.Next(0, 95));
                }

                using SHA256 sha256 = SHA256.Create();
                int offset = sz - 32;
                sha256.ComputeHash(msg, 2, offset - 2).CopyTo(msg, offset);
                return msg;
            }

            public void DoAction()
            {
                log.Notice($"DoAction {PacketsLeft}");
                if (PacketsLeft-- <= 0)
                {
                    Disconnect();
                    return;
                }

                int sz = 128 + 1024 * Spammer.ArgParser.BaseMsgSizeInKb + Spammer.Rnd.Next(0, 1024);
                byte[] msg = MakeMessage(sz);
                Send(msg);
                log.Info($"Send message with size {sz}.");
            }

            public void MessageEvent(Msg msg)
            {
                log.Info($"Client-{Index} OnMessage received {msg.Length} bytes.");
                var prefixLen = 2;
                var hashLen = 32;
                var len = msg.Length;
                var payloadLen = len - hashLen;

                var minimalLen = hashLen + prefixLen;
                if (len < minimalLen)
                {
                    log.Warn($"Invalid incoming message length {len} (too small)");
                    Disconnect();
                    return;
                }

                using SHA256 sha256 = SHA256.Create();
                byte[] calculatedHash = sha256.ComputeHash(msg.SubStream(prefixLen, payloadLen - prefixLen));
                byte[] receivedHash = msg.CopyTail(hashLen);
                for (int i = 0; i < hashLen; ++i)
                {
                    if (calculatedHash[i] != receivedHash[i])
                    {
                        log.Warn($"Invalid incoming message: hash mismatch at byte {i}");
                        Disconnect();
                        return;
                    }
                }

                log.User(msg.CopyString(0, payloadLen));
                DoAction();
            }

            public void MessageSentEvent(int sz)
            {
                log.Info($"Client-{Index} OnMessageSent {sz} bytes.");
            }

            public void ConnectEvent()
            {
                log.Info($"Client-{Index} OnConnect");
                DoAction();
            }

            public void ConnectionErrorEvent()
            {
                log.Info($"Client-{Index} OnConnectionError");
                Stopped = true;
            }

            public void DisconnectEvent()
            {
                log.Info($"Client-{Index} OnDisconnect");
                Stopped = true;
            }
        }

        private static void Start(ConnCredentials uri, int count)
        {
            log.Notice($"Try to send {count} message(s) via proto {uri.Proto} to host {uri.Host} and port: {uri.Port}, the app: {uri.App}");
            var client = new SpammerClient(uri, count);
            client.OnConnectEvent += (sender, e) => (sender as SpammerClient)?.ConnectEvent();
            client.OnConnectionErrorEvent += (sender, e) => (sender as SpammerClient)?.ConnectionErrorEvent();
            client.OnDisconnectEvent += (sender, e) => (sender as SpammerClient)?.DisconnectEvent();
            client.OnMessageEvent += (sender, e) => (sender as SpammerClient)?.MessageEvent(e.Msg);
            client.OnMessageSentEvent += (sender, e) => (sender as SpammerClient)?.MessageSentEvent(e.Length);
            clients.Add(client);
            client.Connect();
        }

        private static bool JobFinished()
        {
            foreach (var client in clients)
            {
                if (!client.Stopped)
                {
                    return false;
                }
            }

            return true;
            throw new NotImplementedException();
        }

        private static long MillisecondsNow()
        {
            return DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond;
        }

        private static void Relax(GcClientMode mode, int maxTime, int sleepTime)
        {
            var start = MillisecondsNow();
            for (;;)
            {
                if (JobFinished())
                {
                    return;
                }

                var current = MillisecondsNow();
                if (current > start + maxTime)
                {
                    throw new BadFlow($"Timeout {maxTime} ms in relaxing");

                }

                if (mode != GcClientMode.Active)
                {
                    int status = Api.ClientService();
                    if (status < 0)
                    {
                        throw new BadFlow($"Bad status returned from Api.ClientService: {status}");
                    }
                }

                if (sleepTime > 0)
                {
                    /* Emulate hard job here */
                    Console.WriteLine($" ... sleep {sleepTime}");
                    Thread.Sleep(sleepTime);
                }
            }
        }

        static void RunMode(GcClientMode mode)
        {
            try
            {
                log.Notice($"Run in mode {mode}");
                clients.Clear();
                Queue<int> uriCounts = new(ArgParser.UriCounts);
                foreach (var uri in ArgParser.Uris)
                {
                    var count = uriCounts.Dequeue();
                    Start(uri, count);
                }

                Relax(mode, ArgParser.RelaxTime, ArgParser.SleepTime);
            }
            catch (Exception e)
            {
                log.Error(e.Message);
                throw;
            }
        }

        static int Main()
        {
            try
            {
                int? exitCode = ArgParser.Parse();
                if (exitCode != null)
                {
                    return (int)exitCode;
                }

                Logger.SetLogOptions(LogLevel.LLL_VERBOSE, "spammer.%p.log", LogFlags.LOG_ALL);
                foreach (var mode in ArgParser.Modes)
                {
                    GcClient.Manager.Init(mode);
                    try
                    {
                        RunMode(mode);
                    }
                    finally
                    {
                        GcClient.Manager.Cleanup();
                    }
                }

                return 0;
            }

            catch (Exception ex)
            {
                Console.Error.WriteLine(ex.Message);
                return 1;
            }
        }
    }
}
