EXEC=projet
SOURCES=gestion_clavier.c  
OBJECTS=$(SOURCES:.c=.o)
CC=gcc
CFLAGS=-std=gnu99 -Wall -g
 
.PHONY: default clean
 
default: $(EXEC)

gestion_clavier.o: gestion_clavier.c 


 
%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(EXEC): $(OBJECTS)
	$(CC) -o $@ $^ -lm

clean:
	rm -rf $(EXEC) $(OBJECTS) $(SOURCES:.c=.c~) $(SOURCES:.c=.h~) Makefile~