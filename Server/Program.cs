using System;
using System.Windows.Forms;
using Server.Forms;

namespace Server
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            ApplicationConfiguration.Initialize();
            Application.Run(new ServerForm());  // 정확한 클래스 참조
        }
    }
}