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

      btnConnect.Enabled = false;
      portTxt.Enabled = false;
      
      manager.Connect(port);
      LogMessage($"서버 {port}에 연결 시도...");
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

    private void HandleServerConnected(string serverInfo)
    {
      messages.Items.Add(serverInfo);
      LogMessage($"서버 연결됨: {serverInfo}");
    }
    private void HandleMessageReceived(string message)
    {
      LogMessage(message);
    }

    private void LogMessage(string message)
    {
      string date = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");

      Console.WriteLine($"LogMessage: {message}");
      messages.Items.Add("[" + date + "] " + message);
    }
  }
}