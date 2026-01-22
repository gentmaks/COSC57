# ===== Project name (CHANGE THIS) =====
NAME = minic

# Tools
CC    = gcc
LEX   = flex
YACC  = yacc

# Source files
LEX_SRC  = $(NAME).l
YACC_SRC = $(NAME).y

# Bison outputs (Bison default naming)
YACC_C = y.tab.c
YACC_H = y.tab.h

# Flex output
LEX_C  = lex.yy.c

# Final executable
TARGET = $(NAME)

# Default target
all: $(TARGET)

# Link step
$(TARGET): $(LEX_C) $(YACC_C)
	$(CC) $(LEX_C) $(YACC_C) -o $(TARGET)

# Generate lexer (needs parser header!)
$(LEX_C): $(LEX_SRC) $(YACC_H)
	$(LEX) $(LEX_SRC)

# Generate parser and header
$(YACC_C) $(YACC_H): $(YACC_SRC)
	$(YACC) -d $(YACC_SRC)

# Cleanup
clean:
	rm -f $(TARGET) $(LEX_C) $(YACC_C) $(YACC_H)
