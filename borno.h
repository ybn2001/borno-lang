/*
 * borno.h — Core data structures for the Borno programming language.
 *
 * Borno (বর্ণ) is a small dynamically-typed language with
 * Bengali-transliterated keywords, implemented with Flex + Bison
 * as a tree-walking interpreter.
 */

#ifndef BORNO_H
#define BORNO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- Runtime values ---------- */

typedef enum { V_NUM, V_STR } ValueType;

typedef struct {
    ValueType type;
    double    num;   /* valid when type == V_NUM */
    char     *str;   /* valid when type == V_STR (heap-allocated) */
} Value;

Value make_num(double n);
Value make_str(const char *s);
Value copy_value(Value v);
void  free_value(Value v);
int   value_truthy(Value v);

/* ---------- AST ---------- */

typedef enum {
    /* expressions */
    N_NUM, N_STR, N_VAR,
    N_ADD, N_SUB, N_MUL, N_DIV, N_MOD, N_POW,
    N_LT, N_GT, N_LE, N_GE, N_EQ, N_NE,
    N_AND, N_OR, N_NOT, N_NEG,
    N_INPUT,           /* nao(prompt?)      */
    N_SQRT, N_ABS, N_LEN,  /* built-in helpers  */
    /* statements */
    N_ASSIGN,          /* name = expr       */
    N_PRINT,           /* dekhao(args...)   */
    N_IF,              /* jodi / nahole     */
    N_WHILE,           /* jotokkhon         */
    N_FOR,             /* chokro            */
    N_SEQ              /* statement list    */
} NodeKind;

typedef struct Node {
    NodeKind kind;
    double        num;     /* N_NUM literal            */
    char         *str;     /* N_STR literal / N_VAR or N_ASSIGN name */
    struct Node  *a, *b, *c, *d;  /* children (operands, branches)   */
} Node;

Node *node0(NodeKind k);
Node *node1(NodeKind k, Node *a);
Node *node2(NodeKind k, Node *a, Node *b);
Node *node3(NodeKind k, Node *a, Node *b, Node *c);
Node *node4(NodeKind k, Node *a, Node *b, Node *c, Node *d);
Node *num_node(double n);
Node *str_node(char *s);
Node *var_node(char *name);
Node *assign_node(char *name, Node *expr);
Node *arg_append(Node *list, Node *expr);

void exec(Node *n);        /* execute a statement (sub)tree */
Value eval(Node *n);       /* evaluate an expression        */
void free_node(Node *n);

/* ---------- Symbol table ---------- */

void  sym_set(const char *name, Value v);
Value sym_get(const char *name);

/* ---------- Error handling ---------- */

void runtime_error(const char *fmt, ...);

extern int yylineno;
int yylex(void);
void yyerror(const char *s);

#endif
