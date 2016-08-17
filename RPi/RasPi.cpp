// RasPi.cpp
//
// Routines for implementing RadioHead on Raspberry Pi
// using BCM2835 library for GPIO
//
// Contributed by Mike Poublon and used with permission


#if (RH_PLATFORM == RH_PLATFORM_RASPI)

#include "RasPi.h"

static pthread_mutex_t spiMutex;

/** Default SPI device */
string device;
/** SPI Mode set */
uint8_t mode;
/** word size*/
uint8_t bits;
/** Set SPI speed*/
uint32_t speed;
/* Bit order (LSB/MSB first)*/
uint8_t bitorder;

int fd = -1;

void SPIClass::begin() {
    //Set SPI Defaults
    uint32_t freq = 8000000;
    uint8_t bt_order = 0;
    uint8_t dt_mode = SPI_MODE_0;

    begin(freq, bt_order, dt_mode);
}

void SPIClass::begin(uint32_t _frequency, uint8_t _bitorder, uint8_t _dataMode) {

    // For our setup, hardcode this
    // We are using SPI1, CS0
    uint32_t busNo = 10;

    device = "/dev/spidev0.0";
    /* set spidev accordingly to busNo like:
     * busNo = 23 -> /dev/spidev2.3
     *
     * a bit messy but simple
     * */
    device[11] += (busNo / 10) % 10;
    device[13] += busNo % 10;
    bits = 8;

    int ret;

    if (fd < 0) // check whether spi is already open
    {
        fd = open(device.c_str(), O_RDWR);

        if (fd < 0) {
            perror("can't open device");
            abort();
        }
    }

    setDataMode(_dataMode);

    /*
     * bits per word
     */
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        perror("can't set bits per word");
        abort();
    }

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1) {
        perror("can't set bits per word");
        abort();
    }

    setFrequency(_frequency);

    setBitOrder(_bitorder);

}

void SPIClass::end() {

    if (!(fd < 0))
        close(fd);
}

void SPIClass::setBitOrder(uint8_t _bitOrder) {

    int ret = ioctl(fd, SPI_IOC_WR_LSB_FIRST, &_bitOrder);
    if (ret == -1) {
        perror("can't set bitorder");
        abort();
    }

    ret = ioctl(fd, SPI_IOC_RD_LSB_FIRST, &_bitOrder);
    if (ret == -1) {
        perror("can't set bitorder");
        abort();
    }
    bitorder = _bitOrder;
}

void SPIClass::setDataMode(uint8_t _mode) {
    int ret = ioctl(fd, SPI_IOC_WR_MODE, &_mode);
    if (ret == -1) {
        perror("can't set spi mode");
        abort();
    }

    ret = ioctl(fd, SPI_IOC_RD_MODE, &_mode);
    if (ret == -1) {
        perror("can't set spi mode");
        abort();
    }
    mode = _mode;
}

void SPIClass::setFrequency(uint32_t _frequency) {
    int ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &_frequency);
    if (ret == -1) {
        perror("can't set max speed hz");
        abort();
    }

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &_frequency);
    if (ret == -1) {
        perror("can't set max speed hz");
        abort();
    }
    speed = _frequency;
}

byte SPIClass::transfer(byte _data) {
    pthread_mutex_lock(&spiMutex);
    int ret;
    uint8_t tx[1] = {_data};
    uint8_t rx[1];

    struct spi_ioc_transfer tr = {
        tr.tx_buf = (unsigned long) &tx[0],
        tr.rx_buf = (unsigned long) &rx[0],
        tr.len = 1, //ARRAY_SIZE(tx),
        tr.delay_usecs = 0,
        tr.cs_change = 1,
    };

    tr.bits_per_word = bits;
    tr.speed_hz = speed;


    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) {
        pthread_mutex_unlock(&spiMutex);
        perror("can't send spi message");
        abort();
    }

    pthread_mutex_unlock(&spiMutex);
    return rx[0];
}

long random(long min, long max) {
    long diff = max - min;
    long ret = diff * rand() + min;
    return ret;
}

void SerialSimulator::begin(int baud) {
    //No implementation neccesary - Serial emulation on Linux = standard console
    //
}

size_t SerialSimulator::println(const char* s) {
    size_t sz = print(s);
    printf("\n");
    return sz;
}

size_t SerialSimulator::print(const char* s) {
    return printf(s);
}

size_t SerialSimulator::print(unsigned int n, int base) {
    size_t sz = 0;
    if (base == DEC)
        sz = printf("%d", n);
    else if (base == HEX)
        sz = printf("%02x", n);
    else if (base == OCT)
        sz = printf("%o", n);
    // TODO: BIN
    return sz;
}

size_t SerialSimulator::print(char ch) {
    return printf("%c", ch);
}

size_t SerialSimulator::println(char ch) {
    return printf("%c\n", ch);
}

size_t SerialSimulator::print(unsigned char ch, int base) {
    return print((unsigned int) ch, base);
}

size_t SerialSimulator::println(unsigned char ch, int base) {
    size_t sz = print((unsigned int) ch, base);
    printf("\n");
    return sz;
}

#endif
