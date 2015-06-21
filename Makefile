include config.mk

SRC = bmpsss.c
SRC_DIR = src
BIN_DIR = bin
C_FILES = $(wildcard $(SRC_DIR)/*.c)

OBJ = $(addprefix $(SRC_DIR)/obj/, $(notdir $(C_FILES:.c=.o)))


$(SRC_DIR)/obj/%.o: $(SRC_DIR)/%.c
	mkdir -p $(SRC_DIR)/obj
	$(CC) -c -o $@ $< $(CFLAGS)

bmpsss.out: $(OBJ)
	mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)

options:
	@echo bmpsss build options:
	@echo "CC     = ${CC}"
	@echo "CFLAGS = ${CFLAGS}"

clean:
	@echo cleaning...
	rm -f $(BIN_DIR)/*
	rm -f -r $(SRC_DIR)/obj

.PHONY: options clean
