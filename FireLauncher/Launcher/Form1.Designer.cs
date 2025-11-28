using Fire_Launcher.Launcher;

namespace FireLauncher
{
    partial class Form1
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            progressBar = new ProgressBar();
            richTextBox = new RichTextBox();
            progressBar1 = new ProgressBar();
            playB = new Button();
            homePB = new Button();
            button1 = new Button();
            SuspendLayout();
            // 
            // progressBar
            // 
            progressBar.Location = new Point(12, 114);
            progressBar.Name = "progressBar";
            progressBar.Size = new Size(349, 23);
            progressBar.TabIndex = 1;
            progressBar.Click += progressBar_Click;
            // 
            // richTextBox
            // 
            richTextBox.Enabled = false;
            richTextBox.Location = new Point(12, 12);
            richTextBox.Name = "richTextBox";
            richTextBox.Size = new Size(349, 96);
            richTextBox.TabIndex = 4;
            richTextBox.Text = "";
            richTextBox.TextChanged += richTextBox_TextChanged;
            // 
            // progressBar1
            // 
            progressBar1.Location = new Point(12, 143);
            progressBar1.Name = "progressBar1";
            progressBar1.Size = new Size(349, 23);
            progressBar1.TabIndex = 5;
            // 
            // playB
            // 
            playB.Enabled = false;
            playB.Location = new Point(12, 172);
            playB.Name = "playB";
            playB.Size = new Size(75, 23);
            playB.TabIndex = 6;
            playB.Text = "Play";
            playB.UseVisualStyleBackColor = true;
            playB.Click += playB_Click;
            // 
            // homePB
            // 
            homePB.Location = new Point(205, 172);
            homePB.Name = "homePB";
            homePB.Size = new Size(75, 23);
            homePB.TabIndex = 7;
            homePB.Text = "Homepage";
            homePB.UseVisualStyleBackColor = true;
            homePB.Click += homePB_Click;
            // 
            // button1
            // 
            button1.Location = new Point(286, 172);
            button1.Name = "button1";
            button1.Size = new Size(75, 23);
            button1.TabIndex = 8;
            button1.Text = "Exit";
            button1.UseVisualStyleBackColor = true;
            button1.Click += button1_Click;
            // 
            // Form1
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            BackColor = SystemColors.Control;
            ClientSize = new Size(375, 204);
            Controls.Add(button1);
            Controls.Add(homePB);
            Controls.Add(playB);
            Controls.Add(progressBar1);
            Controls.Add(richTextBox);
            Controls.Add(progressBar);
            Name = Config.LauncherName;
            StartPosition = FormStartPosition.CenterScreen;
            Text = Config.LauncherName;
            Load += Form1_Load;
            ResumeLayout(false);
        }

        #endregion

        public ProgressBar progressBar;
        public RichTextBox richTextBox;
        public ProgressBar progressBar1;
        public Button playB;
        public Button homePB;
        public Button button1;
    }
}
