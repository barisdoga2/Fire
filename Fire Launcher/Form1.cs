using System.Diagnostics;
using System.Net.Sockets;
using System.Windows.Forms;

namespace FireLauncher
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();

            string icoPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, Config.LauncherName + ".ico");

            if (System.IO.File.Exists(icoPath))
                this.Icon = new Icon(icoPath);
        }

        public void Log(string msg)
        {
            if (InvokeRequired)
            {
                Invoke(new Action<string>(Log), msg);
                return;
            }

            richTextBox.AppendText(msg + Environment.NewLine);
            richTextBox.ScrollToCaret();
        }

        public void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            //if (e.CloseReason == CloseReason.UserClosing ||
            //    e.CloseReason == CloseReason.TaskManagerClosing ||
            //    e.CloseReason == CloseReason.FormOwnerClosing ||
            //    e.CloseReason == CloseReason.MdiFormClosing)
            //{
            //    e.Cancel = true;
            //    return;
            //}
        }

        private void playB_Click(object sender, EventArgs e)
        {
            try
            {
                foreach (var kvp in Program.LockedFiles)
                {
                    try
                    {
                        kvp.Value.Stream?.Dispose();
                    }
                    catch { /* ignore */ }
                }

                Program.LockedFiles.Clear();

                string exePath = Path.Combine(Path.Combine(Config.Game, Config.LocalRunFolder), Config.RunnableName);
                System.Diagnostics.Process.Start(exePath);

                button1.Invoke(() =>
                {
                    button1.Enabled = false;
                });
                Log("Success!");
                //Application.ApplicationExit += (s, e) => {
                //    Environment.FailFast("");
                //};

                //FormClosing += Form1_FormClosing;
                Application.Exit();
            }
            catch (Exception ex)
            {
                Log("Error launching " + Config.RunnableName + ": " + ex.Message);
            }
        }

        private void homePB_Click(object sender, EventArgs e)
        {
            try
            {
                System.Diagnostics.Process.Start(new ProcessStartInfo
                {
                    FileName = Config.Server,
                    UseShellExecute = true
                });
            }
            catch (Exception ex)
            {
                Log("Cannot open homepage: " + ex.Message);
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            this.FormBorderStyle = FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.MinimizeBox = false;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void richTextBox_TextChanged(object sender, EventArgs e)
        {

        }

        private void progressBar_Click(object sender, EventArgs e)
        {

        }
    }
}
