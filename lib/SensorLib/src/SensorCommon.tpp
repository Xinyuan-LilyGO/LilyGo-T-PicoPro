/**
 *
 * @license MIT License
 *
 * Copyright (c) 2022 lewis he
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @file      SensorCommon.tpp
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2022-10-16
 *
 */


#pragma once

#include "SensorLib.h"

typedef union  {
    struct {
        uint8_t low;
        uint8_t high;
    } bits;
    uint16_t full;
} RegData_t;

typedef int     (*iic_fptr_t)(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);
typedef void    (*gpio_write_fprt_t)(uint32_t gpio, uint8_t value);
typedef int     (*gpio_read_fprt_t)(uint32_t gpio);
typedef void    (*gpio_mode_fprt_t)(uint32_t gpio, uint8_t mode);
typedef void    (*delay_ms_fprt_t)(uint32_t ms);

template <class chipType>
class SensorCommon
{
public:
    ~SensorCommon()
    {
#if defined(ARDUINO)
        if (__spiSetting) {
            delete __spiSetting;
        }
#endif
    }

#if defined(ARDUINO)
    void setSpiSetting(uint32_t freq, uint8_t dataOrder = SPI_DATA_ORDER, uint8_t dataMode = SPI_MODE0)
    {
        __freq = freq;
        __dataOrder = dataOrder;
        __dataMode = dataMode;
    }

    bool begin(PLATFORM_WIRE_TYPE &w, uint8_t addr, int sda, int scl)
    {
        log_i("Using Arduino Wire interface.\n");
        if (__has_init)return thisChip().initImpl();
        __wire = &w;
        __sda = sda;
        __scl = scl;
#if defined(NRF52840_XXAA) || defined(NRF52832_XXAA)
        __wire->begin();
#elif defined(ARDUINO_ARCH_RP2040)
        __wire->end();
        __wire->setSDA(__sda);
        __wire->setSCL(__scl);
        __wire->begin();
#else
        __wire->begin(__sda, __scl);
#endif
        __addr = addr;
        __spi = NULL;
        __i2c_master_read = NULL;
        __i2c_master_write = NULL;
        __has_init = thisChip().initImpl();
        return __has_init;
    }

    bool begin(int cs, int mosi = -1, int miso = -1, int sck = -1,
               PLATFORM_SPI_TYPE &spi = SPI
              )
    {
        log_i("Using Arduino SPI interface.\n");
        if (__has_init)return thisChip().initImpl();
        __cs  = cs;
        __spi = &spi;
        pinMode(__cs, OUTPUT);
        digitalWrite(__cs, HIGH);
        __spiSetting = new  DEFAULT_SPISETTING;
        if (!__spiSetting) {
            return false;
        }
        if (mosi != -1 && miso != -1 && sck != -1) {
#if defined(NRF52840_XXAA) || defined(NRF52832_XXAA)
            __spi->begin();
#elif defined(ARDUINO_ARCH_RP2040)
            __spi->setSCK(sck);
            __spi->setRX(miso);
            __spi->setTX(mosi);
            __spi->begin();
#else
            __spi->begin(sck, miso, mosi);
#endif
        } else {
            __spi->begin();
        }
        __wire = NULL;
        __readMask = thisChip().getReadMaskImpl();
        __has_init = thisChip().initImpl();
        return __has_init;
    }

#elif defined(ESP_PLATFORM) && !defined(ARDUINO)

#if ((ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_SENSORLIB_ESP_IDF_NEW_API))
    // * Using the new API of esp-idf 5.x, you need to pass the I2C BUS handle,
    // * which is useful when the bus shares multiple devices.

    bool begin(i2c_master_bus_handle_t i2c_dev_bus_handle, uint8_t addr)
    {
        log_i("Using ESP-IDF Driver interface.\n");
        if (i2c_dev_bus_handle == NULL) return false;
        if (__has_init)return thisChip().initImpl();

        __i2c_master_read = NULL;
        __i2c_master_write = NULL;

        /*
        i2c_master_bus_config_t i2c_bus_config;
        memset(&i2c_bus_config, 0, sizeof(i2c_bus_config));
        i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
        i2c_bus_config.i2c_port = port_num;
        i2c_bus_config.scl_io_num = (gpio_num_t)__scl;
        i2c_bus_config.sda_io_num = (gpio_num_t)__sda;
        i2c_bus_config.glitch_ignore_cnt = 7;

        i2c_new_master_bus(&i2c_bus_config, &bus_handle);
        */
        bus_handle = i2c_dev_bus_handle;

        i2c_device_config_t i2c_dev_conf = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = SENSORLIB_I2C_MASTER_SEEED,
        };

