SERVER=server
CLIENT=client
OBJS=dsc.o

CFLAGS=-Wall -O2
#LDFLAGS+=

all: $(SERVER) $(CLIENT)

$(SERVER): $(OBJS) $(SERVER).o
	$(CC) -o $@ $^ $(LDFLAGS)

$(CLIENT): $(OBJS) $(CLIENT).o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


.PHONY: clean
clean:
	$(RM) *.o *~ $(CLIENT) $(SERVER)
