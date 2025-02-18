using System.Net.Sockets;
using System.Text;
using Common.Network;

namespace ClientNetwork
{
  public class ClientManager : SocketBase
  {
    public event Action<string> OnConnected;
    public event Action<string> OnDisconnected;
    public event Action<string> OnMessageReceived;

    public void Connect(int port)
    {
      try
      {
        socket = CreateSocket(port);
        Console.WriteLine($"Socket: {socket != null}");
        Console.WriteLine($"EndPoint: {endPoint != null}");
        
        if (socket == null || endPoint == null)
        {
          throw new InvalidOperationException("소켓 생성 실패");
        }

        socket.Connect(endPoint);
        

        if (socket.Connected)
        {
            Console.WriteLine("서버에 연결되었습니다.");
            OnConnected?.Invoke(socket.RemoteEndPoint?.ToString() ?? "알 수 없는 서버");
            Task.Run(() => ReceiveMessages(socket));
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