        if (ESP_OK != i2c_master_bus_add_device(bus_handle,
                                                &i2c_dev_conf,
                                                &__i2c_device)) {
            return false;
        }
        __has_init = thisChip().initImpl();
        return __has_init;
    }

#else //ESP 4.X

    bool begin(i2c_port_t port_num, uint8_t addr, int sda, int scl)
    {
        __i2c_num = port_num;
        log_i("Using ESP-IDF Driver interface.\n");
        if (__has_init)return thisChip().initImpl();
        __sda = sda;
        __scl = scl;
        __addr = addr;
        __i2c_master_read = NULL;
        __i2c_master_write = NULL;

        i2c_config_t i2c_conf;
        memset(&i2c_conf, 0, sizeof(i2c_conf));
        i2c_conf.mode = I2C_MODE_MASTER;
        i2c_conf.sda_io_num = sda;
        i2c_conf.scl_io_num = scl;
        i2c_conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        i2c_conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        i2c_conf.master.clk_speed = SENSORLIB_I2C_MASTER_SEEED;

        /**
         * @brief Without checking whether the initialization is successful,
         * I2C may be initialized externally,
         * so just make sure there is an initialization here.
         */
        i2c_param_config(__i2c_num, &i2c_conf);
        i2c_driver_install(__i2c_num,
                           i2c_conf.mode,
                           SENSORLIB_I2C_MASTER_RX_BUF_DISABLE,
                           SENSORLIB_I2C_MASTER_TX_BUF_DISABLE, 0);
        __has_init = thisChip().initImpl();
        return __has_init;
    }
#endif //ESP 5.X

#endif


    bool begin(uint8_t addr, iic_fptr_t readRegCallback, iic_fptr_t writeRegCallback)
    {
        log_i("Using Custom interface.\n");
        if (__has_init)return thisChip().initImpl();
        __i2c_master_read = readRegCallback;
        __i2c_master_write = writeRegCallback;
        __addr = addr;
#if defined(ARDUINO)
        __spi = NULL;
#endif
        __has_init = thisChip().initImpl();
        return __has_init;
    }

    void setGpioWriteCallback(gpio_write_fprt_t cb)
    {
        __set_gpio_level = cb;
    }

    void setGpioReadCallback(gpio_read_fprt_t cb)
    {
        __get_gpio_level = cb;
    }

    void setGpioModeCallback(gpio_mode_fprt_t cb)
    {
        __set_gpio_mode = cb;
    }

    void setDelayCallback(delay_ms_fprt_t cb)
    {
        __delay_ms = cb;
    }

