MAKEFLAGS += --no-print-directory

wireguard-go: $(wildcard *.go) $(wildcard */*.go)
	go build -v -ldflags "-w -s" -o "$@"

clean:
	rm -f wireguard-go

.PHONY: wireguard-go
