# Uncomment to statically link with musl
#CC      = musl-gcc
#LDFLAGS = -lm -static -s
#CFLAGS  = -D_GNU_SOURCE -std=c11 -pedantic -Ofast \

CC      = gcc
LDFLAGS = -lm -s
CFLAGS  = -D_GNU_SOURCE -std=c11 -pedantic -O3

#LDFLAGS = -lm
#CFLAGS  = -D_GNU_SOURCE -g -static -std=c11 -Wpedantic -Wall -Wextra \
          -Wbad-function-cast -Wcast-align -Wcast-qual -Wduplicated-branches \
		  -Wfloat-equal -Wformat=2 -Wformat-truncation=2 \
		  -Wimplicit-fallthrough=4 -Winline -Wlogical-op -Wmaybe-uninitialized \
		  -Wmissing-prototypes -Wnested-externs -Wno-missing-braces \
		  -Wno-missing-field-initializers -Wold-style-definition \
		  -Wpointer-arith -Wredundant-decls -Wrestrict -Wshadow \
		  -Wstrict-aliasing=2 -Wstrict-overflow=5 -Wstrict-prototypes \
		  -Wswitch-default -Wswitch-enum -Wundef -Wuninitialized \
		  -Wunreachable-code -Wunused-macros -O0 \
		  -Werror
