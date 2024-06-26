## mavtools one-liner tricks

See live mavlink traffic, by connecting over TCP
(Most subsequent examples assume this)

```bash
netcat 10.41.1.1 5790 | mavdecode
```

Pretty print it

```bash
netcat 10.41.1.1 5790 | mavdecode | jq
```

Extract message names

```bash
netcat 10.41.1.1 5790 | mavdecode | jq '.name'
```

Measure message rates (over 10s)

```json
(timeout 10 netcat 10.41.1.1 5790; echo) | mavdecode | jq '.name' | sort | uniq -c
```

Only keep a specific messages

```bash
netcat 10.41.1.1 5790 | mavdecode | jq '.name' | grep 'ATTITUDE'
```

Extract single quantity from a message

```bash
timeout 10 netcat 10.41.1.1 5790 | mavdecode | grep "ALTITUDE" | jq '.fields.altitude_local'
```
