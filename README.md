# ESP8266-BetterDNS-Server
A Better DNS Server for the ESP8266


## How is it diffrent?
The original DNS Server by [bryceschober](https://github.com/bryceschober) only allows either one Domain to be resolved or all.
It dosen't for multiple name resolutions.

## Ok Cool. But how is this Useful?
Captive Portals. This new DNS server allows captive portals to work on android.

## How Do i use it?
It's not that diffrent from the Original.

The `start()` function now only takes an Integer, the Port.

You can add Domains using the `addDomain("host.domain.de",IPAddress(1,1,1,1))` function.

**Important! Add the wildcard `*` last!**
