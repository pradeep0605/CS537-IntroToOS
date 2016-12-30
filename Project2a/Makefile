mysh: linked_list.o mysh.o
	gcc linked_list.o mysh.o -o mysh; rm linked_list.o mysh.o

linked_list.o:
	gcc -Wall -Werror -c linked_list.c -o linked_list.o

mysh.o:
	gcc -Wall -Werror -c mysh.c -o mysh.o

clean:
	rm mysh;

debug:
	gcc -O -g -Wall -Werror mysh.c linked_list.c -o mysh
