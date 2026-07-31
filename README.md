# Borno (বর্ণ)

Borno is a small programming language I made for my compiler course using Flex and Bison. The keywords are Bengali words written in English letters, and the file extension is `.brn`.

```
dhori naam = nao("Tomar naam ki? ");
dekhao("Shagotom,", naam + "!");
```

## Why the name "Borno"?

"Borno" (বর্ণ) means a letter of the alphabet in Bengali. I chose it for three reasons:

1. Every program, no matter how big, is just borno — plain characters arranged by rules. In fact the very first stage of my interpreter (the Flex lexer) literally reads the source file one character at a time and groups the characters into tokens. So the name describes exactly where the whole pipeline starts.
2. In Bangladesh, "bornomala" (the alphabet) is the first thing we ever learn as kids. Borno is meant the same way — a first, simple language for learning how programming languages actually work.
3. The keywords themselves are Bengali words (dhori, dekhao, jodi...), so a Bengali name for a Bengali-flavored language felt right.

## What it can do

The required stuff:

- Variables: `dhori x = 5;` to declare, `x = x + 1;` to change
- Arithmetic: `+ - * /` and comparisons `> < >= <= == !=`
- If/else: `jodi (...) { } nahole { }`
- Loops: `jotokkhon` (while) and `chokro` (for)
- Output with `dekhao(...)` and input with `nao("prompt")`

Extra things I added on top:

- Strings as a real type. `+` joins strings, and even joins a string with a number ("mark: " + 90 gives "mark: 90")
- `nao()` is smart — if you type 42 it gives you the number 42, if you type your name it gives you a string
- `%` (remainder) and `^` (power) operators
- Booleans `sotti` / `mittha` and logical `ebong` / `ba` / `na` with short-circuit
- else-if by chaining `nahole jodi`
- Small built-in functions: `borgomul(x)` for square root, `poromman(x)` for absolute value, `dorgho(s)` for string length
- Comments with `#` or `//`
- Error messages with line numbers instead of just crashing

## How I built it

There are three stages:

**1. Lexer (`lexer.l`)** — Flex reads the raw text and cuts it into tokens. So `dhori x = 10;` becomes DHORI, IDENT, '=', NUMBER, ';'. Comments and spaces get thrown away here. Escape sequences like \n inside strings are also handled here.

**2. Parser (`parser.y`)** — Bison takes the tokens and checks them against my grammar. But instead of running the code while parsing, every rule builds a node of a syntax tree (AST). Operator precedence is declared with %left and %right, so Bison handles `2 + 3 * 4` correctly without me writing ten separate grammar rules.

**3. Interpreter (`interpreter.c`)** — after parsing, the program is one big tree. `exec()` walks the statement nodes (assignments, prints, ifs, loops) and `eval()` computes expression nodes. Variables live in a linked-list symbol table. Each value is a struct with a type tag saying whether it's a number or a string.

## Problems I ran into

- At first I tried to execute code directly inside the Bison actions. That works for a calculator but breaks completely for loops, because the tokens are gone after one pass. That's when I understood why you need an AST — a loop just walks the same subtree again.
- `nahole jodi` (else-if) gave me a shift/reduce conflict in the beginning. The fix was letting the else part be either a block or another complete if statement.
- I had to decide what `"abc" + 5` means. I went with: if either side is a string, convert both to strings and join them. Otherwise it's normal addition.
- Strings are heap-allocated in C, so I had to be careful to free temporary values or the interpreter leaks memory in long loops.

## How to run it

 need flex, bison, gcc and make. On Ubuntu/Kali:

```bash
sudo apt install flex bison gcc make
make
./borno examples/01_hello.brn
```

The `examples/` folder has 5 programs that show every feature. `Language_Manual.txt` has the full syntax guide.
