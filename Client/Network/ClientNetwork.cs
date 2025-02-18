using System.Net;
using System.Net.Sockets;
using System.Text;


namespace ClientNetwork
{
  public class ClientManager
  {
    public event Action<string> OnConnected;
    public event Action<string> OnDisconnected;
    public event Action<string> OnMessageReceived;
    protected Socket? socket;
    protected EndPoint? endPoint;
    public void Connect(int port, string clientName)
    {
      string host = Dns.GetHostName();
      IPHostEntry ipHost = Dns.GetHostEntry(host);
      IPAddress ipAddr = IPAddress.Parse("127.0.0.1");

      try
      {
        // TCP 소켓 생성
        socket = new(
          addressFamily: AddressFamily.InterNetwork,
          socketType: SocketType.Stream,
          protocolType: ProtocolType.Tcp
        );

        // 엔드포인트 설정
        endPoint = new IPEndPoint(ipAddr, port);
        
        if (socket == null || endPoint == null)
        {
          throw new InvalidOperationException("소켓 생성 실패");
        }

        socket.Connect(endPoint);

        if (socket.Connected)
        {
          byte[] nameBuffer = Encoding.UTF8.GetBytes(clientName);
          socket.SendAsync(new ArraySegment<byte>(nameBuffer), SocketFlags.None);

          Console.WriteLine("서버에 연결되었습니다.");
          OnConnected?.Invoke(clientName);
          Task.Run(() => ReceiveMessages(socket)); // 메시지 수신
        }
        else
        {
            throw new SocketException();
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine($"서버 연결 오류: {ex.Message}");
        OnDisconnected?.Invoke("error");
      }
    }
    public async Task SendMessage(string message)
    {
      try
      {
        byte[] byteData = Encoding.UTF8.GetBytes(message);
        await socket!.SendAsync(new ArraySegment<byte>(byteData), SocketFlags.None);
      }
      catch (Exception ex)
      {
        Console.WriteLine($"메시지 전송 오류: {ex.Message}");
      }
    }

    private async Task ReceiveMessages(Socket serverSocket)
    {
      try
      {
        while(true)
        {
          byte[] buffer = new byte[1024];
          int received = await socket!.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None);

          if (received == 0) 
          {
            OnDisconnected?.Invoke("error");
            break;
          }

          string message = Encoding.UTF8.GetString(buffer, 0, received);
          OnMessageReceived?.Invoke(message);
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine($"메시지 수신 오류: {ex.Message}");
      }
    }
  }
}