# mavtools (mavencode / mavdecode)

Command line tools to encode / decode binary mavlink into a JSON format. 

## Installation

`mavencode` and `mavdecode` are binaries without any dependencies. 

You can download the binaries from the releases.

## Usage

For a collection of useful uses, see [mavtools one-liner tricks](oneliners.md).

### Decoding

```bash
mavdecode <binary mavlink capture file>
```

You can directly pipe that into jq for pretty-printing:

```bash
mavdecode <binary mavlink capture file> | jq
```

**Input from stdin**

This is useful to decode from a network stream (e.g. using `netcat`) or from a serial port.

```bash
cat <binary mavlink capture file> | mavdecode
```

**Using your own message set**

mavdecode comes with a built-in message set. You can however use your own message set by providing an xml file.

```bash
mavdecode --xml=<path to your xml> <binary mavlink capture file>
```


### Decoded format

The decoded format is a stream of JSON objects, delimited by newlines.

Example:
```json
{
  "id": 32,
  "name": "LOCAL_POSITION_NED",
  "system_id": 1,
  "component_id": 1,
  "seq": 0,
  "fields": {
    "time_boot_ms": 2714311,
    "vx": 20.1446,
    "vy": 23.6353,
    "vz": -0.0114382,
    "x": 12649,
    "y": 35699.4,
    "z": -7.52881
  }
}
{
  "id": 30,
  "name": "ATTITUDE",
  "system_id": 1,
  "component_id": 1,
  "seq": 0,
  "fields": {
    "pitch": 0.0056748,
    "pitchspeed": -0.00214324,
    "roll": -0.0038062,
    "rollspeed": 0.000281513,
    "time_boot_ms": 2714333,
    "yaw": 1.33996,
    "yawspeed": -0.000590359
  }
}
```

### Encoding

You can also encode JSON objects into binary mavlink. The tool uses the same JSON format as the decode tool.

```bash
mavencode <json file>
```

**Input from stdin**

```bash
cat <json file> | mavencode
```

**Using your own message set**

mavencode comes with a built-in message set. You can however use your own message set by providing an xml file.
mavencode can only encode messages that are in the provided xml file.
```bash
mavencode --xml=<path to your xml> <json file>
```

**Directing output to a file**

```bash
mavencode <json file> -o <output file>
```


*The `example.bin` file is taken from the [node-mavlink](https://github.com/ArduPilot/node-mavlink/blob/master/examples/mavlink-v2-3412-packets.bin) project*

**Consuming output from mavdecode**

You can use the output of `mavdecode` as input to `mavencode` to round-trip the data.

```bash
cat <binary mavlink capture file> | mavdecode | mavencode
```
