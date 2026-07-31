/*
 * lexer.l — Flex scanner for the Borno programming language.
 * Converts raw .brn source text into tokens for the Bison parser.
 */

%option noyywrap yylineno

%{
#include "borno.h"
#include "parser.tab.h"

/* Strip surrounding quotes and process \n \t \" \\ escapes. */
static char *unescape(const char *text) {
    size_t len = strlen(text);
    char *out = malloc(len);          /* result is always shorter */
    char *p = out;
    for (size_t i = 1; i + 1 < len; i++) {   /* skip opening/closing quote */
        if (text[i] == '\\' && i + 2 < len + 1) {
            i++;
            switch (text[i]) {
                case 'n': *p++ = '\n'; break;
                case 't': *p++ = '\t'; break;
                case '"': *p++ = '"';  break;
                case '\\': *p++ = '\\'; break;
                default:  *p++ = text[i];
            }
        } else {
            *p++ = text[i];
        }
    }
    *p = '\0';
    return out;
}
%}

DIGIT   [0-9]
ID      [a-zA-Z_][a-zA-Z0-9_]*

%%

"#".*                   { /* single-line comment */ }
"//".*                  { /* also allow C-style comments */ }
[ \t\r\n]+              { /* skip whitespace */ }

"dhori"                 { return DHORI; }      /* let      */
"dekhao"                { return DEKHAO; }     /* print    */
"nao"                   { return NAO; }        /* input    */
"jodi"                  { return JODI; }       /* if       */
"nahole"                { return NAHOLE; }     /* else     */
"jotokkhon"             { return JOTOKKHON; }  /* while    */
"chokro"                { return CHOKRO; }     /* for      */
"ebong"                 { return AND; }        /* and      */
"ba"                    { return OR; }         /* or       */
"na"                    { return NOT; }        /* not      */
"sotti"                 { yylval.node = num_node(1); return BOOL_LIT; }  /* true  */
"mittha"                { yylval.node = num_node(0); return BOOL_LIT; }  /* false */
"borgomul"              { return SQRT_FN; }    /* sqrt     */
"poromman"              { return ABS_FN; }     /* abs      */
"dorgho"                { return LEN_FN; }     /* length   */

{DIGIT}+(\.{DIGIT}+)?   { yylval.num = atof(yytext); return NUMBER; }
\"([^"\\]|\\.)*\"       { yylval.str = unescape(yytext); return STRING; }
{ID}                    { yylval.str = strdup(yytext); return IDENT; }

"=="                    { return EQ; }
"!="                    { return NE; }
"<="                    { return LE; }
">="                    { return GE; }
"<"                     { return '<'; }
">"                     { return '>'; }
"="                     { return '='; }
"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }
"%"                     { return '%'; }
"^"                     { return '^'; }
"("                     { return '('; }
")"                     { return ')'; }
"{"                     { return '{'; }
"}"                     { return '}'; }
";"                     { return ';'; }
","                     { return ','; }

.                       { fprintf(stderr, "Borno: unexpected character '%s' on line %d\n",
                                  yytext, yylineno); }

%%
