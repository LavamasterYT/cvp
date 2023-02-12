#include "renderer.hpp"

#include <opencv2/opencv.hpp>
#include <csignal>

Renderer r;
int videoWidth;
int videoHeight;
int xoffset;

void sig_int(int signum) {
    r.Clean();
    exit(signum);
}

void set_dimensions() {
    videoWidth = (16 * r.Height) / 9;
    videoHeight = (r.Height);
    xoffset = (r.Width / 2) - (videoWidth / 2);

    if (xoffset < 0)
        xoffset = 0;
}

void sig_sz(int signum) {
    if (r.SetDimensions()) {
        set_dimensions();
    }
}

int main(int argc, char** argv) {
    std::string filename;

    videoWidth = 0;
    videoHeight = 0;
    xoffset = 0;

    signal(SIGINT, sig_int);

#ifndef _WIN32
    signal(SIGWINCH, sig_sz);
#endif

    if (argc != 2) {
        filename = "/Users/josem/Desktop/test.mp4";
    }
    else {
        filename = argv[1];
    }

    bool done = false;

    r.Clear({ 0, 0, 0 });
    r.Render();

    set_dimensions();

    cv::VideoCapture src(filename);

    cv::Mat init;
    cv::Mat frame;

    cv::Vec3b col;
    bool flag = false;

    auto finish = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - finish);
    auto start = std::chrono::high_resolution_clock::now();

    while (src.read(init)) {
        cv::resize(init, frame, cv::Size(videoWidth, videoHeight), 0, 0, cv::INTER_NEAREST_EXACT);

        for (int y = 0; y < videoHeight; y++)
            for (int x = 0; x < videoWidth; x++) {
                col = frame.at<cv::Vec3b>(y, x);

                r.SetPixel(x + xoffset, y, { col[2], col[1], col[0] });
            }

        r.Render();

        finish = std::chrono::high_resolution_clock::now();
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);
        src.set(cv::CAP_PROP_POS_MSEC, ms.count());
    }

    r.Clean();
}
