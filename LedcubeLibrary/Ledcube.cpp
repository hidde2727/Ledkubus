#include "Ledcube.h"

Ledcube::Ledcube(std::function<void(Ledcube& ledcube)> setFrame) : dmSPI(1), laSPI(1) {
    LOG("Hello world");
    GPIO::Setup();
    GPIO::Output(LA_LATCH);
    GPIO::Output(DM_LATCH);
    GPIO::Output(DM_ENABLE);
    GPIO::Write0(LA_LATCH);
    GPIO::Write0(DM_LATCH);
    GPIO::Write1(DM_LATCH);
    GPIO::Write0(DM_ENABLE);

    for(int i = 0; i < LEDCUBE_FRAME_BUFFERS; i++) _frameCommandQueue.PushBack(i);
    for(int i = 0; i < LEDCUBE_LAYER_BUFFERS; i++) _colorCommandQueue.PushBack(i);
    memset(&_layerData, 0, 24*8);
    #ifdef LEDCUBE_FRAME_COUNTING
    _amountFrames = 0;
    _endTime = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    #endif

    _frameThread = std::thread(&Ledcube::FrameThread, this, setFrame);
    _writeThread = std::thread(&Ledcube::WriteThread, this);
    _colorThread = std::thread(&Ledcube::ColorThread, this);
}
Ledcube::~Ledcube() {
    _runWriteThread.store(false);
    _writeThread.join();
    _runColorThread.store(false);
    _colorCommandQueue.PushBack(0);
    _colorThread.join();
    _runFrameThread.store(false);
    _frameCommandQueue.PushBack(0);
    _frameThread.join();

    const uint8_t data[24] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    dmSPI.Send<24>(data);
    GPIO::Write(DM_LATCH, true);
    GPIO::Write(DM_LATCH, false);

    const uint8_t data1[1] = {0b11111111};
    laSPI.Send<1>(data1);
    GPIO::Write(LA_LATCH, true);
    GPIO::Write(LA_LATCH, false);

    LOG("stopped")
}

