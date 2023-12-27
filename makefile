all:
	@cd rpcgen && make --no-print-directory -f makefile.rpc

clean:
	@cd rpcgen && make --no-print-directory -f makefile.rpc clean
