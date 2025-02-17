using System.Net;
using System.Net.Sockets;

namespace Common.Network
{
  public class SocketBase
  {
    protected Socket? socket;
    protected IPEndPoint? endPoint;
    protected Socket? CreateSocket(int port)
    {
      try
      {
        // 호스트 이름으로 IP 주소 가져오기
        string host = Dns.GetHostName();
        IPHostEntry ipHost = Dns.GetHostEntry(host);
        IPAddress ipAddr = ipHost.AddressList[0];
        this.endPoint = new(ipAddr, port);
        
        // TCP 소켓 생성
        socket = new(
            addressFamily: ipAddr.AddressFamily,
            socketType: SocketType.Stream,
            protocolType: ProtocolType.Tcp
        );
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