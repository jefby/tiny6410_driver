CC = arm-linux-gcc 

led: Tiny6410_ledapp.o
	$(CC) -o led $^
%.o:%.c
	$(CC) -c $^ -o $@
clean:
	rm -rf *.o *~ led
	
