#ifndef LEDCUBE_H
#define LEDCUBE_H

#include "PCH.h"
#include "GPIO.h"
#include "SPI.h"
#include "MultiThreading.h"

#define DM_LATCH 5//5
#define LA_LATCH 27//27
#define DM_ENABLE 22

#define LEDCUBE_LAYER_BUFFERS 16
#define LEDCUBE_FRAME_BUFFERS 2
#define LEDCUBE_COLOR_PRECISION 2// between 1-8 bits
#define LEDCUBE_FPS 60

class Ledcube {
public:
    // The function is run on another thread then the main thread, Ledcube should not be stored and be used in another thread
    Ledcube(std::function<void(Ledcube& ledcube)> setFrame);
    ~Ledcube();

    void SetPixel(const uint8_t x, const uint8_t y, const uint8_t z, const uint8_t rgb, const uint8_t value);

private:

    void FrameThread(std::function<void(Ledcube& ledcube)> setFrame);
    void ColorThread();
    void WriteThread();

    SPI dmSPI;// write thread
    SPI laSPI;// write thread
    uint8_t _currentBuffer = 0;// Write thread
    uint8_t _currentLayer = 0;// Write thread

    uint8_t _currentLayerColoring = 0;
    uint8_t _currentFrame = 0;// Color thread
    uint8_t _currentStagingFrameBuffer = 0;
    uint16_t _currentColor = 0;// Color thread
    uint8_t _layerData[LEDCUBE_LAYER_BUFFERS][24]; // x buffers with 1 layer
    uint8_t _frameData[LEDCUBE_FRAME_BUFFERS][8][24*8]; // x buffers, 8 layers

    std::thread _writeThread;// main thread
    std::atomic<bool> _runWriteThread = true;// main and write thread

    std::thread _colorThread;// color thread
    std::atomic<bool> _runColorThread = true;// main and color thread
    MultithreadCommandQueue<uint8_t> _colorCommandQueue;// color and write thread

    std::thread _frameThread;// frame thread
    std::atomic<bool> _runFrameThread = true;// frame thread
    MultithreadCommandQueue<uint8_t> _frameCommandQueue;// frame thread
    std::chrono::steady_clock::time_point _frameEndTime = std::chrono::steady_clock::now();
    

    #ifdef LEDCUBE_FRAME_PROFILING
    int _amountFrames = 0;// write thread
    std::chrono::steady_clock::time_point _endTime;// write thread
    int _longestFrameTime = 0;
    int _shortesFrameTime = INT32_MAX;
    std::chrono::steady_clock::time_point _profilingStartTime;
    int _awaitingColorQueue = 0;
    #endif
};

#endif