protected:

    inline void setGpioMode(uint32_t gpio, uint8_t mode)
    {
        if (__set_gpio_mode) {
            return  __set_gpio_mode(gpio, mode);
        }
#if defined(ARDUINO) || defined(ESP_PLATFORM)
        return pinMode(gpio, mode);
#endif
    }

    inline void setGpioLevel(uint32_t gpio, uint8_t level)
    {
        if (__set_gpio_level) {
            return  __set_gpio_level(gpio, level);
        }
#if defined(ARDUINO) || defined(ESP_PLATFORM)
        return digitalWrite(gpio, level);
#endif
    }

    inline int getGpioLevel(uint32_t gpio)
    {
        if (__get_gpio_level) {
            return  __get_gpio_level(gpio);
        }
#if defined(ARDUINO) || defined(ESP_PLATFORM)
        return digitalRead(gpio);
#endif
    }



    bool probe()
    {
#if defined(ARDUINO)
        if (__wire) {
            __wire->beginTransmission(__addr);
            return __wire->endTransmission() == 0;
        }
        return false;
#elif defined(ESP_PLATFORM)
#if ((ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_SENSORLIB_ESP_IDF_NEW_API))
        return i2c_master_probe(bus_handle, __addr, 1000);
#else
        return true;
#endif
#else
        return true;
#endif
    }

    //! Write method
    int writeRegister(uint8_t reg, uint8_t norVal, uint8_t orVal)
    {
        int val = readRegister(reg);
        if (val == DEV_WIRE_ERR) {
            return DEV_WIRE_ERR;
        }
        val &= norVal;
        val |= orVal;
        return writeRegister(reg, val);
    }

    int writeRegister(int reg, uint8_t val)
    {
        return writeRegister(reg, &val, 1);
    }

    int writeRegister(int reg, RegData_t data)
    {
        return writeRegister(reg, (uint8_t *)&data.full, 2);
    }

    int writeThenRead(uint8_t *write_buffer, uint8_t write_len, uint8_t *read_buffer, uint8_t read_len)
    {
#if defined(ARDUINO)
        if (__wire) {
            __wire->beginTransmission(__addr);
            __wire->write(write_buffer, write_len);
            if (__wire->endTransmission(__sendStop) != 0) {
                return DEV_WIRE_ERR;
            }
            __wire->requestFrom(__addr, read_len);
            return __wire->readBytes(read_buffer, read_len) == read_len ? DEV_WIRE_NONE : DEV_WIRE_ERR;
        }
#elif defined(ESP_PLATFORM)

#if ((ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_SENSORLIB_ESP_IDF_NEW_API))
        if (ESP_OK == i2c_master_transmit_receive(
                    __i2c_device,
                    write_buffer,
                    write_len,
                    read_buffer,
                    read_len,
                    -1)) {
            return DEV_WIRE_NONE;
        }
#else //ESP_IDF_VERSION
        if (ESP_OK == i2c_master_write_read_device(
                    __i2c_num,
                    __addr,
                    write_buffer,
                    write_len,
                    read_buffer,
                    read_len,
                    SENSORLIB_I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS)) {
            return DEV_WIRE_NONE;
        }
#endif //ESP_IDF_VERSION

#endif //ESP_PLATFORM
        return  DEV_WIRE_ERR;
    }

    int writeBuffer(uint8_t *buf, size_t length)
    {
#if defined(ARDUINO)
        if (__wire) {
            __wire->beginTransmission(__addr);
            __wire->write(buf, length);
            uint8_t ret = (__wire->endTransmission());
            return ret == 0 ? DEV_WIRE_NONE : DEV_WIRE_ERR;
        }
#elif defined(ESP_PLATFORM)

#if ((ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_SENSORLIB_ESP_IDF_NEW_API))
        if (ESP_OK == i2c_master_transmit(
                    __i2c_device,
                    buf,
                    length,
                    -1)) {
            return DEV_WIRE_NONE;
        }
#else //ESP_IDF_VERSION
        if (ESP_OK == i2c_master_write_to_device(__i2c_num,
                __addr,
                buf,
                length,
                SENSORLIB_I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS)) {
            return DEV_WIRE_NONE;
        }
#endif //ESP_IDF_VERSION

#endif //ESP_PLATFORM
        return  DEV_WIRE_ERR;
    }

    int writeRegister(int reg, uint8_t *buf, uint8_t length)
    {
        if (__i2c_master_write) {
            return __i2c_master_write(__addr, reg, buf, length);
        }
#if defined(ARDUINO)
        if (__wire) {
            __wire->beginTransmission(__addr);
            if (__reg_addr_len == 1) {
                __wire->write(reg);
            } else {
                for (int i = 0; i < __reg_addr_len; ++i) {
                    __wire->write(reg >> (8 * ((__reg_addr_len - 1) - i)));
                }
            }
            __wire->write(buf, length);
            return (__wire->endTransmission() == 0) ? DEV_WIRE_NONE : DEV_WIRE_ERR;
        }
        if (__spi) {
            __spi->beginTransaction(*__spiSetting);
            digitalWrite(__cs, LOW);
            if (__reg_addr_len == 1) {
                __spi->transfer(reg);
            } else {
                for (int i = 0; i < __reg_addr_len; ++i) {
                    __spi->transfer(reg >> (8 * ((__reg_addr_len - 1) - i)));
                }
            }
            __spi->transfer(buf, length);
            digitalWrite(__cs, HIGH);
            __spi->endTransaction();
            return DEV_WIRE_NONE;
        }
        return DEV_WIRE_ERR;

#elif defined(ESP_PLATFORM)
        uint8_t *write_buffer = (uint8_t *)malloc(sizeof(uint8_t) * (length + __reg_addr_len));
        if (!write_buffer) {
            return DEV_WIRE_ERR;
        }
        write_buffer[0] = reg;
        memcpy(write_buffer, &reg, __reg_addr_len);
        memcpy(write_buffer + __reg_addr_len, buf, length);
        int ret = writeBuffer(write_buffer, __reg_addr_len + length);
        free(write_buffer);
        return ret;
#endif //ESP_PLATFORM
    }

    //! Read method
    int readRegister(int reg)
    {
        uint8_t val = 0;
        return readRegister(reg, &val, 1) == -1 ? -1 : val;
    }

    int readRegister(int reg, RegData_t *data)
    {
        return readRegister(reg, (uint8_t *)data, 2);
    }

    int readRegister(int reg, uint8_t *buf, uint8_t length)
    {
        if (__i2c_master_read) {
            return __i2c_master_read(__addr, reg, buf, length);
        }
#if defined(ARDUINO)
        if (__wire) {
            __wire->beginTransmission(__addr);
            if (__reg_addr_len == 1) {
                __wire->write(reg);
            } else {
                for (int i = 0; i < __reg_addr_len; ++i) {
                    __wire->write(reg >> (8 * ((__reg_addr_len - 1) - i)));
                }
            }
            if (__wire->endTransmission(__sendStop) != 0) {
                return DEV_WIRE_ERR;
            }
            __wire->requestFrom(__addr, length);
            return __wire->readBytes(buf, length) == length ? DEV_WIRE_NONE : DEV_WIRE_ERR;
        }
        if (__spi) {
            __spi->beginTransaction(*__spiSetting);
            digitalWrite(__cs, LOW);
            if (__reg_addr_len == 1) {
                __spi->transfer(__readMask != -1 ? (reg  | __readMask) : reg);
            } else {
                uint8_t firstBytes = reg >> (8 * ((__reg_addr_len - 1)));
                __spi->transfer(__readMask != -1 ? (firstBytes  | __readMask) : firstBytes);
                for (int i = 1; i < __reg_addr_len; ++i) {
                    __spi->transfer(reg >> (8 * ((__reg_addr_len - 1) - i)));
                }
            }
            for (size_t i = 0; i < length; i++) {
                buf[i] = __spi->transfer(0x00);
            }
            digitalWrite(__cs, HIGH);
            __spi->endTransaction();
            return DEV_WIRE_NONE;
        }
#elif defined(ESP_PLATFORM)

#if ((ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_SENSORLIB_ESP_IDF_NEW_API))
        if (ESP_OK == i2c_master_transmit_receive(
                    __i2c_device,
                    (const uint8_t *)&reg,
                    __reg_addr_len,
                    buf,
                    length,
                    -1)) {
            return 0;
        }
#else //ESP_IDF_VERSION
        if (ESP_OK == i2c_master_write_read_device(__i2c_num,
                __addr,
                (uint8_t *)&reg,
                __reg_addr_len,
                buf,
                length,
                SENSORLIB_I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS)) {
            return DEV_WIRE_NONE;
        }
#endif //ESP_IDF_VERSION

#endif //ESP_PLATFORM
        return DEV_WIRE_ERR;
    }

    bool inline clrRegisterBit(int registers, uint8_t bit)
    {
        int val = readRegister(registers);
        if (val == DEV_WIRE_ERR) {
            return false;
        }
        return  writeRegister(registers, (val & (~_BV(bit)))) == 0;
    }

    bool inline setRegisterBit(int registers, uint8_t bit)
    {
        int val = readRegister(registers);
        if (val == DEV_WIRE_ERR) {
            return false;
        }
        return  writeRegister(registers, (val | (_BV(bit)))) == 0;
    }

    bool inline getRegisterBit(int registers, uint8_t bit)
    {
        int val = readRegister(registers);
        if (val == DEV_WIRE_ERR) {
            return false;
        }
        return val & _BV(bit);
    }

    uint16_t inline readRegisterH8L4(uint8_t highReg, uint8_t lowReg)
    {
        int h8 = readRegister(highReg);
        int l4 = readRegister(lowReg);
        if (h8 == DEV_WIRE_ERR || l4 == DEV_WIRE_ERR)return 0;
        return (h8 << 4) | (l4 & 0x0F);
    }

    uint16_t inline readRegisterH8L5(uint8_t highReg, uint8_t lowReg)
    {
        int h8 = readRegister(highReg);
        int l5 = readRegister(lowReg);
        if (h8 == DEV_WIRE_ERR || l5 == DEV_WIRE_ERR)return 0;
        return (h8 << 5) | (l5 & 0x1F);
    }

    uint16_t inline readRegisterH6L8(uint8_t highReg, uint8_t lowReg)
    {
        int h6 = readRegister(highReg);
        int l8 = readRegister(lowReg);
        if (h6 == DEV_WIRE_ERR || l8 == DEV_WIRE_ERR)return 0;
        return ((h6 & 0x3F) << 8) | l8;
    }

    uint16_t inline readRegisterH5L8(uint8_t highReg, uint8_t lowReg)
    {
        int h5 = readRegister(highReg);
        int l8 = readRegister(lowReg);
        if (h5 == DEV_WIRE_ERR || l8 == DEV_WIRE_ERR)return 0;
        return ((h5 & 0x1F) << 8) | l8;
    }

    void setRegAddressLenght(uint8_t len)
    {
        __reg_addr_len = len;
    }


    void setReadRegisterSendStop(bool sendStop)
    {
        __sendStop = sendStop;
    }



    /*
     * CRTP Helper
     */
