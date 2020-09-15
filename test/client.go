package main

import (
	"bytes"
	"encoding/binary"
	"net"
)

type Message struct {
	DataLen uint16
	Code    uint16
	Crc32   int32
	Data    []byte
}

func (msg *Message) Pack() []byte {
	dataBuff := bytes.NewBuffer([]byte{})

	binary.Write(dataBuff, binary.LittleEndian, msg.DataLen)
	binary.Write(dataBuff, binary.LittleEndian, msg.Code)
	binary.Write(dataBuff, binary.LittleEndian, msg.Crc32)
	binary.Write(dataBuff, binary.LittleEndian, msg.Data)

	return dataBuff.Bytes()
}

func main() {
	client, err := net.Dial("tcp4", "0.0.0.0:9005")
	if err != nil {
		panic(err)
	}
	defer client.Close()

	msg := Message{5, 12, 23, []byte("Hello")}
	client.Write(msg.Pack())

	msg2 := Message{6, 12121, -19191919, []byte("World!")}
	buf := msg.Pack()
	buf = append(buf, msg2.Pack()...)

	client.Write(buf)
}
