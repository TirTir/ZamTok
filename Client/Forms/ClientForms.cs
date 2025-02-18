using System;
using System.Windows.Forms;
using System.Drawing;
using ClientNetwork;

namespace Client.Forms
{
  public partial class ClientForm : Form
  {
    private ClientManager manager;
    private ListBox messages; // 채팅 목록
    private TextBox messageTxt; // 입력 필드
    private TextBox portTxt;
    private TextBox clientNameTxt;
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
      // 서버 접속 입력 패널
      var serverPanel = new Panel
      {
        Dock = DockStyle.Top,
        Height = 50
      };

      portTxt = new TextBox
      {
        Text = "8888",
        Location = new Point(10, 15),
        Width = 130,
      };

      clientNameTxt = new TextBox
      {
        Location = new Point(150, 15),
        Width = 130,
      };

      btnConnect = new Button
      {
        Location = new Point(290, 15),
        Width = 100,
        Height = 30,
        Text = "Connect"
      };
      btnConnect.Click += btnConnectServer;

      serverPanel.Controls.Add(portTxt);
      serverPanel.Controls.Add(clientNameTxt);
      serverPanel.Controls.Add(btnConnect);

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
      Controls.Add(serverPanel);
    }

    private void btnConnectServer(object sender, EventArgs e)
    {
      if (!int.TryParse(portTxt.Text, out int port))
      {
        LogMessage("올바른 포트 번호를 입력하세요.");
        return;
      }

      if(clientNameTxt.Text == null)
      {
        LogMessage("닉네임을 입력하세요.");
        return;
      }

      try{
        btnConnect.Enabled = false;
        portTxt.Enabled = false;
        clientNameTxt.Enabled = false;
        LogMessage($"서버 {port}에 연결 시도...");
        manager.Connect(port, clientNameTxt.Text);
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
        messageTxt.Clear();
      }
    }

    private void HandleServerConnected(string clientName)
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
        LogMessage(message);
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