include config.mk

SRC = bmpsss.c
OBJ = ${SRC:.c=.o}

all: options bmpsss

options:
	@echo bmpsss build options:
	@echo "CC     = ${CC}"
	@echo "CFLAGS = ${CFLAGS}"

.c.o:
	@echo CC $<
	@${CC} -c -D_GNU_SOURCE ${CFLAGS} $<

${OBJ}: config.mk

bmpsss: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f bmpsss ${OBJ}

.PHONY: all options clean
