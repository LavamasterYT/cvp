using System.Drawing;

namespace ConsoleVideoPlayer;

public class Compression
{
    private Dictionary<Color, int> cache;
    private Dictionary<Color, int> palette;

    public Compression()
    {
        cache = new Dictionary<Color, int>();
        palette = new Dictionary<Color, int>()
        {
            { Color.FromArgb(12, 12, 12), 0 },
            { Color.FromArgb(59, 120, 255), 9 },
            { Color.FromArgb(97, 214, 214), 11 },
            { Color.FromArgb(0, 55, 218), 1 },
            { Color.FromArgb(58, 150, 221), 3 },
            { Color.FromArgb(204, 204, 204), 8 },
            { Color.FromArgb(19, 161, 14), 2 },
            { Color.FromArgb(136, 23, 152), 5 },

            { Color.FromArgb(197, 15, 31), 4 },
            { Color.FromArgb(193, 156, 0), 6 },
            { Color.FromArgb(118, 118, 118), 7 },
            { Color.FromArgb(22, 198, 12), 10 },
            { Color.FromArgb(180, 0, 158), 13 },
            { Color.FromArgb(231, 72, 86), 12 },
            { Color.FromArgb(242, 242, 242), 15 },
            { Color.FromArgb(249, 241, 165), 14 }
        };
    }

    public void SetPalette(string file)
    {
        byte[] data = File.ReadAllBytes(file);
        int[] concolorder = new int[16];

        Array.Copy(palette.Values.ToArray(), concolorder, palette.Values.Count);
        palette = new Dictionary<Color, int>();

        for (int i = 0; i < data.Length; i += 3)
        {
            palette.Add(Color.FromArgb(data[i], data[i + 1], data[i + 2]), concolorder[i / 3]);
        }
    }

    public Bitmap ResizeImage(Bitmap input, int width, int height)
    {
        return new Bitmap(input, new Size(width, height));
    }

    public byte[] ConvertFrame(Bitmap res)
    {
        List<byte> result = new List<byte>();

        byte data = 0;
        int concol;

        for (int y = 0; y < res.Height; y++)
        {
            for (int x = 0; x < res.Width; x++)
            {
                Color color = res.GetPixel(x, y);
                concol = 0;

                if (cache.ContainsKey(color))
                {
                    concol = cache[color];
                }
                else
                {
                    double distance = 10000000;

                    foreach (var i in palette.Keys)
                    {
                        int r = color.R - i.R;
                        int g = color.G - i.G;
                        int b = color.B - i.B;

                        double d = Math.Abs(Math.Sqrt(r * r + g * g + b * b));

                        if (distance > d)
                        {
                            distance = d;
                            concol = palette[i];
                        }
                    }
                }

                if (x % 2 == 0)
                {
                    data = (byte)(concol << 4);
                }
                else
                {
                    data = (byte)(data | concol);
                    result.Add(data);
                    data = 0;
                }
            }
        }

        return result.ToArray();
    }
}
