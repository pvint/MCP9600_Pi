# MCP9600_Pi
## Read temperatures from an MCP9600 - Designed for Raspberry Pi but should function fine for other SBCs

Reference: [MCP9600/96L00 Datasheet](http://ww1.microchip.com/downloads/en/DeviceDoc/MCP9600-Data-Sheet-DS20005426D.pdf)

### Usage Example:
```
$ ./mcp9600 -a 0x65 -b 1 -A  -f 2 -d 200
25.19 23.50
25.19 23.50
25.12 23.38
25.25 23.44
25.25 23.44
25.25 23.44
```

### Options:
**-a** address (example: -a 0x65)

**-b** bus (Example, for /dev/i2c-1 use -b 1)

**-d** delay in ms (Example, to display temperature to STDOUT every 200 ms use -d 200  To display once and exit omit or set to 0.

**-f** filter value. Set the MCP9600's internal filter from 0 (none) to 7 (max)

**-t** thermocouple type. Set the thermocouple tpye K, J, T, N, S, E, B, or R (Default is K)

**-A** Display the ambient (cold junction) temperature

