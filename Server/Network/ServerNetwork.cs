using System.Net;
using System.Net.Sockets;
using System.Text;
using Common.Network;

namespace ServerNetwork
{
  public class ServerManager : SocketBase
  {
    public event Action<string> OnConnected; // 연결 알림
    public event Action<string> OnMessageReceived; // 메시지 수신 알림
    public void StartServer(int port) {
      try
      {
        socket = CreateSocket(port);
        if (socket == null)
        {
          Console.WriteLine("소켓 생성 실패");
          return;
        }

        socket.Bind(endPoint!);
        socket.Listen(10);      
        Console.WriteLine($"서버가 {port} 포트에서 시작되었습니다.");

        Task.Run(() => ConnectClients(socket));
      }
      catch (Exception ex)
      {
        Console.WriteLine($"서버 시작 오류: {ex.Message}");
      }
    }

    private async Task ConnectClients(Socket serverSocket)
    {
      while(true)
      {
        try 
        {
          Socket clientSocket = await Task.Run(() => socket!.Accept());
          OnConnected?.Invoke(clientSocket.RemoteEndPoint?.ToString() ?? "알 수 없는 클라이언트");
          
          Task.Run(() => SendMessage(clientSocket));
        }
        catch (Exception ex)
        {
          Console.WriteLine($"클라이언트 연결 오류: {ex.Message}");
        }
      }
    }

    private async Task SendMessage(Socket clientSocket)
    {
      try
      {
        while(true)
        {
          byte[] buffer = new byte[1024];
          int received = await Task.Run(() => clientSocket.Receive(buffer)); // await Task.Run -> 비동기 처리
          
          if (received == 0) break; // 연결 종료

          string message = Encoding.UTF8.GetString(buffer, 0, received).TrimEnd('\0'); // 공백 제거
          string date = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"); // 현재 시간
          Console.WriteLine($"수신: {message}");

          OnMessageReceived?.Invoke(message);

          // 에코
          await socket!.SendAsync(new ArraySegment<byte>(buffer, 0, received), SocketFlags.None); // 클라이언트에게 비동기 메시지 전송
        }
      }
      catch (Exception ex)
      {
        Console.WriteLine($"클라이언트 통신 오류: {ex.Message}");
      }
      finally
      {
        try
        {
          clientSocket.Shutdown(SocketShutdown.Both);
          clientSocket.Close();
        }
        catch { }
      }
    }
  }
}