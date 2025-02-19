using System.Net;
using System.Net.Sockets;
using System.Text;


namespace ClientNetwork
{
  public class ClientManager
  {
    // public event Action<string> OnConnected;
    public event Action<string> OnDisconnected;
    public event Action<string> OnMessageReceived;
    protected Socket? socket;
    protected EndPoint? endPoint;
    public void Connect(string chatName, string clientName)
    {
      int port = 8888;
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

        // 소켓 연결 완료시 채팅방, 닉네임 전송
        if (socket.Connected)
        {
          Console.WriteLine("서버에 연결되었습니다.");
          
          // 구분자 사용 -> 하나의 패킷으로 전송
          string clientInfo = $"{chatName}|{clientName}";
          byte[] buffer = Encoding.UTF8.GetBytes(clientInfo);
          socket.SendAsync(new ArraySegment<byte>(buffer), SocketFlags.None);

          // OnConnected?.Invoke(clientName);
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
    public void Disconnect()
    {
      socket?.Shutdown(SocketShutdown.Both);
      socket?.Close();
      socket = null;

      OnDisconnected?.Invoke("exit");
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

        // 연결 해제 시
        if (serverSocket.Connected == false)
        {
          OnDisconnected?.Invoke("exit");
          Disconnect();
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine($"메시지 수신 오류: {ex.Message}");
      }
    }
  }
}