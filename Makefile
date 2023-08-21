OBJECTS = obj/main.o obj/bmp.o obj/stego.o
HEADERS = include/bmp.h include/stego.h
OUTPUT = hw-01_bmp
FLAGS = -Wall -Werror -Wextra

$(OUTPUT): obj $(OBJECTS)
	gcc -I include -o $@ $(OBJECTS)
$(OBJECTS): obj/%.o  : src/%.c $(HEADERS) 
	gcc -I include -c  $< -o $@
obj : 
	mkdir -p obj
clean:
	rm -r obj
	rm $(OUTPUT)