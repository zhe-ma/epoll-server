# epoll-server

A network framework.

## Overview

- Use Epoll LT mode.

- The network I/O are in the same thread.

- Use thread pool to handle the business request message.

- Implement timer using hierarchy time wheel.

- Support master-worker process pattern.

- Integrate spdlog, jsoncpp and so on.

## Message format

- Message = Header + Body

- Header = BodyLength + MsgCode + Crc32.

| Filed | Body Length | Msg Code | CRC32 | Body |
| :---- | :-----: | :----: | :----: | :----: |
| Type | uint16 | uint16 | uint32 | byte |
| Size | 2 bytes | 2 bytes | 4 bytes | length |
| ByteOrder | Little Endian | Little Endian | Little Endian | bytes |

## Build

```bash
$ mkdir build

$ cd build

$ cmake -DCMAKE_BUILD_TYPE=debug ..

$ make -j4
```

## Run
```bash
$ ./build/src/app/Server
```

## Test
```bash
$ cd test
$ go  test  test/client -run ^TestMessage$ -count=1 -v
```
