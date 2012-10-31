FILE_SERVER_OUTPUT='server'
FILE_CLIENT_OUTPUT='client'

default: compile

compile: clean
	gcc -ggdb -o $(FILE_SERVER_OUTPUT) server.c sockutils.c
	gcc -ggdb -o $(FILE_CLIENT_OUTPUT) client.c sockutils.c

clean:
	rm -rf *.o
	rm -rf $(FILE_SERVER_OUTPUT)
	rm -rf $(FILE_CLIENT_OUTPUT)
