using System;
using System.Windows.Forms;
using System.Drawing;
using ClientNetwork;

namespace Client.Forms
{
  public partial class ClientForm : Form
  {
    private ClientManager manager;
    private ListBox messages;
    private TextBox messageTxt;
    private TextBox portTxt;
    private TextBox logTxt;
    private Button btnConnect;
    private Button btnSend;
    public ClientForm()
    {
      InitializeComponent();
      this.Text = "채팅 클라이언트";
      this.Size = new Size(600, 600);

      manager = new ClientManager();
      manager.OnConnected += HandleServerConnected;
      manager.OnDisconnected += HandleDisConnected;
      manager.OnMessageReceived += HandleMessageReceived;

      InitializeUI();
    }

    private void InitializeUI()
    {
      // 포트 입력 패널
      var portPanel = new Panel
      {
        Text = "연결하기",
        Dock = DockStyle.Top,
        Height = 50
      };

      portTxt = new TextBox
      {
        Text = "8888",
        Location = new Point(10, 15),
        Width = 140,
      };

      btnConnect = new Button
      {
        Location = new Point(170, 15),
        Width = 100,
        Height = 30,
        Text = "Connect"
      };
      btnConnect.Click += btnConnectServer;

      portPanel.Controls.Add(portTxt);
      portPanel.Controls.Add(btnConnect);

      // 로그 & 메시지 영역
      var mainPanel = new Panel
      {
        Dock = DockStyle.Fill
      };

      messages = new ListBox
      {
        Dock = DockStyle.Fill,
        Font = new Font("맑은 고딕", 10)
      };

      mainPanel.Controls.Add(messages);

      var bottomPanel = new Panel
      {
        Dock = DockStyle.Bottom,
        Height = 50
      };

      btnSend = new Button
      {
        Location = new Point(450, 10),
        Width = 100,
        Height = 30,
        Text = "Send"
      };
      btnSend.Click += btnSendMessage;

      messageTxt = new TextBox
      {
        Location = new Point(10, 10),
        Height = 30,
        Width = 400,
        Multiline = true,
        Font = new Font("맑은 고딕", 10),
      };
      bottomPanel.Controls.Add(messageTxt);
      bottomPanel.Controls.Add(btnSend);

      Controls.Add(bottomPanel);
      Controls.Add(mainPanel);
      Controls.Add(portPanel);
    }

    private void btnConnectServer(object sender, EventArgs e)
    {
      if (!int.TryParse(portTxt.Text, out int port))
      {
        LogMessage("올바른 포트 번호를 입력하세요.");
        return;
      }

      try{
        btnConnect.Enabled = false;
        portTxt.Enabled = false;
      
        LogMessage($"서버 {port}에 연결 시도...");
        manager.Connect(port);
      }
      catch(Exception ex)
      {
        LogMessage($"연결 오류: {ex.Message}");
      }
    }

    private async void btnSendMessage(object sender, EventArgs e)
    {
      string message = messageTxt.Text.Trim();
      if(!string.IsNullOrEmpty(message))
      {
        await manager.SendMessage(message);
        LogMessage($"전송: {message}");
        messageTxt.Clear();
      }
    }

    private void HandleServerConnected(string serverInfo)
    {
      LogMessage($"서버에 연결되었습니다.");
    }

    private void HandleDisConnected(String type)
    {
      if (this.InvokeRequired)
      {
        this.Invoke(new Action<string>(HandleDisConnected), type);
        return;
      }

      try
      {
        btnConnect.Enabled = true;
        portTxt.Enabled = true;

        if(type == "error")
        {
          LogMessage("서버 연결 실패");
        }
        else if(type == "exit")
        {
          LogMessage("서버 종료");
        }
      }
      catch(Exception ex)
      {
        LogMessage($"오류: {ex.Message}");
      }
    }

    private void HandleMessageReceived(string message)
    {
      if (this.InvokeRequired)
      {
        this.Invoke(new Action<string>(HandleMessageReceived), message);
        return;
      }

      try
      {
        LogMessage($"수신: {message}");
      }
      catch(Exception ex)
      {
        LogMessage($"오류: {ex.Message}");
      }
    }


    private void LogMessage(string message)
    {
      string date = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");
      Console.WriteLine($"LogMessage: {message}");

      if (this.InvokeRequired)
      {
        this.Invoke(new Action<string>(LogMessage), message);
        return;
      }

      try
      {
        messages.Items.Add("[" + date + "] " + message);
      }
      catch(Exception ex)
      {
        LogMessage($"오류: {ex.Message}");
      }
    }
  }
}