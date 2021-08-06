/**
 * Copyright (C) 2021, Mate Kukri <km@mkukri.xyz>
 * Based on "pico-serprog" by Thomas Roth <code@stacksmashing.net>
 * 
 * Licensed under GPLv3
 *
 * Based on the spi_flash pico-example, which is:
 *  Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Also based on stm32-vserprog:
 *  https://github.com/dword1511/stm32-vserprog
 * 
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pio/pio_spi.h"
#include "spi.h"

#define PIN_CS      2
#define PIN_MISO    3
#define PIN_MOSI    4
#define PIN_SCK     5

#define S_SUPPORTED_BUS (1 << 3)    // BUS_SPI

#define S_CMD_MAP ( \
  (1 << S_CMD_NOP)       | \
  (1 << S_CMD_Q_IFACE)   | \
  (1 << S_CMD_Q_CMDMAP)  | \
  (1 << S_CMD_Q_PGMNAME) | \
  (1 << S_CMD_Q_SERBUF)  | \
  (1 << S_CMD_Q_BUSTYPE) | \
  (1 << S_CMD_SYNCNOP)   | \
  (1 << S_CMD_O_SPIOP)   | \
  (1 << S_CMD_S_BUSTYPE) | \
  (1 << S_CMD_S_SPI_FREQ)| \
  (1 << S_CMD_S_PIN_STATE) \
)


static inline void cs_select(uint cs_pin)
{
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop"); // FIXME
}

static inline void cs_deselect(uint cs_pin)
{
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop"); // FIXME
}

static inline void readbytes(uint8_t *b, uint32_t len)
{
    fread(b, len, 1, stdin);
}

static inline uint32_t getu24()
{
    uint8_t buf[3];
    fread(buf, sizeof buf, 1, stdin);
    return (uint32_t) buf[0]
            | ((uint32_t) buf[1] << 8)
            | ((uint32_t) buf[2] << 16);
}

static inline void sendbytes(uint8_t *b, uint32_t len)
{
    fwrite(b, len, 1, stdout);
}

static inline void process(pio_spi_inst_t *spi)
{
    static uint8_t buf[4096];

    for (;;) {
        switch(getchar()) {
        case S_CMD_NOP:
            putchar(S_ACK);
            break;
        case S_CMD_Q_IFACE:
            putchar(S_ACK);
            putchar(0x01);
            putchar(0x00);
            break;
        case S_CMD_Q_CMDMAP:
            putchar(S_ACK);
            memset(buf, 0, 32);
            *(uint32_t *) buf = S_CMD_MAP;
            sendbytes(buf, 32);
            break;
        case S_CMD_Q_PGMNAME:
            putchar(S_ACK);
            fwrite("pico-serprog\x0\x0\x0\x0\x0", 1, 16, stdout);
            break;
        case S_CMD_Q_SERBUF:
            putchar(S_ACK);
            putchar(0xFF);
            putchar(0xFF);
            break;
        case S_CMD_Q_BUSTYPE:
            putchar(S_ACK);
            putchar(S_SUPPORTED_BUS);
            break;
        case S_CMD_SYNCNOP:
            putchar(S_NAK);
            putchar(S_ACK);
            break;
        case S_CMD_S_BUSTYPE:
            {
                int bustype = getchar();
                if((bustype | S_SUPPORTED_BUS) == S_SUPPORTED_BUS) {
                    putchar(S_ACK);
                } else {
                    putchar(S_NAK);
                }
            }
            break;
        case S_CMD_O_SPIOP:
            {
                uint32_t wlen = getu24();
                uint32_t rlen = getu24();

                cs_select(PIN_CS);

                while (wlen) {
                    uint32_t cur = MIN(wlen, sizeof buf);
                    readbytes(buf, cur);
                    pio_spi_write8_blocking(spi, buf, cur);
                    wlen -= cur;
                }

                putchar(S_ACK);

                while (rlen) {
                    uint32_t cur = MIN(rlen, sizeof buf);
                    pio_spi_read8_blocking(spi, buf, cur);
                    sendbytes(buf, cur);
                    rlen -= cur;
                }

                cs_deselect(PIN_CS);
            }
            break;
        case S_CMD_S_SPI_FREQ:
            // TODO
            putchar(S_ACK);
            putchar(0x0);
            putchar(0x40);
            putchar(0x0);
            putchar(0x0);
            break;
        case S_CMD_S_PIN_STATE:
            //TODO:
            getchar();
            putchar(S_ACK);
            break;
        default:
            putchar(S_NAK);
        }
        fflush(stdout);
    }
}

int main() {
    stdio_init_all();
    stdio_set_translate_crlf(&stdio_usb, false);

    // Initialize CS
    gpio_init(PIN_CS);
    gpio_put(PIN_CS, 1);
    gpio_set_dir(PIN_CS, GPIO_OUT);

    // We use PIO 1
    pio_spi_inst_t spi = {
            .pio = pio1,
            .sm = 0,
            .cs_pin = PIN_CS
    };

    uint offset = pio_add_program(spi.pio, &spi_cpha0_program);

    pio_spi_init(spi.pio, spi.sm, offset,
                 8,       // 8 bits per SPI frame
                 2.5f,    // 12.5 MHz @ 125 clk_sys
                 false,   // CPHA = 0
                 false,   // CPOL = 0
                 PIN_SCK,
                 PIN_MOSI,
                 PIN_MISO);

    process(&spi);
    return 0;
}
