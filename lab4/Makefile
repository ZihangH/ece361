#the compiler: gcc for C program, define as g++ for C++
CC = gcc

#compiler flags:
#-g adds debugging information to the executable file
#-Wall turns on most, but not all, compiler warnings
CFLAGS = -g	-Wall  -lm	-pthread

#the build target executable:
TARGET = server	client

all:$(TARGET)

client:client.c client.h
	$(CC) $(CFLAGS) -o client client.c

server:server.c server.h lookup_table.c lookup_table.h
	$(CC) $(CFLAGS) -o server server.c lookup_table.c

clean:
	$(RM) $(TARGET) *~
