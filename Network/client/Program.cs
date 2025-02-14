using System;
using System.Net;
using System.Net.Sockets;
using System.Text;

class Program
{
  TcpClient client = null;

  static void Main(string[] args)
  {
    Console.OutputEncoding = Encoding.Unicode;
    Console.InputEncoding = Encoding.Unicode;
    
    Program program = new Program();
    program.Run();
  }
  
    public void Run()
    {
        while (true)
        {
            Console.WriteLine("==========클라이언트==========");
            Console.WriteLine("1.서버연결");
            Console.WriteLine("2.Message 보내기");
            Console.WriteLine("==============================");

            string key = Console.ReadLine();
            int order = 0;

            if (int.TryParse(key, out order))
            {
                switch(order)
                {
                    case 1:
                    {
                        if (client != null)
                        {
                            Console.WriteLine("이미 연결되어있습니다.");
                            Console.ReadKey();
                        }
                        else
                        {
                        	Connect();
                        }

                        break;
                    }
                    case 2:
                    {
                        if (client == null)
                        {
                            Console.WriteLine("먼저 서버와 연결해주세요");
                            Console.ReadKey();
                        }
                        else
                        {
                        	SendMessage();
                        }
                        break;
                    }
                }
            }

            else
            {
                Console.WriteLine("잘못 입력하셨습니다.");
                Console.ReadKey();
            }
            Console.Clear();
        }
    }

    private void SendMessage()
    {
      try 
      {
        if (client != null && client.Connected) 
        {
            Console.WriteLine("보낼 message를 입력해주세요");
            string message = Console.ReadLine();
            byte[] byteData = Encoding.UTF8.GetBytes(message);

            client.GetStream().Write(byteData, 0, byteData.Length);
            Console.WriteLine("전송성공");
        }
        else 
        {
            Console.WriteLine("서버와의 연결이 끊어졌습니다. 다시 연결해주세요.");
        }
      }
      catch (Exception ex)
      {
          Console.WriteLine($"메시지 전송 중 오류 발생: {ex.Message}");
      }
      Console.ReadKey();
    }


    private void Connect()
    {
      try
      {
        client = new TcpClient();
        client.Connect("127.0.0.1", 5000);
        Console.WriteLine("서버연결 성공");
      }
      catch (SocketException ex)
      {
        Console.WriteLine($"서버 연결 실패: {ex.Message}");
      }
      catch (Exception ex)
      {
        Console.WriteLine($"오류 발생: {ex.Message}");
      }
        Console.ReadKey();
    }
}
