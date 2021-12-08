build: main.c
	gcc -fdiagnostics-color=always -g main.c -o main -lssl -lcrypto -ljansson -lresolv
run: main
	./main