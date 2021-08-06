# pico-serprog

Slightly less terrible serprog implementation for the Raspberry Pi Pico and
possibly other RP2040 based boards. Based on pico-serprog by GitHub user
"stacksmashing".

Pinout for the SPI lines:
| Pin | Function |
|-----|----------|
| GP2 | CS       |
| GP3 | MISO     |
| GP4 | MOSI     |
| GP5 | SCK      |

## Example

Substitute ttyACMx with the actual tty device corresponding to the firmware.

Read chip:

```
flashrom -p serprog:dev=/dev/ttyACMx:1000000 -r flash.bin
```

Write chip:
```
flashrom -p serprog:dev=/dev/ttyACMx:1000000 -w flash.bin
```

## License

The project is based on the spi_flash example by Raspberry Pi (Trading) Ltd.
which is licensed under BSD-3-Clause.

As a lot of the code itself was heavily inspired/influenced by `stm32-vserprog`
this code is licensed under GPLv3.
