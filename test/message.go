package test

import (
	"bytes"
	"encoding/binary"
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
