/**
 * Copyright (C) 2021, Mate Kukri <km@mkukri.xyz>
 * Based on "pico-serprog" by Thomas Roth <code@stacksmashing.net>
 * 
 * Licensed under GPLv3
 *
 * Also based on stm32-vserprog:
 *  https://github.com/dword1511/stm32-vserprog
 * 
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "serprog.h"

#define SPI_IF      spi0        // Which PL022 to use
#define SPI_BAUD    12000000    // Default baudrate (12 MHz)
#define SPI_CS      5
#define SPI_MISO    4
#define SPI_MOSI    3
#define SPI_SCK     2

static void enable_spi(uint baud)
{
    // Setup chip select GPIO
    gpio_init(SPI_CS);
    gpio_put(SPI_CS, 1);
    gpio_set_dir(SPI_CS, GPIO_OUT);

    // Setup PL022
    spi_init(SPI_IF, baud);
    gpio_set_function(SPI_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK,  GPIO_FUNC_SPI);
}

static void disable_spi()
{
    // Set all pins to SIO inputs
    gpio_init(SPI_CS);
    gpio_init(SPI_MISO);
    gpio_init(SPI_MOSI);
    gpio_init(SPI_SCK);

    // Disable all pulls
    gpio_set_pulls(SPI_CS, 0, 0);
    gpio_set_pulls(SPI_MISO, 0, 0);
    gpio_set_pulls(SPI_MOSI, 0, 0);
    gpio_set_pulls(SPI_SCK, 0, 0);

    // Disable SPI peripheral
    spi_deinit(SPI_IF);
}

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

static inline void readbytes(void *b, uint32_t len)
{
    fread(b, len, 1, stdin);
}

static inline void sendbytes(const void *b, uint32_t len)
{
    fwrite(b, len, 1, stdout);
}

static void command_loop(void)
{
    uint baud = spi_get_baudrate(SPI_IF);

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
            {
                static const uint32_t cmdmap[8] = {
                    (1 << S_CMD_NOP)       |
                      (1 << S_CMD_Q_IFACE)   |
                      (1 << S_CMD_Q_CMDMAP)  |
                      (1 << S_CMD_Q_PGMNAME) |
                      (1 << S_CMD_Q_SERBUF)  |
                      (1 << S_CMD_Q_BUSTYPE) |
                      (1 << S_CMD_SYNCNOP)   |
                      (1 << S_CMD_O_SPIOP)   |
                      (1 << S_CMD_S_BUSTYPE) |
                      (1 << S_CMD_S_SPI_FREQ)|
                      (1 << S_CMD_S_PIN_STATE)
                };

                putchar(S_ACK);
                sendbytes((uint8_t *) cmdmap, sizeof cmdmap);
                break;
            }
        case S_CMD_Q_PGMNAME:
            {
                static const char progname[16] = "pico-serprog";

                putchar(S_ACK);
                sendbytes(progname, sizeof progname);
                break;
            }
        case S_CMD_Q_SERBUF:
            putchar(S_ACK);
            putchar(0xFF);
            putchar(0xFF);
            break;
        case S_CMD_Q_BUSTYPE:
            putchar(S_ACK);
            putchar((1 << 3)); // BUS_SPI
            break;
        case S_CMD_SYNCNOP:
            putchar(S_NAK);
            putchar(S_ACK);
            break;
        case S_CMD_S_BUSTYPE:
            // If SPI is among the requsted bus types we succeed, fail otherwise
            if((uint8_t) getchar() & (1 << 3))
                putchar(S_ACK);
            else
                putchar(S_NAK);
            break;
        case S_CMD_O_SPIOP:
            {
                static uint8_t buf[4096];

                uint32_t wlen = 0;
                readbytes(&wlen, 3);
                uint32_t rlen = 0;
                readbytes(&rlen, 3);

                cs_select(SPI_CS);

                while (wlen) {
                    uint32_t cur = MIN(wlen, sizeof buf);
                    readbytes(buf, cur);
                    spi_write_blocking(SPI_IF, buf, cur);
                    wlen -= cur;
                }

                putchar(S_ACK);

                while (rlen) {
                    uint32_t cur = MIN(rlen, sizeof buf);
                    spi_read_blocking(SPI_IF, 0, buf, cur);
                    sendbytes(buf, cur);
                    rlen -= cur;
                }

                cs_deselect(SPI_CS);
            }
            break;
        case S_CMD_S_SPI_FREQ:
            {
                uint32_t want_baud;
                readbytes(&want_baud, 4);
                if (want_baud) {
                    // Set frequence
                    baud = spi_set_baudrate(SPI_IF, want_baud);
                    // Send back actual value
                    putchar(S_ACK);
                    sendbytes(&baud, 4);
                } else {
                    // 0 Hz is reserved
                    putchar(S_NAK);
                }
                break;
            }
        case S_CMD_S_PIN_STATE:
            if (getchar())
                enable_spi(baud);
            else
                disable_spi();
            putchar(S_ACK);
            break;
        default:
            putchar(S_NAK);
            break;
        }
        fflush(stdout);
    }
}

int main()
{
    stdio_init_all();
    stdio_set_translate_crlf(&stdio_usb, false);
    enable_spi(SPI_BAUD);
    command_loop();
}
