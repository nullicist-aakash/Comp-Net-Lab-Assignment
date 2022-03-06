make:
	gcc server.c -o server.out
	gcc client.c -o client.out

clean:
	rm *.out
