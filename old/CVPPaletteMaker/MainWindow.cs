namespace CVPPaletteMaker
{
    public partial class MainWindow : Form
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void ColorPicked(object sender, EventArgs e)
        {
            dlgColor.FullOpen = true;
            dlgColor.Color = ((Panel)sender).BackColor;
            
            if (dlgColor.ShowDialog() == DialogResult.OK)
            {
                ((Panel)sender).BackColor = dlgColor.Color;
            }
        }

        private void SaveClicked(object sender, EventArgs e)
        {
            byte[] data = new byte[]
            {
                black.BackColor.R, black.BackColor.G, black.BackColor.B,
                blue.BackColor.R, blue.BackColor.G, blue.BackColor.B,
                cyan.BackColor.R, cyan.BackColor.G, cyan.BackColor.B,
                dblue.BackColor.R, dblue.BackColor.G, dblue.BackColor.B,
                dcyan.BackColor.R, dcyan.BackColor.G, dcyan.BackColor.B,
                dgray.BackColor.R, dgray.BackColor.G, dgray.BackColor.B,
                dgreen.BackColor.R, dgreen.BackColor.G, dgreen.BackColor.B,
                dmagenta.BackColor.R, dmagenta.BackColor.G, dmagenta.BackColor.B,

                dred.BackColor.R, dred.BackColor.G, dred.BackColor.B,
                dyellow.BackColor.R, dyellow.BackColor.G, dyellow.BackColor.B,
                gray.BackColor.R, gray.BackColor.G, gray.BackColor.B,
                green.BackColor.R, green.BackColor.G, green.BackColor.B,
                magenta.BackColor.R, magenta.BackColor.G, magenta.BackColor.B,
                red.BackColor.R, red.BackColor.G, red.BackColor.B,
                white.BackColor.R, white.BackColor.G, white.BackColor.B,
                yellow.BackColor.R, yellow.BackColor.G, yellow.BackColor.B,
            };

            string file = "";

            dlgSave.Filter = ".cvpp files (*.cvpp)|*.cvpp|All files (*.*)|*.*";
            dlgSave.FileName = "Palette 1.cvpp";

            if (dlgSave.ShowDialog() == DialogResult.OK)
            {
                file = dlgSave.FileName;
                File.WriteAllBytes(file, data);
            }
        }

        private void OpenClicked(object sender, EventArgs e)
        {
            dlgOpen.Filter = ".cvpp files (*.cvpp)|*.cvpp|All files (*.*)|*.*";
            
            if (dlgOpen.ShowDialog() == DialogResult.OK)
            {
                byte[] data = File.ReadAllBytes(dlgOpen.FileName);

                black.BackColor = Color.FromArgb(data[0], data[1], data[2]);
                blue.BackColor = Color.FromArgb(data[3], data[4], data[5]);
                cyan.BackColor = Color.FromArgb(data[6], data[7], data[8]);
                dblue.BackColor = Color.FromArgb(data[9], data[10], data[11]);
                dcyan.BackColor = Color.FromArgb(data[12], data[13], data[14]);
                dgray.BackColor = Color.FromArgb(data[15], data[16], data[17]);
                dgreen.BackColor = Color.FromArgb(data[18], data[19], data[20]);
                dmagenta.BackColor = Color.FromArgb(data[21], data[22], data[23]);

                dred.BackColor = Color.FromArgb(data[24], data[25], data[26]);
                dyellow.BackColor = Color.FromArgb(data[27], data[28], data[29]);
                gray.BackColor = Color.FromArgb(data[30], data[31], data[32]);
                green.BackColor = Color.FromArgb(data[33], data[34], data[35]);
                magenta.BackColor = Color.FromArgb(data[36], data[37], data[38]);
                red.BackColor = Color.FromArgb(data[39], data[40], data[41]);
                white.BackColor = Color.FromArgb(data[42], data[43], data[44]);
                yellow.BackColor = Color.FromArgb(data[45], data[46], data[47]);
            }
        }
    }
}
