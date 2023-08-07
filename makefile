compile:
	gcc -g -Wall -pedantic client.c -o client -pthread -lncursesw 
	gcc -g -Wall -pedantic server.c -o server -pthread -lncursesw -lm
	gcc -g -Wall -pedantic bot.c -o bot -pthread -lncursesw 
	
files = server bot client

.PHONY : clean

clean :
	rm -f $(files) *.o
