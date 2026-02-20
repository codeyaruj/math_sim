# ── Makefile for math_sim ─────────────────────────────────────────────────────

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Werror -pedantic
TARGET  := math_sim

SRCS    := main.c lexer.c parser.c ast.c eval.c ir.c codegen.c cpu.c alu.c memory.c
OBJS    := $(SRCS:.c=.o)

# Default expression used by `make run`
EXPR    ?= "3 + 5 * 2"

# ── Targets ───────────────────────────────────────────────────────────────────

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Quick smoke test with a single expression
run: $(TARGET)
	@echo $(EXPR) | ./$(TARGET)

# Run all required test expressions
test: $(TARGET)
	@echo "===== 3+4 ====="
	@echo "3+4" | ./$(TARGET)
	@echo ""
	@echo "===== 3+4*2 ====="
	@echo "3+4*2" | ./$(TARGET)
	@echo ""
	@echo "===== (3+4)*2 ====="
	@echo "(3+4)*2" | ./$(TARGET)
	@echo ""
	@echo "===== (1+(2*3))+4 ====="
	@echo "(1+(2*3))+4" | ./$(TARGET)
	@echo ""
	@echo "===== 12 + 5 * 9 (with spaces) ====="
	@echo "  12   +   5 *  9" | ./$(TARGET)
	@echo ""
	@echo "===== 3 + * 4 (expect error) ====="
	@echo "3 + * 4" | ./$(TARGET); true
	@echo ""
	@echo "===== 10 / 0 (expect error) ====="
	@echo "10 / 0" | ./$(TARGET); true
	@echo ""
	@echo "===== flag: 0+0 (expect Z=1) ====="
	@echo "0+0" | ./$(TARGET)
	@echo ""
	@echo "===== flag: 2147483647+1 overflow (expect V=1 N=1) ====="
	@echo "2147483647+1" | ./$(TARGET)
	@echo ""
	@echo "===== flag: 0-1 borrow (expect N=1 C=0) ====="
	@echo "0-1" | ./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
