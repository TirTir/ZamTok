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

            // 클라이언트1
            Thread thread1 = new Thread(() => Application.Run(new ClientForm()));
            thread1.SetApartmentState(ApartmentState.STA);
            thread1.Start();

            // 클라이언트2
            Thread thread2 = new Thread(() => Application.Run(new ClientForm()));
            thread2.SetApartmentState(ApartmentState.STA);
            thread2.Start();
        }
  }
}