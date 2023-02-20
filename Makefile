### __PROJECT_MAKEFILE__ ###

CC = g++
# compiling in 64 bits
CFLAGS = -m64 -std=c++17 -Ofast
LDFLAGS = -m64 -L"./lib" -Ofast
EXEC = bin/output.exe

all : $(EXEC)

$(EXEC): main.o DCMatrix.o DCBuffer.o DCQueue.o UnaryTests.o
		$(CC) -o $(EXEC) $^ $(LDFLAGS)

main.o:	src/main.cpp
		$(CC) -c $< $(CFLAGS)

UnaryTests.o: src/UnaryTests.cpp
		$(CC) -c $< $(CFLAGS)

# DEEP COMPRESSION MODULE...
DCMatrix.o: src/DCompress/DCMatrix.cpp
		$(CC) -c $< $(CFLAGS)

DCBuffer.o: src/DCompress/DCBuffer.cpp
		$(CC) -c $< $(CFLAGS)

DCQueue.o: src/DCompress/DCQueue.cpp
		$(CC) -c $< $(CFLAGS)

#modules test rules ...

filter: main.o DCMatrix.o DCBuffer.o DCQueue.o
		$(CC) -o bin/filter.exe $^ $(LDFLAGS)

unfilter: main.o DCMatrix.o DCBuffer.o DCQueue.o
		$(CC) -o bin/unfilter.exe $^ $(LDFLAGS)

clean:
		del *.o

mrproper: clean 
		del -f $(EXEC)