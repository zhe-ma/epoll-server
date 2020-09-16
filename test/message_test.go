package test

import (
	"net"
	"testing"
)

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

	msg := Message{5, 12, 23, []byte("Hello")}
	if _, err = client.Write(msg.Pack()); err != nil {
		t.Error(err)
	}
}

func TestMessage2(t *testing.T) {
	client, err := net.Dial("tcp4", "127.0.0.1:9005")
	if err != nil {
		t.Error(err)
	}
	defer client.Close()

	msg := Message{5, 12, 23, []byte("Hello")}
	msg2 := Message{6, 12121, -19191919, []byte("World!")}
	buf := msg.Pack()
	buf = append(buf, msg2.Pack()...)

	if _, err = client.Write(buf); err != nil {
		t.Error(err)
	}
}
