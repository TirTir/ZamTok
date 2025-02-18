using System;
using System.Windows.Forms;
using Client.Forms;

namespace Client
{
  class Program
  {
    [STAThread]
        static void Main()
        {
            ApplicationConfiguration.Initialize();
            Application.Run(new ClientForm());
        }
  }
}