package client

import (
	"fmt"
	"net"
	"testing"
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
	msg := NewMessage(2020, data)
	if _, err = client.Write(msg.Pack()); err != nil {
		t.Error(err)
	}

	rsp := Message{}
	if err = rsp.Unpack(client); err != nil {
		t.Error(err)
	}

	fmt.Println(rsp)
}

func TestMessage2(t *testing.T) {
	client, err := net.Dial("tcp4", "127.0.0.1:9005")
	if err != nil {
		t.Error(err)
	}
	defer client.Close()

	data := []byte("Hello")
	msg := NewMessage(2020, data)
	data = []byte("World")
	msg2 := NewMessage(2020, data)
	buf := msg.Pack()
	buf = append(buf, msg2.Pack()...)

	if _, err = client.Write(buf); err != nil {
		t.Error(err)
	}

	rsp := Message{}
	if err = rsp.Unpack(client); err != nil {
		t.Error(err)
	}

	fmt.Println(rsp)

	if err = rsp.Unpack(client); err != nil {
		t.Error(err)
	}

	fmt.Println(rsp)
}
