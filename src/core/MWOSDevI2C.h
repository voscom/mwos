#ifndef MWOS3_MWOSDevI2C_H
#define MWOS3_MWOSDevI2C_H
#include <Wire.h>

/***
 * Модуль работы с устройством I2C
 * требует библиотеку Wire
 * принцип, как в библиотеке  https://github.com/adafruit/RTClib.git
 */
class MWOSDevI2C  {
public:

    TwoWire *_wire;
    uint8_t _addr;

    MWOSDevI2C(TwoWire *wireInstance = &Wire) {
        _wire=wireInstance;
    }

    void write_register(uint8_t reg, uint8_t val) {
        uint8_t buffer[2] = {reg, val};
        writeI2C(buffer, 2);
    }

    uint8_t read_register(uint8_t reg) {
        uint8_t buffer[1];
        writeI2C(&reg, 1);
        readI2C(buffer, 1);
        return buffer[0];
    }

protected:
    /*!
        @brief  Convert a binary coded decimal value to binary. RTC stores
      time/date values as BCD.
        @param val BCD value
        @return Binary value
    */
    uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }
    /*!
        @brief  Convert a binary value to BCD format for the RTC registers
        @param val Binary value
        @return BCD value
    */
    uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }

    bool writeI2C(const uint8_t *buffer, size_t len, bool stop = true,
               const uint8_t *prefix_buffer = nullptr, size_t prefix_len = 0) {
        _wire->beginTransmission(_addr);
        // Write the prefix data (usually an address)
        if ((prefix_len != 0) && (prefix_buffer != nullptr)) {
            if (_wire->write(prefix_buffer, prefix_len) != prefix_len) {
                return false;
            }
        }
        // Write the data itself
        if (_wire->write(buffer, len) != len) {
            return false;
        }
        if (_wire->endTransmission(stop) == 0) {
            return true;
        } else {
            return false;
        }
    }

    bool _readI2C(uint8_t *buffer, size_t len, bool stop) {
        size_t recv = _wire->requestFrom((uint8_t) _addr, (uint8_t)len, (uint8_t)stop);
        if (recv != len) {
            // Not enough data available to fulfill our obligation!
            return false;
        }
        for (uint16_t i = 0; i < len; i++) {
            buffer[i] = _wire->read();
        }
        return true;
    }

    bool readI2C(uint8_t *buffer, size_t len, bool stop = true) {
        size_t pos = 0;
        while (pos < len) {
            size_t read_len =len - pos;
            bool read_stop = (pos < (len - read_len)) ? false : stop;
            if (!_readI2C(buffer + pos, read_len, read_stop))
                return false;
            pos += read_len;
        }
        return true;
    }


    bool write_then_read(const uint8_t *write_buffer,
                                             size_t write_len, uint8_t *read_buffer,
                                             size_t read_len, bool stop = false) {
        if (!writeI2C(write_buffer, write_len, stop)) {
            return false;
        }

        return readI2C(read_buffer, read_len);
    }


};


#endif //MWOS3_MWOSTIMEDS3231_H
