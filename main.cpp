#include "Ledcube.h"
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <iostream>
#include <bitset>

void setNonBlockingInput(bool enable) {
    static struct termios oldt, newt;

    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);           // save old settings
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);         // disable buffering & echo
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);  // apply new settings

        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // non-blocking mode
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // restore old settings
    }
}

int main() {
    //setNonBlockingInput(true);
    std::string c;
    int n = 0;
    LOG("threads: " << std::thread::hardware_concurrency());

    int frameCount = 0;
    Ledcube ledcube([&frameCount](Ledcube& ledcube) {
        frameCount++;
        for(int i = 0; i < 8; i++) {
            for(int j = 0; j < 8; j++) {
                for(int k = 0; k < 8; k++) {
                    ledcube.SetPixel(i, j, k, 0, 255);
                    ledcube.SetPixel(i, j, k, 2, 255);
                    if(k == 7||k==6) ledcube.SetPixel(i, j, k, 1, k*(255./7.));
                    else ledcube.SetPixel(i, j, k, 1, 0);
                }
            }
        }
    });
    while(true) {// read(STDIN_FILENO, &ch, 1) <= 0
        std::cin >> c;
        if(c == "s") break;
    }
}