package io

type Reader interface {
	Read(p []byte) (n int, err error)
}

type ReadWriter interface {
	Read(p []byte) (n int, err error)
	Write(p []byte) (n int, err error)
}
