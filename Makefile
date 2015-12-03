TARGET:=emu41

CC := gcc
LDFLAGS :=
INCLUDE := -Igccdos

#CFLAGS=-O2 -Wstrict-prototypes -Wmissing-prototypes $(INCLUDE)
CFLAGS=-O2 -Wno-unused-result $(INCLUDE)
SRC := $(wildcard *.c)
OBJ := $(patsubst %.c,%.o,$(SRC))
LIBS=

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -s $(OBJ) -o $@ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o *~ gccdos/*~

tar:
	tar cvzf $(TARGET).tgz ../emu41gcc/*

