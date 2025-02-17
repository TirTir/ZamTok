using System.Windows.Forms;
using ServerNetwork;

namespace Server.Forms
{
    public partial class ServerForm : Form
    {
        private ServerManager manager;
        private ListBox connectedClients;
        private TextBox txtLog;
        private Button btnStart;

        public ServerForm()
        {
            InitializeComponent();
            this.Text = "채팅 서버";  // 폼 제목 설정
            this.Size = new Size(650, 600);  // 폼 크기 설정
            
            manager = new ServerManager();
            manager.OnConnected += HandleClientConnected;
            manager.OnMessageReceived += HandleMessageReceived;

            InitializeUI();
        }

        private void InitializeUI()
        {
            // 포트 입력 패널
            var portPanel = new Panel
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
                Text = "서버 시작"
            };
            btnStart.Click += btnStartServer;

            portPanel.Controls.Add(btnStart);

            // 로그 & 클라이언트 목록 영역
            var mainPanel = new Panel
            {
                Dock = DockStyle.Fill
            };

            connectedClients = new ListBox
            {
                Dock = DockStyle.Left,
                Width = 200,
                Font = new Font("맑은 고딕", 10)
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

            var lblClients = new Label
            {
                Text = "연결된 클라이언트",
                Dock = DockStyle.Top,
                Height = 30,
                Font = new Font("맑은 고딕", 10, FontStyle.Bold),
                TextAlign = ContentAlignment.MiddleCenter
            };

            var leftPanel = new Panel
            {
                Dock = DockStyle.Left,
                Width = 200
            };
            leftPanel.Controls.Add(connectedClients);
            leftPanel.Controls.Add(lblClients);

            mainPanel.Controls.Add(txtLog);
            mainPanel.Controls.Add(leftPanel);

            Controls.Add(mainPanel);
            Controls.Add(portPanel);

            this.Load += (s, e) => LogMessage("서버 시작 버튼을 눌러주세요.");
        }

        private void btnStartServer(object sender, EventArgs e)
        {
            btnStart.Enabled = false;

            manager.StartServer(8888);
            LogMessage($"서버가 포트 {8888}에서 시작되었습니다.");
        }

        private void HandleClientConnected(string clientInfo)
        {
            connectedClients.Items.Add(clientInfo);
            LogMessage($"클라이언트 연결됨: {clientInfo}");
        }

        private void HandleMessageReceived(string message)
        {
            LogMessage($"수신된 메시지: {message}");
        }

        private void LogMessage(string message)
        {
            string date = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");

            Console.WriteLine($"LogMessage: {message}");
            Invoke(new MethodInvoker(delegate ()
            {
                txtLog!.AppendText("[" + date + "] " + message + "\r\n");
            }));
        }
    }
}