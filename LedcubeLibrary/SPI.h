#include "PCH.h"

#define SPI_DEVICE "/dev/spidev0.0"

class SPI {
public:
    SPI(const uint32_t speed = 100000, const uint8_t bits = 8, const uint8_t mode = 0, const uint8_t delay = 0);
    ~SPI();

    inline void ReInitConnection() {
        if(_mode != _currentMode) {
            if (ioctl(_connection, SPI_IOC_WR_MODE, &_mode) < 0){// || ioctl(_connection, SPI_IOC_RD_MODE, &_mode) < 0) {
                close(_connection);
                perror("Failed to set SPI mode: ");
                EXCEPTION("Failed to set SPI mode");
                return;
            }
            _currentMode = _mode;
        }
    
        if(_bits != _currentBits) {
            if (ioctl(_connection, SPI_IOC_WR_BITS_PER_WORD, &_bits) < 0){// || ioctl(_connection, SPI_IOC_RD_BITS_PER_WORD, &_bits) < 0) {
                close(_connection);
                EXCEPTION("Failed to set bits per word");
                return;
            }
            _currentBits = _bits;
        }
    
        if(_speed != _currentSpeed) {
            if (ioctl(_connection, SPI_IOC_WR_MAX_SPEED_HZ, &_speed) < 0){// || ioctl(_connection, SPI_IOC_RD_MAX_SPEED_HZ, &_speed) < 0) {
                close(_connection);
                EXCEPTION("Failed to set max speed");
                return;
            }
            _currentSpeed = _speed;
        }
    }

    template<std::size_t N>
    inline void Send(const uint8_t data[N]) noexcept {
        //LOG(_connection);
        ReInitConnection();
        struct spi_ioc_transfer transfer;
        memset(&transfer, 0, sizeof(transfer));
        transfer.tx_buf = (unsigned long)data;
        transfer.rx_buf = 0;
        transfer.len = N;
        transfer.delay_usecs = _delay;
        transfer.bits_per_word = _bits;
        transfer.cs_change = 1;
        //LOG("SPEED " << _speed << " Max SPEED " << _currentSpeed);
        transfer.speed_hz = _speed;

        if (ioctl(_connection, SPI_IOC_MESSAGE(1), &transfer) < 0) {
            close(_connection);
            perror("SPI transfer failed");
            EXCEPTION("Failed to transfer SPI message");
            return;
        }
    }

private:
    struct spiTransferStruct {
        spiTransferStruct(
            __u64		tx_buf = 0,
            __u64		rx_buf = 0,
            __u32		len = 0,
            __u32		speed_hz = 0,
            __u16		delay_usecs = 0,
            __u8		bits_per_word = 0
        ) : tx_buf(tx_buf), rx_buf(rx_buf), len(len), speed_hz(speed_hz), delay_usecs(delay_usecs), bits_per_word(bits_per_word) {}
        __u64		tx_buf = 0;
        __u64		rx_buf = 0;

        __u32		len = 0;
        __u32		speed_hz = 0;

        __u16		delay_usecs = 0;
        __u8		bits_per_word = 0;
        __u8		cs_change = 0;
        __u8		tx_nbits = 0;
        __u8		rx_nbits = 0;
        __u8		word_delay_usecs = 0;
        __u8		pad = 0;
    };

    static int _connection;
    static int _maxID;
    static int _currentID;
    static int _openConnections;
    static uint8_t _currentMode;
    static uint8_t _currentBits;
    static uint32_t _currentSpeed;
    static uint16_t _currentDelay;
    
    int _id = 0;

    uint8_t _mode = 0;
    uint8_t _bits = 8;
    uint32_t _speed = 1000000;
    uint16_t _delay = 0;

};