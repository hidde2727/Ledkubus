#include "SPI.h"

int SPI::_connection;
int SPI::_maxID;
int SPI::_currentID;
int SPI::_openConnections = 0;
uint8_t SPI::_currentMode = -1;
uint8_t SPI::_currentBits = -1;
uint32_t SPI::_currentSpeed = 0;
uint16_t SPI::_currentDelay = -1;

SPI::SPI(const uint32_t speed, const uint8_t bits, const uint8_t mode, const uint8_t delay) : 
_speed(speed), _bits(bits), _mode(mode), _delay(delay)
{
    _id = _maxID;
    _maxID++;
    _currentID = _id;

    _openConnections++;

    if(_maxID == 1) {
        _connection = open(SPI_DEVICE, O_WRONLY);
        LOG("Created connection: " + std::to_string(_connection))
        if (_connection < 0) {
            EXCEPTION("Failed to open SPI device"); return;
        }
        int lsbFirst = 0;
        if (ioctl(_connection, SPI_IOC_WR_LSB_FIRST, &lsbFirst) < 0){// || ioctl(_connection, SPI_IOC_RD_LSB_FIRST, &_mode) < 0) {
            close(_connection);
            perror("Failed to set SPI lsb first: ");
            EXCEPTION("Failed to set SPI lsb first");
            return;
        }
    }
}

SPI::~SPI() {
    _openConnections--;
    if(_openConnections == 0) close(_connection);
}