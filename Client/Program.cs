﻿using System;
using System.Windows.Forms;
using Client.Forms;

namespace Client
{
  class Program
  {
    [STAThread]
    internal static void Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new ClientForm());
    }
  }
}