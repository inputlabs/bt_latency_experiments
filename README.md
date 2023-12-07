# bt_latency_experiments
Bluetooth latency experiments between two Raspberry Pi Picos

![Sequence](/sequence.png?raw=true "Sequence")

## Setup
- two pico-w
- connected via SPI cable

![Setup](/setup.jpg?raw=true "Setup")


## Build
- currently requires `develop` branch of BlueKitchen Bluetooth stack
-- `<path_to_pico_sdk>$ git submodule update --init`
-- `<path_to_pico_sdk>/lib/btstack$ git checkout develop`

- `cmake -DPICO_SDK_PATH=<path_to_pico_sdk_with_develop_branch_of_bluekitchen_bluetooth_stack> -DPICO_BOARD=pico_w ..`
- `make`
- `picotool load host/host.uf2`
- `picotool load client/client.uf2`

## Results
Delay ms | Average | Min. | Max. Latency ms
---|---|---|---
1000 | 15   | 3.5 | 27.4
100  | 15.5 | 3.5 | 27.5
10   | 13   | 3.1 | 25.5
5    | 7.2  | 3.1 | 24.5
1    | 3.5  | 3.1 | 3.9