protected:

//     bool begin()
//     {
// #if defined(ARDUINO)
//         if (__has_init) return thisChip().initImpl();
//         __has_init = true;

//         if (__wire) {
//             log_i("SDA:%d SCL:%d", __sda, __scl);
// #if defined(NRF52840_XXAA) || defined(NRF52832_XXAA)
//             __wire->begin();
// #elif defined(ARDUINO_ARCH_RP2040)
//             __wire->end();
//             __wire->setSDA(__sda);
//             __wire->setSCL(__scl);
//             __wire->begin();
// #else
//             __wire->begin(__sda, __scl);
// #endif
//         }
//         if (__spi) {
//             // int cs, int mosi = -1, int miso = -1, int sck = -1, SPIClass &spi = SPI
//             begin(__cs, __mosi, __miso, __sck, *__spi);
//         }

// #endif  /*ARDUINO*/
//         return thisChip().initImpl();
//     }

    void end()
    {
#if defined(ARDUINO)
        if (__wire) {
#if defined(ESP_IDF_VERSION)
#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4,4,0)
            __wire->end();
#endif  /*ESP_IDF_VERSION*/
#endif  /*ESP_IDF_VERSION*/
        }
#endif /*ARDUINO*/
    }


    inline const chipType &thisChip() const
    {
        return static_cast<const chipType &>(*this);
    }

    inline chipType &thisChip()
    {
        return static_cast<chipType &>(*this);
    }

