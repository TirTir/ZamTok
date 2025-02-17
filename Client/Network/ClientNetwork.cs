using System.Net.Sockets;
using System.Text;
using Common.Network;

namespace ClientNetwork
{
  public class ClientManager : SocketBase
  {
    public event Action<string> OnConnected;
    public event Action<string> OnMessageReceived;

    public void Connect(int port)
    {
      try
      {
        socket = CreateSocket(port);
        if (socket == null) 
        {
            Console.WriteLine("소켓 생성 실패");
            return;
        }

        socket.Connect(endPoint);
        Console.WriteLine("서버에 연결되었습니다.");

        OnConnected?.Invoke(socket.RemoteEndPoint?.ToString() ?? "알 수 없는 서버");
        Task.Run(() => ReceiveMessages(socket));
      }
      catch (Exception ex)
      {
        Console.WriteLine($"서버 연결 오류: {ex.Message}");
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
      while(true)
      {
        try
        {
          byte[] buffer = new byte[1024];
          int received = await Task.Run(() => socket!.Receive(buffer));
          if (received == 0) break;
          
          string message = Encoding.UTF8.GetString(buffer, 0, received);
          OnMessageReceived?.Invoke(message);
        }
        catch (Exception ex)
        {
          Console.WriteLine($"메시지 수신 오류: {ex.Message}");
        }
        finally
        {
          try
          {
            serverSocket.Shutdown(SocketShutdown.Both);
            serverSocket.Close();
          }
          catch { }
        }
      }
    }
  }
}