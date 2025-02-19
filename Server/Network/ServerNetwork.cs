using System.Net;
using System.Net.Sockets;
using System.Text;

namespace ServerNetwork
{
  public class ServerManager
  {
    public event Action<string, string> OnConnected; // 연결 알림
    public event Action<string> OnMessageReceived; // 메시지 수신 알림
    public event Action<string, string> OnDisconnected; // 연결 해제 알림
    protected Socket? socket;
    protected EndPoint? endPoint;
    // 채팅방 고유 ID / 채팅방 내 유저 목록
    private readonly Dictionary<int, Dictionary<(string, Socket)>> ConnectedClients = new Dictionary<int, Dictionary<(string, Socket)>>();
    private readonly Dictionary<string, int> ChatRooms = new Dictionary<string, int>();
    private readonly object LockObject = new object();
    public void StartServer() {
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
    public void StopServer()
    {
      socket?.Shutdown(SocketShutdown.Both);
      socket?.Close();
      socket = null;
    }
    private async Task BroadCast(int chatRoomId, string message)
    {
      byte[] buffer = Encoding.UTF8.GetBytes(message);
      lock(LockObject)
      {
        // 닉네임, 소켓 목록
        List<(string, Socket)> clients = ConnectedClients[chatRoomId];
        foreach (var client in clients)
        {
          try
          {
            client.Item2.SendAsync(new ArraySegment<byte>(buffer, 0, buffer.Length), SocketFlags.None); // 클라이언트에게 비동기 메시지 전송
          }
          catch (Exception ex)
          {
            Console.WriteLine($"메시지 전송 오류: {ex.Message}");
          }
        }
      }
    }
    private async Task ConnectClients(Socket serverSocket)
    {
      string chatName = "";
      string clientName = "";
      int chatRoomId = 0;

      while(true)
      {
        try 
        {
          Socket clientSocket = await serverSocket.AcceptAsync();
          if (clientSocket.Connected)
          {
            // 유저 정보 - 닉네임
            byte[] buffer = new byte[1024];
            int received = await Task.Run(() => clientSocket.Receive(buffer));
            string clientInfo = Encoding.UTF8.GetString(buffer, 0, received).TrimEnd('\0');

            // 채팅방 고유 ID / 채팅방 내 유저 목록
            string[] info = clientInfo.Split('|');
            chatName = info[0];
            clientName = info[1];

            // 동시성 제어
            lock(LockObject)
            {
              // 존재하지 않는 채팅방 -> 새로운 채팅방 생성
              if (!ChatRooms.ContainsKey(chatName))
              {
                int temp = ChatRooms.Count + 1;
                ChatRooms[chatName] = temp;
                ConnectedClients[temp] = new List<(string, Socket)>();
              }

              chatRoomId = ChatRooms[chatName];
              ConnectedClients[chatRoomId].Add((clientName, clientSocket));
            }
            
            // UI 업데이트
            if (Application.OpenForms.Count > 0)
            {
              Application.OpenForms[0].BeginInvoke(new Action(() => {
                OnConnected?.Invoke(chatName, clientName);
              }));
            }
            
            // 입장 알림
            Console.WriteLine($"{clientName}님이 {chatName}방에 입장하셨습니다.");
            OnMessageReceived?.Invoke($"{clientName}님이 {chatName}방에 입장하셨습니다.");
            await BroadCast(chatRoomId, $"{clientName}님이 {chatName}방에 입장하셨습니다.");
            
            // 메시지 수신 시작
            _ = Task.Run(() => ReceiveMessages(clientSocket, chatRoomId, clientName));
          }

          // 연결 해제 시
          if (clientSocket.Connected == false)
          {
            lock(LockObject)
            {
              ConnectedClients[chatRoomId].Remove((clientName, clientSocket));
              OnDisconnected?.Invoke(chatName, clientName);
            }
          }
        }
        catch (Exception ex)
        {
          Console.WriteLine($"클라이언트 연결 오류: {ex.Message}");
        }
      }
    }
    private async Task ReceiveMessages(Socket clientSocket, int chatRoomId, string clientName)
    {
      while(true)
      {
        byte[] buffer = new byte[1024];
        int received = await Task.Run(() => clientSocket.Receive(buffer)); // await Task.Run -> 비동기 처리

        if (received == 0) break; // 연결 종료

        string message = Encoding.UTF8.GetString(buffer, 0, received).TrimEnd('\0'); // 공백 제거
        Console.WriteLine($"[{clientName}] 수신: {message}");

        OnMessageReceived?.Invoke($"{clientName} : {message}");
        await BroadCast(chatRoomId, $"{clientName} : {message}");
      }
    }
  }
}