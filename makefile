.PHONY: all rpcgen server client clean
all: server client

rpcgen:
	@cd src/rpcgen && make --no-print-directory -f makefile.rpc

server:
	@cd src/server && make --no-print-directory -f makefile.server

client:
	@cd src/client && make --no-print-directory -f makefile.client

clean:
	@cd src/rpcgen && make --no-print-directory -f makefile.rpc clean
	@cd src/server && make --no-print-directory -f makefile.server clean
	@cd src/client && make --no-print-directory -f makefile.client clean
