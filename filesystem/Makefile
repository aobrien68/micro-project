SOURCES_C=$(wildcard *.c)
OBJECTS_C=$(patsubst %.c, build/%.o, $(SOURCES_C))

target: build/myfs

build/myfs: $(OBJECTS_C)
	gcc -o $@ $^

build/%.o: %.c
	gcc -c -o $@ $<

clean:
	rm build/*