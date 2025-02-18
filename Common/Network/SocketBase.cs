using System.Net;
using System.Net.Sockets;

namespace Common.Network
{
  public class SocketBase
  {
    protected Socket? socket;
    protected EndPoint? endPoint;
    protected Socket? CreateSocket(int port)
    {

      try
      {
        string host = Dns.GetHostName();
        IPHostEntry ipHost = Dns.GetHostEntry(host);
        IPAddress ipAddr = IPAddress.Parse("127.0.0.1");
        
        // TCP 소켓 생성
        socket = new(
            addressFamily: AddressFamily.InterNetwork,
            socketType: SocketType.Stream,
            protocolType: ProtocolType.Tcp
        );

        // 엔드포인트 설정
        endPoint = new IPEndPoint(ipAddr, port);

        return socket;
      }
      catch (Exception ex)
      {
          Console.WriteLine($"소켓 생성 오류: {ex.Message}");
          return null;
      }
    }
  }
}