void Ledcube::FrameThread(std::function<void(Ledcube& ledcube)> setFrame) {
    while(!_runFrameThread.load()) {}
    LOG("Color thread started" << std::boolalpha << _runFrameThread.load())
    while(_runFrameThread.load()) {
        _currentStagingFrameBuffer = _frameCommandQueue.AwaitAvailable();
        setFrame(*this);
        _frameCommandQueue.PopFront();
    }

}
void Ledcube::ColorThread() {
    while(!_runColorThread.load()) {}
    LOG("Color thread started" << std::boolalpha << _runColorThread.load())
    while(_runColorThread.load()) {
        uint8_t processBuffer = _colorCommandQueue.AwaitAvailable();
        _currentLayerColoring++;
        _currentLayerColoring %= 8;
        if(_currentLayerColoring == 0) {
            _currentColor++;
            _currentColor %= 0xFF >> (8-LEDCUBE_COLOR_PRECISION);
        }
        if(_frameEndTime < std::chrono::steady_clock::now()) {
            uint8_t availableFrame = _currentFrame;
            _currentFrame++;
            _currentFrame %= LEDCUBE_FRAME_BUFFERS;
            _frameCommandQueue.AwaitProcessed(_currentFrame);
            _frameCommandQueue.PushBack(availableFrame);
            _frameEndTime = std::chrono::steady_clock::now() + std::chrono::nanoseconds(100000000/LEDCUBE_FPS);
        }

        // Process
        for(int i = 0; i < 24; i++) {
            _layerData[processBuffer][i] =
            (_frameData[_currentFrame][_currentLayerColoring][8*i+0] > _currentColor)<<0 |
            (_frameData[_currentFrame][_currentLayerColoring][8*i+1] > _currentColor)<<1 |
            (_frameData[_currentFrame][_currentLayerColoring][8*i+2] > _currentColor)<<2 |
            (_frameData[_currentFrame][_currentLayerColoring][8*i+3] > _currentColor)<<3 |
            (_frameData[_currentFrame][_currentLayerColoring][8*i+4] > _currentColor)<<4 |
            (_frameData[_currentFrame][_currentLayerColoring][8*i+5] > _currentColor)<<5 |
            (_frameData[_currentFrame][_currentLayerColoring][8*i+6] > _currentColor)<<6 |
            (_frameData[_currentFrame][_currentLayerColoring][8*i+7] > _currentColor)<<7;
        }

        _colorCommandQueue.PopFront();
    }
}
void Ledcube::WriteThread() {
    while(!_runWriteThread.load()) {}
    LOG("Writing thread started" << std::boolalpha << _runWriteThread.load())
    while(_runWriteThread.load()) {
        _currentBuffer++;
        _currentBuffer %= LEDCUBE_LAYER_BUFFERS;
        _currentLayer++;
        _currentLayer %= 8;

        #ifdef LEDCUBE_FRAME_PROFILING
        if(!_colorCommandQueue.Contains(_currentBuffer)) _awaitingColorQueue++;
        #endif
        _colorCommandQueue.AwaitProcessed(_currentBuffer);

        /*dmSPI.Send<1>(&_layerData[_currentBuffer][0]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][1]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][2]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][3]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][4]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][5]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][6]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][7]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][8]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][9]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][10]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][11]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][12]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][13]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][14]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][15]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][16]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][17]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][18]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][19]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][20]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][21]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][22]);
        GPIO::Write(DM_ENABLE, true);
        dmSPI.Send<1>(&_layerData[_currentBuffer][23]);
        GPIO::Write(DM_ENABLE, true);*/
        dmSPI.Send<1>(_layerData[_currentBuffer]);
        //GPIO::Write(DM_ENABLE, true);
        GPIO::Write(DM_LATCH, true);
        GPIO::Write(DM_LATCH, false);
        const uint8_t dataOn[1] = {0xFF ^ (1 << _currentLayer)};
        laSPI.Send<1>(dataOn);
        GPIO::Write(LA_LATCH, true);
        //GPIO::Write(DM_ENABLE, false);
        GPIO::Write(LA_LATCH, false);

        _colorCommandQueue.PushBack(_currentBuffer);

        #ifdef LEDCUBE_FRAME_PROFILING
        _amountFrames++;
        const auto now = std::chrono::steady_clock::now();
        const auto frameDuration = now - _profilingStartTime;
        if(frameDuration.count() < _shortesFrameTime) _shortesFrameTime = frameDuration.count();
        if(frameDuration.count() > _longestFrameTime) _longestFrameTime = frameDuration.count();
        _profilingStartTime = now;

        if(_endTime < std::chrono::steady_clock::now()) {
            LOG("FPS: " + std::to_string(_amountFrames) + " - Longest frame: " + std::to_string(_longestFrameTime) + " - Shortest frame: " + std::to_string(_shortesFrameTime) + " - waited color queue: " + std::to_string(_awaitingColorQueue));
            _amountFrames = 0;
            _endTime = std::chrono::steady_clock::now() + std::chrono::seconds(1);
            _longestFrameTime = 0;
            _shortesFrameTime = INT32_MAX;
            _awaitingColorQueue = 0;
        }
        #endif
    }
}

