make:
	mkdir bin
	gcc -c Trie.c -o bin/Trie.o
	gcc -c tcp_server.c -o bin/server.o
	gcc -o server.out bin/Trie.o bin/server.o
	rm -r bin

clean:
	rm -r bin
	rm server.out
