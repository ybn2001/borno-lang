/*
 * parser.y — Bison grammar for the Borno programming language.
 * Builds an AST from tokens; execution happens in interpreter.c.
 */

%{
#include "borno.h"

extern FILE *yyin;
Node *program_root = NULL;
%}

%union {
    double       num;
    char        *str;
    struct Node *node;
}

%token DHORI DEKHAO NAO JODI NAHOLE JOTOKKHON CHOKRO
%token AND OR NOT EQ NE LE GE
%token SQRT_FN ABS_FN LEN_FN
%token <num>  NUMBER
%token <str>  STRING IDENT
%token <node> BOOL_LIT

%type <node> program stmt_list stmt block simple_assign
%type <node> if_stmt else_part expr arg_list

/* Precedence: lowest first */
%left OR
%left AND
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%right '^'
%right NOT UMINUS

%start program

%%

program
    : stmt_list                       { program_root = $1; }
    ;

stmt_list
    : /* empty */                     { $$ = NULL; }
    | stmt_list stmt                  { $$ = $1 ? node2(N_SEQ, $1, $2) : $2; }
    ;

stmt
    : simple_assign ';'               { $$ = $1; }
    | DEKHAO '(' arg_list ')' ';'     { $$ = node1(N_PRINT, $3); }
    | if_stmt                         { $$ = $1; }
    | JOTOKKHON '(' expr ')' block    { $$ = node2(N_WHILE, $3, $5); }
    | CHOKRO '(' simple_assign ';' expr ';' simple_assign ')' block
                                      { $$ = node4(N_FOR, $3, $5, $7, $9); }
    ;

simple_assign
    : DHORI IDENT '=' expr            { $$ = assign_node($2, $4); }
    | IDENT '=' expr                  { $$ = assign_node($1, $3); }
    ;

if_stmt
    : JODI '(' expr ')' block else_part
                                      { $$ = node3(N_IF, $3, $5, $6); }
    ;

else_part
    : /* empty */                     { $$ = NULL; }
    | NAHOLE block                    { $$ = $2; }
    | NAHOLE if_stmt                  { $$ = $2; }   /* "nahole jodi" = elif */
    ;

block
    : '{' stmt_list '}'               { $$ = $2; }
    ;

arg_list
    : expr                            { $$ = node2(N_SEQ, $1, NULL); }
    | arg_list ',' expr               { $$ = arg_append($1, $3); }
    ;

expr
    : NUMBER                          { $$ = num_node($1); }
    | STRING                          { $$ = str_node($1); }
    | BOOL_LIT                        { $$ = $1; }
    | IDENT                           { $$ = var_node($1); }
    | NAO '(' ')'                     { $$ = node0(N_INPUT); }
    | NAO '(' expr ')'                { $$ = node1(N_INPUT, $3); }
    | SQRT_FN '(' expr ')'            { $$ = node1(N_SQRT, $3); }
    | ABS_FN '(' expr ')'             { $$ = node1(N_ABS, $3); }
    | LEN_FN '(' expr ')'             { $$ = node1(N_LEN, $3); }
    | expr '+' expr                   { $$ = node2(N_ADD, $1, $3); }
    | expr '-' expr                   { $$ = node2(N_SUB, $1, $3); }
    | expr '*' expr                   { $$ = node2(N_MUL, $1, $3); }
    | expr '/' expr                   { $$ = node2(N_DIV, $1, $3); }
    | expr '%' expr                   { $$ = node2(N_MOD, $1, $3); }
    | expr '^' expr                   { $$ = node2(N_POW, $1, $3); }
    | expr '<' expr                   { $$ = node2(N_LT, $1, $3); }
    | expr '>' expr                   { $$ = node2(N_GT, $1, $3); }
    | expr LE  expr                   { $$ = node2(N_LE, $1, $3); }
    | expr GE  expr                   { $$ = node2(N_GE, $1, $3); }
    | expr EQ  expr                   { $$ = node2(N_EQ, $1, $3); }
    | expr NE  expr                   { $$ = node2(N_NE, $1, $3); }
    | expr AND expr                   { $$ = node2(N_AND, $1, $3); }
    | expr OR  expr                   { $$ = node2(N_OR, $1, $3); }
    | NOT expr                        { $$ = node1(N_NOT, $2); }
    | '-' expr %prec UMINUS           { $$ = node1(N_NEG, $2); }
    | '(' expr ')'                    { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Borno: syntax error on line %d: %s\n", yylineno, s);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s program.brn\n", argv[0]);
        return 1;
    }
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        fprintf(stderr, "Borno: cannot open file '%s'\n", argv[1]);
        return 1;
    }
    if (yyparse() != 0) return 1;   /* syntax error already reported */
    exec(program_root);
    return 0;
}
