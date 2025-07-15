#include "GPIO.h"

void* GPIO::_gpioMap = nullptr;
volatile unsigned int* GPIO::_gpio = nullptr;

void GPIO::Setup() {
    int _memoryFileDescriptor = 0;
    if ((_memoryFileDescriptor = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
        printf("can't open /dev/mem \n");
        exit(-1);
    }
    _gpioMap = mmap(
        NULL,             //Any adddress in our space will do
        BLOCK_SIZE,       //Map length
        PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
        MAP_SHARED,       //Shared with other processes
        _memoryFileDescriptor,           //File to map
        GPIO_BASE         //Offset to GPIO peripheral
    );
    close(_memoryFileDescriptor);
    if (_gpioMap == MAP_FAILED) {
        printf("mmap error %d\n", (long)_gpioMap);//errno also set!
        exit(-1);
    }
    _gpio = (volatile unsigned *)_gpioMap;
}