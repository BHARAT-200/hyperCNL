flags   = -g -O0 -Wall -std=c23
ldflags =
BIN     = hypercnl
OBJS    = hyperCNL.o lexer.o parser.o ast.o generate.o

.PHONY: all clean run install

# ── default target ────────────────────────────────────────────────────
all: $(BIN)

$(BIN): $(OBJS)
	cc $(flags) $^ -o $@ $(ldflags)

# ── object rules ──────────────────────────────────────────────────────
hyperCNL.o: hyperCNL.c hyperCNL.h
	cc $(flags) -c $<

lexer.o: lexer.c lexer.h
	cc $(flags) -c $<

parser.o: parser.c parser.h ast.h
	cc $(flags) -c $<

ast.o: ast.c ast.h
	cc $(flags) -c $<

generate.o: generate.c generate.h ast.h
	cc $(flags) -c $<

# ── convenience ───────────────────────────────────────────────────────

# Run against a demo file (create it first with `make demo`)
run: $(BIN) demo.hcnl
	./$(BIN) demo.hcnl

# Generate a demo source file so `make run` works out of the box
demo.hcnl:
	@printf '<program>\n    <var name="x" value="10" />\n    <print>Hello World</print>\n    <if condition="x > 5">\n        <print>Big</print>\n    </if>\n</program>\n' > demo.hcnl
	@echo "Created demo.hcnl"

# Install the binary to /usr/local/bin (requires sudo)
install: $(BIN)
	install -m 755 $(BIN) /usr/local/bin/$(BIN)
	@echo "Installed $(BIN) to /usr/local/bin"

# ── clean ─────────────────────────────────────────────────────────────
clean:
	rm -f $(OBJS) $(BIN) output.c output demo.hcnl
