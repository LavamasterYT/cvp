# cvp
 cvp (**C**onsole **V**ideo **P**layer), is a lightweight cross platform video player in the console!

![Showcase of Big Buck Bunny playing](img/showcase.gif?raw=true)

## Download

Binaries should be provided "hopefully" for the latest release.
I can't guarantee that the binaries will run successfully, so I recommend that you build this yourself.

## Building

I finally got proper build instructions.

Install the necessary dependencies. There may be more required than the listed ones below based on your OS/distro.
Check with your distro/OS to make sure you got everything needed to run correctly.

<details>
<summary>Windows</summary>

I recommend using Visual Studio for building, but you could in theory use any IDE that can support CMake.

Download [CMake](https://cmake.org/download/) and [Git](https://github.com/git-for-windows/git).

If using Visual Studio, make sure you have the proper C++ packages installed.

</details>

<details>
<summary>macOS</summary>

Install the Xcode command line tools:
```
xcode-select --install
```

Make sure you install [brew](https://brew.sh/) and any other dependencies that may be needed:
```
brew install cmake pkg-config nasm
```

</details>

<details>
<summary>Linux</summary>

Make sure you install `cmake` and `git` for building and cloning. Also make sure you have
some developer tools installed like a C++ compiler/linker, etc.

</details>


Afterwards, building should be straightforward.

Clone the GitHub repository along with the submodules:
```
~ $ git clone --recursive https://github.com/LavamasterYT/cvp
```

Go into the directory and setup `vcpkg`:
```
~ $ cd cvp/vcpkg

# Windows:
~/cvp/vcpkg $ .\bootstrap-vcpkg.bat

# macOS/Linux
~/cvp/vcpkg $ ./bootstrap-vcpkg.sh
```

Create the build directory:
```
~/cvp/vcpkg $ cd ..
~/cvp $ mkdir build && cd build
```

Build the project files using CMake. This can differ based on what type of project files you want. For Makefile:
```
~/cvp/build $ cmake ..
~/cvp/build $ make
```


## Usage
To play a video file, run the following command:
```
$ cvp <video file>
```
To play a video file in full RGB mode, run the following command:
```
$ cvp -f <video file>
```
To play a video file with audio:
```
$ cvp -a <video file>
```
More help is available by running:
```
$ cvp -h
```

> Some codecs (i think thats the issue? i still need to look into it) have trouble playing back. If you do, you can run `cvp` with multithreading by adding the `-t`  flag.

### Playback
The dimension the video will be rendered will be based off the current size of the terminal (so feel free to experiment with different sizes). The video will by default render in 16 colors for compatibility with most terminal emulators. Video frames are scaled down to the size of the terminal, then its colors are converted to the closest color in the 16 color palette that is defined in [renderer.c](https://github.com/LavamasterYT/cvp/blob/main/src/renderer.c#L19).

If your terminal supports truecolor, you can render in 24 bit by using the `-f` or `--full-color` flag, however, it may be slower due to having to push more data to the terminal.

## Development
Where to begin...

This is a rewrite of the old version I made in C#, if you want to look at how that worked, [look at the old repository](https://github.com/LavamasterYT/cvp/tree/180db8f0c03c20cdbccaf0f8848f757fa73888d8).

The problem with that version though was that it was not cross-platform, it only worked on Windows. For the longest time, I wanted to make it cross-platform, and make it better and faster. I finally got around to doing it, however, it had to be made in C and using FFmpeg.

Although I had no reason to, the [decoder](https://github.com/LavamasterYT/cvp/blob/main/src/decoder.h) and [renderer](https://github.com/LavamasterYT/cvp/blob/main/src/renderer.h) are able to be used independently of each other and this project. I don't for what reasons you would want to, but it's there.

### Decoder
The decoder part of the project is all located and implemented in the [decoder.c](https://github.com/LavamasterYT/cvp/blob/main/src/decoder.c) and [decoder.h](https://github.com/LavamasterYT/cvp/blob/main/src/decoder.h) files.

Decoding is being done using the [FFmpeg](https://ffmpeg.org/) libraries. This has the added advantage that it is able to support a large number of codecs. While I could have used OpenCV like I did last time (and it used OpenCV in the beginning), I think having more control over the decoding process was better, and this also removes any overhead that OpenCV may introduce (even though it may be little to none). This also makes building the project a lot more simpler and smaller.

### Renderer
The renderer is located and implemented in the [renderer.c](https://github.com/LavamasterYT/cvp/blob/main/src/renderer.c) and [renderer.h](https://github.com/LavamasterYT/cvp/blob/main/src/renderer.h) files.

Unlike the previous version which utilized the Windows API to manually write to the console buffer, this version uses ANSI sequences to render to the terminal. Although it may be slower, this has the advantage of being cross-platform and having more potential color support.

For rendering in palette mode, the output buffer is converted to the closest color in the 16 color palette using the euclidean algorithm. Then using ANSI sequences, it is written to the terminal. In truecolor mode, there is no conversion and instead each RGB value is rendered in the terminal. Due to more data being passed, it is slower than palette mode. However, it is still faster than the previous version. The program switches to the alternate screen buffer to prevent from messing with the main screen buffer.

With now using ANSI sequences to render to the terminal, it is possible to render video that looks as good as this:
![Demon Slayer in truecolor](img/truecolor1.jpg?raw=true)
![Demon Slayer in truecolor](img/truecolor2.jpg?raw=true)

As opposed to this (palette mode, also how old version looked):
![Demon Slayer in palette mode](img/palette1.png?raw=true)
![Demon Slayer in palette mode](img/palette2.png?raw=true)

## Credits
- [FFmpeg](https://ffmpeg.org/) for decoding functions