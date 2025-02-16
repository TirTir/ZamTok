namespace ServerNetwork
{
  public class ServerManager
  {
    private Socket socket;
    public event Action<string> OnConnected;

    public void StartServer(int port) {
      try
      {
        string host = Dns.GetHostName();
        IPHostEntry ipHost = Dns.GetHostEntry(host);
        IPAddress ipAddr = ipHost.AddressList[0];
        IPEndPoint endPoint = new(address: ipAddr, port: port);

        // TCP 소켓 생성
        socket = new(
          addressFamily: ipAddr.AddressFamily,
          socketType: SocketType.Stream,
          protocolType: ProtocolType.Tcp);

        // 소켓 바인딩
        socket.Bind(endPoint);
        socket.Listen(backlog: 10);      
        Console.WriteLine($"서버가 {port} 포트에서 시작되었습니다.");

        Task.Run(AcceptClients);
      }
      catch (Exception ex)
      {
        Console.WriteLine($"서버 시작 오류: {ex.Message}");
      }
    }

    private async Task ConnectClients()
    {
      while(true)
      {
        try 
        {
          // 비동기 처리
          Socket clientSocket = await Task.Run(() => socket.Accept());
          // 
          OnConnected?.Invoke(clientSocket.RemoteEndPoint?.ToString() ?? "알 수 없는 클라이언트");
          
          // 각 클라이언트마다 별도의 태스크에서 처리
          _ = Task.Run(() => HandleClientCommunication(clientSocket));
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
          int received = await Task.Run(() => clientSocket.Receive(buffer));
          
          if (received == 0) break; // 연결 종료

          string message = Encoding.UTF8.GetString(buffer, 0, received).TrimEnd('\0');
          Console.WriteLine($"수신: {message}");

          // 에코
          await Task.Run(() => clientSocket.Send(buffer, 0, received, SocketFlags.None));
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