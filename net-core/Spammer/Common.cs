using GameCarrier.Common;
using System.Runtime.InteropServices;

namespace GameCarrier.Examples
{
    public class BadArguments : Exception
    {
        public BadArguments(string message) : base(message)
        {
        }
    }

    public class BadFlow : Exception
    {
        public BadFlow(string message) : base(message)
        {
        }
    }

    public class ConnCredentials
    {
        public ConnCredentials(ConnectionProtocol proto, string host, int port, string app)
        {
            Host = host;
            Proto = proto;
            Port = port;
            App = app;
        }

        public ConnectionProtocol Proto { get; }
        public string Host { get; }
        public int Port { get; }
        public string App { get; }
    }
}
