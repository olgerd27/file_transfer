all:
	@cd src/rpcgen && make --no-print-directory -f makefile.rpc
	@cd src/server && make --no-print-directory -f makefile.server
	@cd src/client && make --no-print-directory -f makefile.client

clean:
	@cd src/rpcgen && make --no-print-directory -f makefile.rpc clean
	@cd src/server && make --no-print-directory -f makefile.server clean
	@cd src/client && make --no-print-directory -f makefile.client clean
