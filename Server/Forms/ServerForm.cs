using System.Windows.Forms;
using ServerNetwork;

namespace Server.Forms
{
    public partial class ServerForm : Form
    {
        private ServerManager manager;
        private ListBox connectedClients;
        private ListBox connectedRooms;
        private Button btnStart;
        private Button btnStop;
        private TextBox txtLog;

        public ServerForm()
        {
            InitializeComponent();
            this.Text = "채팅 서버";  // 폼 제목 설정
            this.Size = new Size(600, 650);  // 폼 크기 설정
            
            manager = new ServerManager();
            manager.OnConnected += HandleClientConnected;
            manager.OnDisconnected += HandleClientDisconnected;
            manager.OnMessageReceived += HandleMessageReceived;

            InitializeUI();
        }

        private void InitializeUI()
        {
            // 서버 시작/종료 버튼 패널
            var seraverPanel = new Panel
            {
                Dock = DockStyle.Top,
                Height = 50,
                Padding = new Padding(10)
            };

            btnStart = new Button
            {
                Location = new Point(10, 15),
                Width = 100,
                Height = 30,
                Text = "Start"
            };
            btnStart.Click += btnStartServer;

            btnStop = new Button
            {
                Location = new Point(120, 15),
                Width = 100,
                Height = 30,
                Text = "Stop"
            };
            btnStop.Click += btnStopServer;

            seraverPanel.Controls.Add(btnStart);
            seraverPanel.Controls.Add(btnStop);

            // 클라이언트 & 채팅방 목록 영역
            var tablePanel = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                ColumnCount = 2, // 2개의 열
            };

            tablePanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
            tablePanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));

            connectedClients = new ListBox
            {
                Dock = DockStyle.Fill,
                Font = new Font("맑은 고딕", 10)
            };

            connectedRooms = new ListBox
            {
                Dock = DockStyle.Fill,
                Font = new Font("맑은 고딕", 10)
            };

            var lblRooms = new Label
            {
                Text = "채팅방 목록",
                Dock = DockStyle.Top,
                Font = new Font("맑은 고딕", 10, FontStyle.Bold),
                TextAlign = ContentAlignment.MiddleCenter
            };

            var lblClients = new Label
            {
                Text = "연결된 클라이언트",
                Dock = DockStyle.Top,
                Font = new Font("맑은 고딕", 10, FontStyle.Bold),
                TextAlign = ContentAlignment.MiddleCenter
            };

            var leftPanel = new Panel
            {
                Dock = DockStyle.Fill,
            };
            leftPanel.Controls.Add(connectedClients);
            leftPanel.Controls.Add(lblClients);

            var rightPanel = new Panel
            {
                Dock = DockStyle.Fill,
            };
            rightPanel.Controls.Add(connectedRooms);
            rightPanel.Controls.Add(lblRooms);

            tablePanel.Controls.Add(leftPanel, 0, 0);
            tablePanel.Controls.Add(rightPanel, 1, 0);
            
            var bottomPanel = new Panel
            {
                Dock = DockStyle.Bottom,
                Height = 250
            };

            txtLog = new TextBox
            {
                Dock = DockStyle.Fill,
                Multiline = true,
                ScrollBars = ScrollBars.Vertical,
                ReadOnly = true,
                Font = new Font("맑은 고딕", 10),
                BackColor = Color.White
            };
            bottomPanel.Controls.Add(txtLog);

            Controls.Add(tablePanel);
            Controls.Add(seraverPanel);
            Controls.Add(bottomPanel);
            this.Load += (s, e) => LogMessage("서버 시작 버튼을 눌러주세요.");
        }

        private void btnStartServer(object sender, EventArgs e)
        {
            btnStart.Enabled = false;
            manager.StartServer();
            LogMessage($"서버가 시작되었습니다.");
        }
        private void btnStopServer(object sender, EventArgs e)
        {
            btnStart.Enabled = true;
            manager.StopServer();
            LogMessage($"서버가 종료되었습니다.");
            this.Load += (s, e) => LogMessage("서버 시작 버튼을 눌러주세요.");
        }

        private void HandleClientConnected(string chatName, string clientName)
        {
            connectedClients.Items.Add(clientName); // 연결된 클라이언트
            if(!connectedRooms.Items.Contains(chatName))
            {
                connectedRooms.Items.Add(chatName); // 연결된 채팅방
            }
            LogMessage($"{clientName}님이 {chatName}에 입장하셨습니다.");
        }

        private void HandleClientDisconnected(string clientName)
        {
            connectedClients.Items.Remove(clientName);
            LogMessage($"{clientName}님이 채팅방을 나갔습니다.");
        }

        private void HandleMessageReceived(string message)
        {
            Invoke((MethodInvoker)delegate {
                LogMessage(message);
            });
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
                txtLog.AppendText("[" + date + "] " + message + "\r\n");
            }
            catch(Exception ex)
            {
                LogMessage($"오류: {ex.Message}");
            }
        }
    }
}