using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static ConsoleVideoPlayer.Win32;

namespace ConsoleVideoPlayer
{
    public class NativeInput
    {
        public Dictionary<int, (ushort, ushort)> KeyStates { get; set; }

        public NativeInput(int[] keys)
        {
            this.KeyStates = new Dictionary<int, (ushort, ushort)>();

            foreach (var i in keys)
            {
                KeyStates.Add(i, (0, 0));
            }
        }

        public void HandleInput()
        {
            foreach (var i in KeyStates.Keys)
            {
                ushort old = KeyStates[i].Item1;
                KeyStates[i] = (GetAsyncKeyState(i), old);
            }
        }
    }
}
