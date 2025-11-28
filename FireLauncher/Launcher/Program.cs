using FireLauncher;
using Microsoft.VisualBasic.ApplicationServices;
using Microsoft.VisualBasic.Logging;
using System;
using System.IO;
using System.Linq;
using System.Security.Policy;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using static System.Net.WebRequestMethods;
using static System.Runtime.InteropServices.JavaScript.JSType;
using static System.Windows.Forms.VisualStyles.VisualStyleElement.TaskbarClock;




namespace Fire_Launcher.Launcher
{
    public static class Config
    {
        public static string Game { get; private set; } = "";
        public static string Server { get; private set; } = "";
        public static string RemoteDir { get; private set; } = "";
        public static string RemotePhp { get; private set; } = "";
        public static string RunnableName { get; private set; } = "";
        public static string LocalRunFolder { get; private set; } = "";
        public static string AppName { get; private set; } = "";
        public static string LauncherName { get; private set; } = "";

        public static void Load()
        {
            string iniPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "config.ini");
            if (!System.IO.File.Exists(iniPath))
                throw new Exception("config.ini not found!");

            foreach (string line in System.IO.File.ReadAllLines(iniPath))
            {
                if (string.IsNullOrWhiteSpace(line) || !line.Contains("="))
                    continue;

                var parts = line.Split('=', 2);

                string key = parts[0].Trim().ToLower();
                string val = parts[1].Trim();

                switch (key)
                {
                    case "server": Server = "https://" + val; break;
                    case "remote_dir": RemoteDir = val; break;
                    case "remote_php": RemotePhp = val; break;
                    case "runnable_name": RunnableName = val; break;
                    case "local_run_folder": LocalRunFolder = val; break;
                    case "launcher_name": LauncherName = val; break;
                }
            }

