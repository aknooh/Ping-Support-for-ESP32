# Ping Support for ESP32

Ping is one of the most important and most useful tools in the networking world.
It comes in handy for debugging many types of problems, such as checking if a
sever is up or making sure that this device can reach another host across the
network, or checking if the device can reach the switch or the main access point
(AP). It is the first

At the moment, ESP32 and ESP8266 do not have a valid ping example. This aims to
change that. This repo adds a well tested ping console example that can be used
along with other applications.

## Requirements
Requires the latest
[ESP32 SDK](https://github.com/espressif/esp-idf)
## Installation

1) Change directory to `~/esp-idf/examples/system`
2) Clone this repo: `git clone
https://github.com/aknooh/Ping-Support-for-ESP32.git`
3) Source `install.sh`: `. ./install.sh`
4) Export variables from `~/esp-idf/` and compile

## Usage

```bash

esp32>
esp32> ping 192.168.1.1
PING 192.168.1.1 (192.168.1.1) 32(60) bytes of data.
60 bytes from 192.168.1.1: icmp_seq=1 time=13 ms
60 bytes from 192.168.1.1: icmp_seq=2 time=11 ms
60 bytes from 192.168.1.1: icmp_seq=3 time=7 ms
60 bytes from 192.168.1.1: icmp_seq=4 time=13 ms
60 bytes from 192.168.1.1: icmp_seq=5 time=25 ms

--- 192.168.1.1 ping statistics ---
5 packets transmitted, 5 received, 0% packet loss, time 69ms
rtt min/avg/max = 7/13.80/25 ms
esp32>

```
## Command Options

```bash

ping  <IP address>
Send an ICMP message to an IPv4/IPv6 address
<IPv4/IPv6>  Target IP Address
-c, --count=<n>  Number of messages
-t, --timeout=<t>  Connection timeout, ms
-d, --delay=<t>  Delay between messges, ms
-s, --size=<n>  Packet data size, bytes
--tos=<n>  Type of service

```

## Contributing
Pull requests are welcome. For major changes, please open
an issue first to discuss what you would like to change.

Please make sure to unit-test the changes before pushing.

## License
[MIT](https://choosealicense.com/licenses/mit/)
