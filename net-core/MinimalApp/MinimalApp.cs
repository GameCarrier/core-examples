using GameCarrier.Adapter;

namespace GameCarrier.Examples
{
    public sealed class MinimalApp : GcApplicationBase
    {
        readonly Logger log = new("MinimalApp");

        public override GcConnectionBase OnConnect(IntPtr connenctionHandle, ConnectionInfo info)
        {
            return new MinimalAppConn(connenctionHandle, this);
        }

        public override void OnShutdown()
        {
            log.Notice("OnShutdown");
        }

        public override void OnStart()
        {
            log.Notice("OnStart");
        }

        public override void OnStop()
        {
            log.Notice("OnStop");
        }
    }
}
