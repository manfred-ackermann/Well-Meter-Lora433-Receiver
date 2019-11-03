# Well Meter Lora433 Receiver

## Abstract

Receives data packages via LoRa from the `Well Meter Lora433 Sender`
and sends a median value to thinkspeak.com.

## Cabeling

### LoRa Module XL1278-SMT to D1 Mini

| XL1278 | Color  | Di Mini |
| ------ | ------ | ------- |
| GND    | Black  | GND     |
| VCC    | Red    | 3.3v    |
| MISO   | Orange | D6      |
| MOSI   | Yellow | D7      |
| SLCK   | Green  | D5      |
| NSS    | Brown  | D3      |
| DIO0   | Blue   | D2      |
| REST   | Purple | D1      |

## Development

For development set the OS environment variable
`PLATFORMIO_BUILD_FLAGS` to your development environment
at e.g. `.bashrc` or `.zshrc`.

```
PLATFORMIO_BUILD_FLAGS='-DTHINGSPEAK_WELLMETER=\"YourChannelApiKey\"'
```