void Ledcube::SetPixel(const uint8_t x, const uint8_t y, const uint8_t z, const uint8_t rgb, const uint8_t value) {
    switch(y<<5 | x<<2 | rgb) {
        case 0b00000000 /* 0 0 r */: _frameData[_currentStagingFrameBuffer][z][16*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00000001 /* 0 0 g */: _frameData[_currentStagingFrameBuffer][z][16*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00000010 /* 0 0 b */: _frameData[_currentStagingFrameBuffer][z][16*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00000100 /* 0 1 r */: _frameData[_currentStagingFrameBuffer][z][17*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00000101 /* 0 1 g */: _frameData[_currentStagingFrameBuffer][z][17*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00000110 /* 0 1 b */: _frameData[_currentStagingFrameBuffer][z][17*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00001000 /* 0 2 r */: _frameData[_currentStagingFrameBuffer][z][18*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00001001 /* 0 2 g */: _frameData[_currentStagingFrameBuffer][z][18*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00001010 /* 0 2 b */: _frameData[_currentStagingFrameBuffer][z][18*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00001100 /* 0 3 r */: _frameData[_currentStagingFrameBuffer][z][19*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00001101 /* 0 3 g */: _frameData[_currentStagingFrameBuffer][z][19*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00001110 /* 0 3 b */: _frameData[_currentStagingFrameBuffer][z][19*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00010000 /* 0 4 r */: _frameData[_currentStagingFrameBuffer][z][20*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00010001 /* 0 4 g */: _frameData[_currentStagingFrameBuffer][z][20*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00010010 /* 0 4 b */: _frameData[_currentStagingFrameBuffer][z][20*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00010100 /* 0 5 r */: _frameData[_currentStagingFrameBuffer][z][21*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00010101 /* 0 5 g */: _frameData[_currentStagingFrameBuffer][z][21*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00010110 /* 0 5 b */: _frameData[_currentStagingFrameBuffer][z][21*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00011000 /* 0 6 r */: _frameData[_currentStagingFrameBuffer][z][22*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00011001 /* 0 6 g */: _frameData[_currentStagingFrameBuffer][z][22*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00011010 /* 0 6 b */: _frameData[_currentStagingFrameBuffer][z][22*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00011100 /* 0 7 r */: _frameData[_currentStagingFrameBuffer][z][23*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00011101 /* 0 7 g */: _frameData[_currentStagingFrameBuffer][z][23*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00011110 /* 0 7 b */: _frameData[_currentStagingFrameBuffer][z][23*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;

        case 0b00100000 /* 1 0 r */: _frameData[_currentStagingFrameBuffer][z][16*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00100001 /* 1 0 g */: _frameData[_currentStagingFrameBuffer][z][16*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00100010 /* 1 0 b */: _frameData[_currentStagingFrameBuffer][z][16*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00100100 /* 1 1 r */: _frameData[_currentStagingFrameBuffer][z][17*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00100101 /* 1 1 g */: _frameData[_currentStagingFrameBuffer][z][17*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00100110 /* 1 1 b */: _frameData[_currentStagingFrameBuffer][z][17*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00101000 /* 1 2 r */: _frameData[_currentStagingFrameBuffer][z][18*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00101001 /* 1 2 g */: _frameData[_currentStagingFrameBuffer][z][18*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00101010 /* 1 2 b */: _frameData[_currentStagingFrameBuffer][z][18*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00101100 /* 1 3 r */: _frameData[_currentStagingFrameBuffer][z][19*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00101101 /* 1 3 g */: _frameData[_currentStagingFrameBuffer][z][19*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00101110 /* 1 3 b */: _frameData[_currentStagingFrameBuffer][z][19*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00110000 /* 1 4 r */: _frameData[_currentStagingFrameBuffer][z][20*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00110001 /* 1 4 g */: _frameData[_currentStagingFrameBuffer][z][20*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;// weird
        case 0b00110010 /* 1 4 b */: _frameData[_currentStagingFrameBuffer][z][20*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;// weird
        case 0b00110100 /* 1 5 r */: _frameData[_currentStagingFrameBuffer][z][21*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00110101 /* 1 5 g */: _frameData[_currentStagingFrameBuffer][z][21*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00110110 /* 1 5 b */: _frameData[_currentStagingFrameBuffer][z][21*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00111000 /* 1 6 r */: _frameData[_currentStagingFrameBuffer][z][22*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00111001 /* 1 6 g */: _frameData[_currentStagingFrameBuffer][z][22*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00111010 /* 1 6 b */: _frameData[_currentStagingFrameBuffer][z][22*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00111100 /* 1 7 r */: _frameData[_currentStagingFrameBuffer][z][23*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00111101 /* 1 7 g */: _frameData[_currentStagingFrameBuffer][z][23*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b00111110 /* 1 7 b */: _frameData[_currentStagingFrameBuffer][z][23*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;

        case 0b01000000 /* 2 0 r */: _frameData[_currentStagingFrameBuffer][z][17*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01000001 /* 2 0 g */: _frameData[_currentStagingFrameBuffer][z][17*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01000010 /* 2 0 b */: _frameData[_currentStagingFrameBuffer][z][16*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01000100 /* 2 1 r */: _frameData[_currentStagingFrameBuffer][z][10*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01000101 /* 2 1 g */: _frameData[_currentStagingFrameBuffer][z][16*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01000110 /* 2 1 b */: _frameData[_currentStagingFrameBuffer][z][18*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01001000 /* 2 2 r */: _frameData[_currentStagingFrameBuffer][z][19*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01001001 /* 2 2 g */: _frameData[_currentStagingFrameBuffer][z][19*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01001010 /* 2 2 b */: _frameData[_currentStagingFrameBuffer][z][18*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01001100 /* 2 3 r */: _frameData[_currentStagingFrameBuffer][z][10*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01001101 /* 2 3 g */: _frameData[_currentStagingFrameBuffer][z][10*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01001110 /* 2 3 b */: _frameData[_currentStagingFrameBuffer][z][10*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01010000 /* 2 4 r */: _frameData[_currentStagingFrameBuffer][z][10*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01010001 /* 2 4 g */: _frameData[_currentStagingFrameBuffer][z][10*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01010010 /* 2 4 b */: _frameData[_currentStagingFrameBuffer][z][10*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01010100 /* 2 5 r */: _frameData[_currentStagingFrameBuffer][z][20*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01010101 /* 2 5 g */: _frameData[_currentStagingFrameBuffer][z][21*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01010110 /* 2 5 b */: _frameData[_currentStagingFrameBuffer][z][21*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01011000 /* 2 6 r */: _frameData[_currentStagingFrameBuffer][z][8 *8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01011001 /* 2 6 g */: _frameData[_currentStagingFrameBuffer][z][20*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01011010 /* 2 6 b */: _frameData[_currentStagingFrameBuffer][z][22*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01011100 /* 2 7 r */: _frameData[_currentStagingFrameBuffer][z][22*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01011101 /* 2 7 g */: _frameData[_currentStagingFrameBuffer][z][23*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01011110 /* 2 7 b */: _frameData[_currentStagingFrameBuffer][z][23*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;

        case 0b01100000 /* 3 0 r */: _frameData[_currentStagingFrameBuffer][z][14*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01100001 /* 3 0 g */: _frameData[_currentStagingFrameBuffer][z][14*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01100010 /* 3 0 b */: _frameData[_currentStagingFrameBuffer][z][14*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01100100 /* 3 1 r */: _frameData[_currentStagingFrameBuffer][z][14*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01100101 /* 3 1 g */: _frameData[_currentStagingFrameBuffer][z][14*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01100110 /* 3 1 b */: _frameData[_currentStagingFrameBuffer][z][14*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01101000 /* 3 2 r */: _frameData[_currentStagingFrameBuffer][z][11*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01101001 /* 3 2 g */: _frameData[_currentStagingFrameBuffer][z][11*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01101010 /* 3 2 b */: _frameData[_currentStagingFrameBuffer][z][10*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01101100 /* 3 3 r */: _frameData[_currentStagingFrameBuffer][z][11*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01101101 /* 3 3 g */: _frameData[_currentStagingFrameBuffer][z][11*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01101110 /* 3 3 b */: _frameData[_currentStagingFrameBuffer][z][11*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01110000 /* 3 4 r */: _frameData[_currentStagingFrameBuffer][z][11*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01110001 /* 3 4 g */: _frameData[_currentStagingFrameBuffer][z][11*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01110010 /* 3 4 b */: _frameData[_currentStagingFrameBuffer][z][11*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01110100 /* 3 5 r */: _frameData[_currentStagingFrameBuffer][z][9*8 +7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01110101 /* 3 5 g */: _frameData[_currentStagingFrameBuffer][z][9*8 +6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01110110 /* 3 5 b */: _frameData[_currentStagingFrameBuffer][z][8*8 +0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01111000 /* 3 6 r */: _frameData[_currentStagingFrameBuffer][z][8*8 +1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01111001 /* 3 6 g */: _frameData[_currentStagingFrameBuffer][z][8*8 +2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01111010 /* 3 6 b */: _frameData[_currentStagingFrameBuffer][z][8*8 +3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01111100 /* 3 7 r */: _frameData[_currentStagingFrameBuffer][z][8*8 +6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01111101 /* 3 7 g */: _frameData[_currentStagingFrameBuffer][z][8*8 +5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b01111110 /* 3 7 b */: _frameData[_currentStagingFrameBuffer][z][8*8 +4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;

        case 0b10000000 /* 4 0 r */: _frameData[_currentStagingFrameBuffer][z][15*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10000001 /* 4 0 g */: _frameData[_currentStagingFrameBuffer][z][15*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10000010 /* 4 0 b */: _frameData[_currentStagingFrameBuffer][z][14*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10000100 /* 4 1 r */: _frameData[_currentStagingFrameBuffer][z][15*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10000101 /* 4 1 g */: _frameData[_currentStagingFrameBuffer][z][15*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10000110 /* 4 1 b */: _frameData[_currentStagingFrameBuffer][z][15*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10001000 /* 4 2 r */: _frameData[_currentStagingFrameBuffer][z][15*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10001001 /* 4 2 g */: _frameData[_currentStagingFrameBuffer][z][15*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10001010 /* 4 2 b */: _frameData[_currentStagingFrameBuffer][z][15*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10001100 /* 4 3 r */: _frameData[_currentStagingFrameBuffer][z][13*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10001101 /* 4 3 g */: _frameData[_currentStagingFrameBuffer][z][13*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10001110 /* 4 3 b */: _frameData[_currentStagingFrameBuffer][z][12*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10010000 /* 4 4 r */: _frameData[_currentStagingFrameBuffer][z][12*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10010001 /* 4 4 g */: _frameData[_currentStagingFrameBuffer][z][12*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10010010 /* 4 4 b */: _frameData[_currentStagingFrameBuffer][z][12*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10010100 /* 4 5 r */: _frameData[_currentStagingFrameBuffer][z][12*8+6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10010101 /* 4 5 g */: _frameData[_currentStagingFrameBuffer][z][12*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10010110 /* 4 5 b */: _frameData[_currentStagingFrameBuffer][z][12*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10011000 /* 4 6 r */: _frameData[_currentStagingFrameBuffer][z][9*8 +3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10011001 /* 4 6 g */: _frameData[_currentStagingFrameBuffer][z][9*8 +4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10011010 /* 4 6 b */: _frameData[_currentStagingFrameBuffer][z][9*8 +5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10011100 /* 4 7 r */: _frameData[_currentStagingFrameBuffer][z][9*8 +2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10011101 /* 4 7 g */: _frameData[_currentStagingFrameBuffer][z][9*8 +1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10011110 /* 4 7 b */: _frameData[_currentStagingFrameBuffer][z][9*8 +0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;

        case 0b10100000 /* 5 0 r */: _frameData[_currentStagingFrameBuffer][z][0*8 +7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10100001 /* 5 0 g */: _frameData[_currentStagingFrameBuffer][z][2*8 +7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10100010 /* 5 0 b */: _frameData[_currentStagingFrameBuffer][z][14*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10100100 /* 5 1 r */: _frameData[_currentStagingFrameBuffer][z][0*8 +0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10100101 /* 5 1 g */: _frameData[_currentStagingFrameBuffer][z][1*8 +7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10100110 /* 5 1 b */: _frameData[_currentStagingFrameBuffer][z][1*8 +6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10101000 /* 5 2 r */: _frameData[_currentStagingFrameBuffer][z][3*8 +6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10101001 /* 5 2 g */: _frameData[_currentStagingFrameBuffer][z][3*8 +7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10101010 /* 5 2 b */: _frameData[_currentStagingFrameBuffer][z][2*8 +0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10101100 /* 5 3 r */: _frameData[_currentStagingFrameBuffer][z][13*8+3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10101101 /* 5 3 g */: _frameData[_currentStagingFrameBuffer][z][13*8+4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10101110 /* 5 3 b */: _frameData[_currentStagingFrameBuffer][z][13*8+5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10110000 /* 5 4 r */: _frameData[_currentStagingFrameBuffer][z][13*8+2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10110001 /* 5 4 g */: _frameData[_currentStagingFrameBuffer][z][13*8+1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10110010 /* 5 4 b */: _frameData[_currentStagingFrameBuffer][z][13*8+0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10110100 /* 5 5 r */: _frameData[_currentStagingFrameBuffer][z][4*8 +0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10110101 /* 5 5 g */: _frameData[_currentStagingFrameBuffer][z][5*8 +7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10110110 /* 5 5 b */: _frameData[_currentStagingFrameBuffer][z][5*8 +6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10111000 /* 5 6 r */: _frameData[_currentStagingFrameBuffer][z][7*8 +6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10111001 /* 5 6 g */: _frameData[_currentStagingFrameBuffer][z][7*8 +7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10111010 /* 5 6 b */: _frameData[_currentStagingFrameBuffer][z][6*8 +0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10111100 /* 5 7 r */: _frameData[_currentStagingFrameBuffer][z][4*8 +7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10111101 /* 5 7 g */: _frameData[_currentStagingFrameBuffer][z][6*8 +7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b10111110 /* 5 7 b */: _frameData[_currentStagingFrameBuffer][z][12*8+7] = value>>(8-LEDCUBE_COLOR_PRECISION); break;

        case 0b11000000 /* 6 0 r */: _frameData[_currentStagingFrameBuffer][z][1*8 +3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11000001 /* 6 0 g */: _frameData[_currentStagingFrameBuffer][z][1*8 +4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11000010 /* 6 0 b */: _frameData[_currentStagingFrameBuffer][z][1*8 +5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11000100 /* 6 1 r */: _frameData[_currentStagingFrameBuffer][z][0*8 +3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11000101 /* 6 1 g */: _frameData[_currentStagingFrameBuffer][z][0*8 +2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11000110 /* 6 1 b */: _frameData[_currentStagingFrameBuffer][z][0*8 +1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11001000 /* 6 2 r */: _frameData[_currentStagingFrameBuffer][z][3*8 +3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11001001 /* 6 2 g */: _frameData[_currentStagingFrameBuffer][z][3*8 +4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11001010 /* 6 2 b */: _frameData[_currentStagingFrameBuffer][z][3*8 +5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11001100 /* 6 3 r */: _frameData[_currentStagingFrameBuffer][z][2*8 +3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11001101 /* 6 3 g */: _frameData[_currentStagingFrameBuffer][z][2*8 +2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11001110 /* 6 3 b */: _frameData[_currentStagingFrameBuffer][z][2*8 +1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11010000 /* 6 4 r */: _frameData[_currentStagingFrameBuffer][z][5*8 +3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11010001 /* 6 4 g */: _frameData[_currentStagingFrameBuffer][z][5*8 +4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11010010 /* 6 4 b */: _frameData[_currentStagingFrameBuffer][z][5*8 +5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11010100 /* 6 5 r */: _frameData[_currentStagingFrameBuffer][z][4*8 +3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11010101 /* 6 5 g */: _frameData[_currentStagingFrameBuffer][z][4*8 +2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11010110 /* 6 5 b */: _frameData[_currentStagingFrameBuffer][z][4*8 +1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11011000 /* 6 6 r */: _frameData[_currentStagingFrameBuffer][z][7*8 +3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11011001 /* 6 6 g */: _frameData[_currentStagingFrameBuffer][z][7*8 +4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11011010 /* 6 6 b */: _frameData[_currentStagingFrameBuffer][z][7*8 +5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11011100 /* 6 7 r */: _frameData[_currentStagingFrameBuffer][z][6*8 +3] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11011101 /* 6 7 g */: _frameData[_currentStagingFrameBuffer][z][6*8 +2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11011110 /* 6 7 b */: _frameData[_currentStagingFrameBuffer][z][6*8 +1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;

        case 0b11100000 /* 7 0 r */: _frameData[_currentStagingFrameBuffer][z][1*8 +0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11100001 /* 7 0 g */: _frameData[_currentStagingFrameBuffer][z][1*8 +1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11100010 /* 7 0 b */: _frameData[_currentStagingFrameBuffer][z][1*8 +2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11100100 /* 7 1 r */: _frameData[_currentStagingFrameBuffer][z][0*8 +6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11100101 /* 7 1 g */: _frameData[_currentStagingFrameBuffer][z][0*8 +5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11100110 /* 7 1 b */: _frameData[_currentStagingFrameBuffer][z][0*8 +4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11101000 /* 7 2 r */: _frameData[_currentStagingFrameBuffer][z][3*8 +0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11101001 /* 7 2 g */: _frameData[_currentStagingFrameBuffer][z][3*8 +1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11101010 /* 7 2 b */: _frameData[_currentStagingFrameBuffer][z][3*8 +2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11101100 /* 7 3 r */: _frameData[_currentStagingFrameBuffer][z][2*8 +6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11101101 /* 7 3 g */: _frameData[_currentStagingFrameBuffer][z][2*8 +5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11101110 /* 7 3 b */: _frameData[_currentStagingFrameBuffer][z][2*8 +4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11110000 /* 7 4 r */: _frameData[_currentStagingFrameBuffer][z][5*8 +0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11110001 /* 7 4 g */: _frameData[_currentStagingFrameBuffer][z][5*8 +1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11110010 /* 7 4 b */: _frameData[_currentStagingFrameBuffer][z][5*8 +2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11110100 /* 7 5 r */: _frameData[_currentStagingFrameBuffer][z][4*8 +6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11110101 /* 7 5 g */: _frameData[_currentStagingFrameBuffer][z][4*8 +5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11110110 /* 7 5 b */: _frameData[_currentStagingFrameBuffer][z][4*8 +4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11111000 /* 7 6 r */: _frameData[_currentStagingFrameBuffer][z][7*8 +0] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11111001 /* 7 6 g */: _frameData[_currentStagingFrameBuffer][z][7*8 +1] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11111010 /* 7 6 b */: _frameData[_currentStagingFrameBuffer][z][7*8 +2] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11111100 /* 7 7 r */: _frameData[_currentStagingFrameBuffer][z][6*8 +6] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11111101 /* 7 7 g */: _frameData[_currentStagingFrameBuffer][z][6*8 +5] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
        case 0b11111110 /* 7 7 b */: _frameData[_currentStagingFrameBuffer][z][6*8 +4] = value>>(8-LEDCUBE_COLOR_PRECISION); break;
    }
}