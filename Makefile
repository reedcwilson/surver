
CC= 						gcc $(CFLAGS)

TCP_SRC=				$(wildcard tcp/*.c)
TCP= 						$(addprefix obj/,$(notdir $(TCP_SRC:.c=.o)))

OBJS= 					$(TCP)

LIBS= -pthread

CFLAGS= -g

main.exe:$(TCP)
	$(CC) -o $@ $^ $(TCP) $(LIBS)

obj/%.o: tcp/%.c
	$(CC) -c -o $@ $<

clean:
	rm -f $(OBJS) $(OBJS:.o=.d)

realclean:
	rm -f $(OBJS) $(OBJS:.o=.d) server
