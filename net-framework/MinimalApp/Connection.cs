using GameCarrier.Adapter;
using System.Text;
using System.Security.Cryptography;
using System;

namespace GameCarrier.Examples
{
    public sealed class MinimalAppConn : GcConnectionBase
    {
        readonly Logger log;
        readonly private MinimalApp app;

        public MinimalAppConn(IntPtr id, MinimalApp app) : base(id)
        {
            this.app = app;
            log = new Logger($"Conn #{id}");
            log.Debug("new connection");
        }

        protected override void OnDisconnect()
        {
            log.Notice($"disconnect");
        }

        private bool CheckMessage(byte[] data)
        {
            var len = data.Length;
            var hashLen = 32;
            var prefixLen = 2;
            var reservedLen = hashLen + prefixLen;
            if (len < reservedLen)
            {
                log.Warn($"Invalid incoming message length {len} (too small)");
                return false;
            }

            using (SHA256 sha256 = SHA256.Create())
            {
                byte[] hash = sha256.ComputeHash(data, prefixLen, len - reservedLen);
                for (int i = 0, j = len - hashLen; i < hashLen; ++i, ++j)
                {
                    if (hash[i] != data[j])
                    {
                        log.Warn($"Invalid incoming message: hash mismatch at byte {i}");
                        return false;
                    }
                }
            }

            return true;
        }

        private static byte[] SignText(string text)
        {
            byte[] msg = Encoding.ASCII.GetBytes(text);
            var len = msg.Length + 1; // Include trailing zero
            byte[] result = new byte[len + 32];

            msg.CopyTo(result, 0);
            result[msg.Length] = 0;

            using (SHA256 sha256 = SHA256.Create())
            {
                sha256.ComputeHash(result, 2, len - 2).CopyTo(result, len);
            }
            return result;
        }

        protected override void OnMessage(byte[] data)
        {
            var len = data.Length;
            log.Debug($"Incoming message, {len} bytes.");

            if (!CheckMessage(data))
            {
                return;
            }

            byte[] msg = SignText($"R: app-{app.Id} received {len} bytes.");
            SendMessage(msg);
        }

        protected override void OnMessageSent(long cookie, int len, int status)
        {
            log.Debug($"message sent, status: {status}, len: {len}, cookie: {cookie}.");
        }

        protected override int OnIssue(int id, long value)
        {
            return GC_STAT_REACTION_DEFAULT;
        }
    }
}
