all:
	@cd rpcgen && make --no-print-directory -f makefile.rpc
	@cd server && make --no-print-directory -f makefile.server

clean:
	@cd rpcgen && make --no-print-directory -f makefile.rpc clean
	@cd server && make --no-print-directory -f makefile.server clean
