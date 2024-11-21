CC = gcc
CFLAGS = -pthread -Wall
TARGET = program

$(TARGET): gpio.c thread_functions.o main.o
	$(CC) -o $(TARGET) gpio.c thread_functions.o main.o $(CFLAGS)

main.o: main.c railway_system.h
	$(CC) $(CFLAGS) -c main.c

setup_functions.o: gpio.c railway_system.h
	$(CC) $(CFLAGS) -c setup_functions.c

thread_functions.o: thread_functions.c railway_system.h
	$(CC) $(CFLAGS) -c thread_functions.c

clean:
	rm -f *.o $(TARGET)