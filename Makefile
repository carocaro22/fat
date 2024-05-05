all:
	gcc fat.c functions/delete.c functions/list.c functions/list_dos.c functions/directoryTree.c functions/read.c functions/write.c -g -fsanitize=address -o fat

clean:
	rm -f fat
