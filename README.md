# Compiler
A compiler for a C like language to machine code. Uses DFA (deterministic finite automation) to tokenize code and stores in stack. Use simple maximal munch to define commands. I create a parse tree to store the tokens and presevere syntax. The parse trees are then converted to machine code using a tree stack.
