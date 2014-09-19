CC = arm-linux-gcc 

led: Tiny6410_ledapp.o
	$(CC) -o led  -O0 $^
%.o:%.c
	$(CC) -c $^ -o $@
clean:
	rm -rf *.o *~ led
	
