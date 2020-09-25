package client

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"hash/crc32"
	"io"
	"unsafe"
)

type Message struct {
	DataLen uint16
	Code    uint16
	Crc32   uint32
	Data    []byte
}

func (msg *Message) GetHeadLen() uint16 {
	len := unsafe.Sizeof(msg.DataLen) + unsafe.Sizeof(msg.Code) + unsafe.Sizeof(msg.Crc32)
	return uint16(len)
}

func (msg Message) String() string {
	return fmt.Sprintf("Len: %v, Code: %v, Crc32: %v, Data: %v", msg.DataLen, msg.Code, msg.Crc32, string(msg.Data))
}

func (msg *Message) Pack() []byte {
	dataBuff := bytes.NewBuffer([]byte{})

	binary.Write(dataBuff, binary.LittleEndian, msg.DataLen)
	binary.Write(dataBuff, binary.LittleEndian, msg.Code)
	binary.Write(dataBuff, binary.LittleEndian, msg.Crc32)
	binary.Write(dataBuff, binary.LittleEndian, msg.Data)

	return dataBuff.Bytes()
}

func (msg *Message) Unpack(conn io.Reader) error {
	headData := make([]byte, msg.GetHeadLen())
	if _, err := io.ReadFull(conn, headData); err != nil {
		return err
	}

	buff := bytes.NewReader(headData)
	binary.Read(buff, binary.LittleEndian, &msg.DataLen)
	binary.Read(buff, binary.LittleEndian, &msg.Code)
	binary.Read(buff, binary.LittleEndian, &msg.Crc32)

	msg.Data = make([]byte, msg.DataLen)
	if _, err := io.ReadFull(conn, msg.Data); err != nil {
		return err
	}

	crc32 := crc32.ChecksumIEEE(msg.Data)
	if crc32 != msg.Crc32 {
		return errors.New("CRC32 check failed")
	}

	return nil
}

func NewMessage(code uint16, data []byte) *Message {
	return &Message{
		DataLen: uint16(len(data)),
		Code:    code,
		Crc32:   crc32.ChecksumIEEE(data),
		Data:    data,
	}
}
