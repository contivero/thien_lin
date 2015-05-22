include config.mk

SRC = cripto.c
OBJ = ${SRC:.c=.o}

all: options cripto

options:
	@echo cripto build options:
	@echo "CC       = ${CC}"
	@echo "CFLAGS   = ${CFLAGS}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

cripto: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f a.out ${OBJ}

.PHONY: all options clean