protected:
    bool                __has_init              = false;
#if defined(ARDUINO)
    PLATFORM_WIRE_TYPE  *__wire                 = NULL;
    PLATFORM_SPI_TYPE   *__spi                  = NULL;
    SPISettings         *__spiSetting           = NULL;
    uint8_t             __dataOrder             = SPI_DATA_ORDER;
    uint8_t             __dataMode              = SPI_MODE0;
    uint32_t            __freq                  = 400000;

#elif defined(ESP_PLATFORM)
    i2c_port_t  __i2c_num;
#if ((ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_SENSORLIB_ESP_IDF_NEW_API))
    i2c_master_bus_handle_t  bus_handle;
    i2c_master_dev_handle_t  __i2c_device;
#endif //ESP_IDF_VERSION

#endif //ESP_PLATFORM

    int                 __readMask              = -1;
    int                 __sda                   = -1;
    int                 __scl                   = -1;
    int                 __cs                    = -1;
    int                 __miso                  = -1;
    int                 __mosi                  = -1;
    int                 __sck                   = -1;
    bool                __sendStop              = true;
    uint8_t             __addr                  = 0xFF;
    uint8_t             __reg_addr_len          = 1;
    iic_fptr_t          __i2c_master_read       = NULL;
    iic_fptr_t          __i2c_master_write      = NULL;
    gpio_write_fprt_t   __set_gpio_level        = NULL;
    gpio_read_fprt_t    __get_gpio_level        = NULL;
    gpio_mode_fprt_t    __set_gpio_mode         = NULL;
    delay_ms_fprt_t     __delay_ms              = NULL;

};
