package client

import (
	"hash/crc32"
	"net"
	"testing"
	"time"
)

// go  test  test/client -run ^TestMessage$ -count=1 -v

func TestConnection(t *testing.T) {
	c, _ := net.Dial("tcp4", "127.0.0.1:9005")
	c1, _ := net.Dial("tcp4", "127.0.0.1:9005")
	c2, _ := net.Dial("tcp4", "127.0.0.1:9005")
	c3, _ := net.Dial("tcp4", "127.0.0.1:9005")

	c.Close()
	c1.Close()
	c2.Close()
	c3.Close()
}

func TestMessage(t *testing.T) {
	client, err := net.Dial("tcp4", "127.0.0.1:9005")
	if err != nil {
		t.Error(err)
	}
	defer client.Close()

	data := []byte("Hi")
	msg := Message{uint16(len(data)), 2020, crc32.ChecksumIEEE(data), data}
	if _, err = client.Write(msg.Pack()); err != nil {
		t.Error(err)
	}

	time.Sleep(time.Second * 10)
}

func TestMessage2(t *testing.T) {
	client, err := net.Dial("tcp4", "127.0.0.1:9005")
	if err != nil {
		t.Error(err)
	}
	defer client.Close()

	data := []byte("Hello")
	msg := Message{uint16(len(data)), 2020, crc32.ChecksumIEEE(data), data}
	data = []byte("World")
	msg2 := Message{uint16(len(data)), 2020, crc32.ChecksumIEEE(data), data}
	buf := msg.Pack()
	buf = append(buf, msg2.Pack()...)

	if _, err = client.Write(buf); err != nil {
		t.Error(err)
	}

	time.Sleep(time.Second * 10)
}