            Game = AppDomain.CurrentDomain.BaseDirectory;
            AppName = RunnableName.Split('.')[0];
        }
    }

    enum UpdateResult
    {
        IN_PROGRESS,
        DONE,
        SERVER_ERROR,
    }

    internal static class Program
    {
        static Form1? form;
        public static bool fast = true;
        static UpdateResult updateResult = UpdateResult.IN_PROGRESS;
        public static Dictionary<string, (FileStream Stream, string Md5)> LockedFiles = new Dictionary<string, (FileStream, string)>(StringComparer.OrdinalIgnoreCase);

        static async Task<UpdateResult> Update()
        {

            var content = await GetIndexHtmlAsync(Config.Server + "/" + Config.RemoteDir + "/index.html");
            if (content.Length == 0)
                return UpdateResult.SERVER_ERROR;

            form.progressBar1.Invoke(() =>
            {
                form.progressBar1.Value = 25;
            });

            var files = ParseIndexHtml(content);
            if (files.Count == 0)
                return UpdateResult.SERVER_ERROR;

            var mismatches = await GetUnmatchedFiles(files, Config.Game);
            if (mismatches.Count == 0)
                return UpdateResult.DONE;

            form.progressBar1.Invoke(() =>
            {
                form.progressBar1.Value = 50;
            });

            var down = await DownloadMismatchedFiles(mismatches, Config.Game, Config.Server + "/" + Config.RemoteDir + "/");
            if (!down)
                return UpdateResult.SERVER_ERROR;

            content = await GetIndexHtmlAsync(Config.Server + "/" + Config.RemoteDir + "/index.html");
            if (content.Length == 0)
                return UpdateResult.SERVER_ERROR;

            form.progressBar1.Invoke(() =>
            {
                form.progressBar1.Value = 75;
            });

            files = ParseIndexHtml(content);
            if (files.Count == 0)
                return UpdateResult.SERVER_ERROR;

            mismatches = await GetUnmatchedFiles(files, Config.Game);
            if (mismatches.Count == 0)
                return UpdateResult.DONE;

            form.progressBar1.Invoke(() =>
            {
                form.progressBar1.Value = 100;
            });

            return UpdateResult.SERVER_ERROR;
        }

        [STAThread]
        static void Main()
        {

            ApplicationConfiguration.Initialize();
            Config.Load();

            form = new Form1();
            form.Show();



            Task.Run(async () =>
            {
                var result = await Update();

                if (result == UpdateResult.DONE)
                {
                    string shrt = Config.RunnableName;
                    string iconPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, Config.AppName + ".ico");

                    CreateDesktopShortcut(Config.LauncherName, Path.Combine(Config.Game, Config.LauncherName) + ".exe");
                    form.progressBar1.Invoke(() =>
                    {
                        form.progressBar1.Value = 100;
                    });
                    form.playB.Invoke(() =>
                    {
                        form.playB.Enabled = true;
                    });
                    form.progressBar.Invoke(() =>
                    {
                        form.progressBar.Value = 100;
                    });

                }
                else
                {
                    form.progressBar.Invoke(() =>
                    {
                        form.progressBar.Value = 0;
                    });
                    form.progressBar1.Invoke(() =>
                    {
                        form.progressBar1.Value = 0;
                    });
                    form.Log("Failed!");
                }
            });

            Application.Run(form);
        }

        private static async Task<bool> DownloadMismatchedFiles(List<(string Path, string ServerMd5, string LocalMd5)> mismatches, string basePath, string baseUrl)
        {
            bool success = true;

            int i = 0;
            foreach (var m in mismatches)
            {
                try
                {
                    string remoteUrl = baseUrl + m.Path.Replace("\\", "/");
                    string localFile = Path.Combine(basePath, m.Path);

                    Directory.CreateDirectory(Path.GetDirectoryName(localFile)!);

                    Log(string.Format("Downloading({0} / {1}), '{2}'...", ++i, mismatches.Count, GetRelativeFromUrl(remoteUrl)));
                    await DownloadFileWithProgress(remoteUrl, localFile);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"ERROR downloading {m.Path}: {ex.Message}");
                    success = false;
                }
            }

            return success;
        }

        private static async Task DownloadFileWithProgress(string url, string localPath)
        {

            using HttpClient client = new HttpClient();
            using var response = await client.GetAsync(url, HttpCompletionOption.ResponseHeadersRead);
            using var stream = await response.Content.ReadAsStreamAsync();
            using var file = System.IO.File.Create(localPath);

            long total = response.Content.Headers.ContentLength ?? 0;
            long read = 0;
            byte[] buffer = new byte[8192];
            int bytesRead;

            while ((bytesRead = await stream.ReadAsync(buffer)) > 0)
            {
                await file.WriteAsync(buffer, 0, bytesRead);
                read += bytesRead;

                int percent = (int)(read * 100 / total);

                form.progressBar.Invoke(() =>
                {
                    form.progressBar.Value = percent;
                });
            }

            if (!fast)
                await Task.Delay(1000);
        }

        private static async Task<List<(string Path, string ServerMd5, string LocalMd5)>> GetUnmatchedFiles(List<(string Path, string Md5)> parsedList, string basePath)
        {
            Log("Checking files...");

            var unmatched = new List<(string Path, string ServerMd5, string LocalMd5)>();

            foreach (var item in parsedList)
            {
                string localFilePath = Path.Combine(basePath, item.Path);

                if (!LockedFiles.ContainsKey(item.Path))
                {
                    var result = ComputeMd5AndLockIfMatch(localFilePath, item.Md5, item.Path);
                    
                    if (!result.Match)
                    {
                        unmatched.Add((item.Path, item.Md5, result.ComputedMD5));
                    }
                }
            }

            if (!fast)
                await Task.Delay(1000);

            if (unmatched.Count == 0)
                Log("All files are up to date.");
            else
                Log(string.Format("{0} files are out of date.", unmatched.Count));

            await Task.Delay(1000);

            return unmatched;
        }

        private static List<(string Path, string Md5)> ParseIndexHtml(string html)
        {
            var list = new List<(string Path, string Md5)>();

            var matches = Regex.Matches(html, @"<p>(.*?)<\/p>", RegexOptions.IgnoreCase);

            foreach (Match m in matches)
            {
                string line = m.Groups[1].Value.Trim();

                string[] parts = line.Split(':', 2);
                if (parts.Length != 2)
                    continue;

                list.Add((parts[0], parts[1]));
            }

            return list;
        }

        private static async Task<string> GetIndexHtmlAsync(string addr)
        {
            Log("Getting file list...");

            if (!fast)
                await Task.Delay(1000);

            HttpClient client = new HttpClient();

            HttpResponseMessage response = await client.GetAsync(addr);

            string content = await response.Content.ReadAsStringAsync();

            return content;
        }

        private static void CreateDesktopShortcut(string name, string exePath)
        {
            string desktop = Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
            string shortcutPath = Path.Combine(desktop, name + ".lnk");

            string exe = exePath.Replace("\\", "\\\\");
            string workDir = Path.GetDirectoryName(exePath)?.Replace("\\", "\\\\") ?? "";

            // ICON = exe’nin kendi ikonu
            string icon = exe;

            string vbs = $@"
Set oWS = WScript.CreateObject(""WScript.Shell"")
sLinkFile = ""{shortcutPath.Replace("\\", "\\\\")}""
Set oLink = oWS.CreateShortcut(sLinkFile)
oLink.TargetPath = ""{exe}""
oLink.WorkingDirectory = ""{workDir}""
oLink.IconLocation = ""{icon}""
oLink.Save
";

            string tempVbs = Path.Combine(Path.GetTempPath(), "create_shortcut.vbs");
            System.IO.File.WriteAllText(tempVbs, vbs);

            var p = new System.Diagnostics.Process();
            p.StartInfo.FileName = "wscript.exe";
            p.StartInfo.Arguments = "\"" + tempVbs + "\"";
            p.StartInfo.CreateNoWindow = true;
            p.Start();
        }

        private static (bool Match, string ComputedMD5)ComputeMd5AndLockIfMatch(string filePath, string expectedMd5, string relativePath)
        {
            // Dosya yoksa
            if (!System.IO.File.Exists(filePath))
                return (false, "");

            FileStream fs = null;

            try
            {
                fs = new FileStream(filePath, FileMode.Open, FileAccess.ReadWrite, FileShare.None);

                using (var md5 = System.Security.Cryptography.MD5.Create())
                {
                    byte[] hash = md5.ComputeHash(fs);
                    string computed = Convert.ToHexString(hash);

                    if (computed.Equals(expectedMd5, StringComparison.OrdinalIgnoreCase))
                    {
                        fs.Position = 0;
                        LockedFiles[relativePath] = (fs, computed);
                        return (true, computed);
                    }
                    else
                    {
                        fs.Dispose();
                        return (false, computed);
                    }
                }
            }
            catch
            {
                fs?.Dispose();
                return (false, "");
            }
        }

        private static string GetRelativeFromUrl(string fullUrl)
        {
            int i = fullUrl.IndexOf("/up/", StringComparison.OrdinalIgnoreCase);
            if (i == -1) return fullUrl;

            return fullUrl.Substring(i + 4);  // skip "/up/"
        }

        private static void Log(string msg)
        {
            if (form == null)
                return;

            if (form.InvokeRequired)
            {
                form.Invoke(new Action<string>(Log), msg);
                return;
            }

            form.richTextBox.AppendText(msg + Environment.NewLine);
            form.richTextBox.SelectionStart = form.richTextBox.TextLength;
            form.richTextBox.ScrollToCaret();
        }
    }
}