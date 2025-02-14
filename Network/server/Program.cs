using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

public class ChatServer
{
    static async Task Main(string[] args)
    {
      // 콘솔 인코딩을 UTF-8로 설정
      Console.OutputEncoding = Encoding.UTF8;
      Console.InputEncoding = Encoding.UTF8;

      TcpListener server = new TcpListener(IPAddress.Any, 5000);
      server.Start();
      Console.WriteLine("채팅 서버가 시작되었습니다...");

      TcpClient client = server.AcceptTcpClient();
      Console.WriteLine("클라이언트가 연결되었습니다.");
      
      while (true)
      {
        // 소켓은 byte[] 형식으로 데이터를 주고받음.
        byte[] byteData = new byte[1024];
        // 클라이언트에서 메시지 수신
        int bytesRead = client.GetStream().Read(byteData, 0, byteData.Length);
        
        // null 문자 제거
        string message = Encoding.UTF8.GetString(byteData, 0, bytesRead).TrimEnd('\0');
        Console.WriteLine("받은 메시지: " + message);
      }
    }
}