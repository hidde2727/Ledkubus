// https://elinux.org/RPi_GPIO_Code_Samples#Direct_register_access
// https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf
#include "PCH.h"

#define RASPBERRY_BASE 0xFE000000
#define GPIO_BASE 0xFE000000 + 0x200000

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

class GPIO {
public:

    static void Setup();
    // GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
    static inline void Input(unsigned char pin) {
        *(_gpio+((pin)/10)) &= ~(7<<(((pin)%10)*3));
        usleep(1000);
    }
    static inline void Output(unsigned char pin) {
        Input(pin);
         *(_gpio+((pin)/10)) |= (1<<(((pin)%10)*3));
         usleep(1000);
    }
    static inline void SetAlt(unsigned char pin, unsigned char alt) {
        *(_gpio+(((pin)/10))) |= (((alt)<=3?(alt)+4:(alt)==4?3:2)<<(((pin)%10)*3));
        usleep(1000);
    }

    static inline void Write(uint64_t pin, bool value) {
        if(value) Write1(pin);
        else Write0(pin);
    }
    static inline void Write1(uint32_t pin) {
        if(pin < 32) *(_gpio+7) = 1 << pin;
        else *(_gpio+8) = 1 << pin;
    }
    static inline void Write0(uint32_t pin) {
        if(pin < 32) *(_gpio+10) = 1 << pin;
        else *(_gpio+11) = 1 << (pin-32);
    }

    static inline bool Get(unsigned char pin) {
        if(pin<32) return *(_gpio+13)&(1<<pin);
        else return *(_gpio+14)&(1<<(pin-32));
    }

    static inline void GPIO_PULL(unsigned char pin) {
        *(_gpio+37); // Pull up/pull down
    }
    static inline void GPIO_PULLCLK0(unsigned char pin) {
        *(_gpio+38);// Pull up/pull down clock
    }

private:
    static void* _gpioMap;
    static volatile unsigned* _gpio;
};