# pico-serprog

Slightly less terrible serprog implementation for the Raspberry Pi Pico and
possibly other RP2040 based boards. Based on pico-serprog by GitHub user
"stacksmashing".

Pinout for the SPI lines:
| Pin | Function |
|-----|----------|
| GP5 | CS       |
| GP4 | MISO     |
| GP3 | MOSI     |
| GP2 | SCK      |

## Example

Substitute ttyACMx with the actual tty device corresponding to the firmware.

Read chip:

```
flashrom -p serprog:dev=/dev/ttyACMx -r flash.bin
```

Write chip:
```
flashrom -p serprog:dev=/dev/ttyACMx -w flash.bin
```

## License

As a lot of the code itself was heavily inspired/influenced by `stm32-vserprog`
this code is licensed under GPLv3.
