/*
 * interpreter.c — Runtime for the Borno programming language.
 * Contains AST constructors, a symbol table, and a tree-walking evaluator.
 */

#include "borno.h"
#include <math.h>
#include <stdarg.h>

/* ================= Values ================= */

Value make_num(double n) {
    Value v; v.type = V_NUM; v.num = n; v.str = NULL; return v;
}

Value make_str(const char *s) {
    Value v; v.type = V_STR; v.num = 0; v.str = strdup(s); return v;
}

Value copy_value(Value v) {
    if (v.type == V_STR) return make_str(v.str);
    return v;
}

void free_value(Value v) {
    if (v.type == V_STR && v.str) free(v.str);
}

int value_truthy(Value v) {
    if (v.type == V_NUM) return v.num != 0;
    return v.str && v.str[0] != '\0';
}

/* ================= AST constructors ================= */

static Node *alloc_node(NodeKind k) {
    Node *n = calloc(1, sizeof(Node));
    n->kind = k;
    return n;
}

Node *node0(NodeKind k)                                 { return alloc_node(k); }
Node *node1(NodeKind k, Node *a)                        { Node *n = alloc_node(k); n->a = a; return n; }
Node *node2(NodeKind k, Node *a, Node *b)               { Node *n = node1(k, a); n->b = b; return n; }
Node *node3(NodeKind k, Node *a, Node *b, Node *c)      { Node *n = node2(k, a, b); n->c = c; return n; }
Node *node4(NodeKind k, Node *a, Node *b, Node *c, Node *d) { Node *n = node3(k, a, b, c); n->d = d; return n; }

Node *num_node(double v)      { Node *n = alloc_node(N_NUM); n->num = v; return n; }
Node *str_node(char *s)       { Node *n = alloc_node(N_STR); n->str = s; return n; }
Node *var_node(char *name)    { Node *n = alloc_node(N_VAR); n->str = name; return n; }

Node *assign_node(char *name, Node *expr) {
    Node *n = alloc_node(N_ASSIGN);
    n->str = name;
    n->a = expr;
    return n;
}

/* Append an expression to the end of an N_SEQ argument list. */
Node *arg_append(Node *list, Node *expr) {
    Node *tail = list;
    while (tail->b) tail = tail->b;
    tail->b = node2(N_SEQ, expr, NULL);
    return list;
}

void free_node(Node *n) {
    if (!n) return;
    free_node(n->a); free_node(n->b); free_node(n->c); free_node(n->d);
    if ((n->kind == N_STR || n->kind == N_VAR || n->kind == N_ASSIGN) && n->str)
        free(n->str);
    free(n);
}

/* ================= Symbol table ================= */

typedef struct Sym {
    char *name;
    Value value;
    struct Sym *next;
} Sym;

static Sym *symbols = NULL;

void sym_set(const char *name, Value v) {
    for (Sym *s = symbols; s; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            free_value(s->value);
            s->value = v;
            return;
        }
    }
    Sym *s = malloc(sizeof(Sym));
    s->name = strdup(name);
    s->value = v;
    s->next = symbols;
    symbols = s;
}

Value sym_get(const char *name) {
    for (Sym *s = symbols; s; s = s->next)
        if (strcmp(s->name, name) == 0)
            return copy_value(s->value);
    runtime_error("undefined variable '%s'", name);
    return make_num(0); /* unreachable */
}

/* ================= Errors ================= */

void runtime_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "Borno runtime error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

/* ================= Helpers ================= */

static void print_value(Value v) {
    if (v.type == V_STR) {
        fputs(v.str, stdout);
    } else if (v.num == (long long)v.num && fabs(v.num) < 1e15) {
        printf("%lld", (long long)v.num);   /* print whole numbers cleanly */
    } else {
        printf("%g", v.num);
    }
}

static double want_num(Value v, const char *op) {
    if (v.type != V_NUM)
        runtime_error("operator '%s' needs a number, got a string", op);
    return v.num;
}

static char *num_to_str(double n) {
    char buf[64];
    if (n == (long long)n && fabs(n) < 1e15)
        snprintf(buf, sizeof buf, "%lld", (long long)n);
    else
        snprintf(buf, sizeof buf, "%g", n);
    return strdup(buf);
}

/* '+' concatenates if either side is a string. */
static Value do_add(Value l, Value r) {
    if (l.type == V_NUM && r.type == V_NUM)
        return make_num(l.num + r.num);
    char *ls = (l.type == V_STR) ? strdup(l.str) : num_to_str(l.num);
    char *rs = (r.type == V_STR) ? strdup(r.str) : num_to_str(r.num);
    char *out = malloc(strlen(ls) + strlen(rs) + 1);
    strcpy(out, ls); strcat(out, rs);
    free(ls); free(rs);
    Value v; v.type = V_STR; v.num = 0; v.str = out;
    return v;
}

static int values_equal(Value l, Value r) {
    if (l.type != r.type) return 0;
    if (l.type == V_NUM) return l.num == r.num;
    return strcmp(l.str, r.str) == 0;
}

/* nao() — read one line; return a number if it parses, else a string. */
static Value do_input(Node *prompt) {
    if (prompt) {
        Value p = eval(prompt);
        print_value(p);
        fflush(stdout);
        free_value(p);
    }
    char buf[1024];
    if (!fgets(buf, sizeof buf, stdin)) return make_str("");
    buf[strcspn(buf, "\r\n")] = '\0';
    char *end;
    double n = strtod(buf, &end);
    if (end != buf && *end == '\0') return make_num(n);
    return make_str(buf);
}

/* ================= Evaluator ================= */

Value eval(Node *n) {
    if (!n) return make_num(0);
    switch (n->kind) {
        case N_NUM: return make_num(n->num);
        case N_STR: return make_str(n->str);
        case N_VAR: return sym_get(n->str);
        case N_INPUT: return do_input(n->a);

        case N_ADD: {
            Value l = eval(n->a), r = eval(n->b);
            Value out = do_add(l, r);
            free_value(l); free_value(r);
            return out;
        }
        case N_SUB: case N_MUL: case N_DIV: case N_MOD: case N_POW:
        case N_LT: case N_GT: case N_LE: case N_GE: {
            Value lv = eval(n->a), rv = eval(n->b);
            double l = want_num(lv, "arithmetic/comparison");
            double r = want_num(rv, "arithmetic/comparison");
            free_value(lv); free_value(rv);
            switch (n->kind) {
                case N_SUB: return make_num(l - r);
                case N_MUL: return make_num(l * r);
                case N_DIV:
                    if (r == 0) runtime_error("division by zero");
                    return make_num(l / r);
                case N_MOD:
                    if (r == 0) runtime_error("modulo by zero");
                    return make_num(fmod(l, r));
                case N_POW: return make_num(pow(l, r));
                case N_LT:  return make_num(l <  r);
                case N_GT:  return make_num(l >  r);
                case N_LE:  return make_num(l <= r);
                case N_GE:  return make_num(l >= r);
                default: break;
            }
            break;
        }
        case N_EQ: case N_NE: {
            Value l = eval(n->a), r = eval(n->b);
            int eq = values_equal(l, r);
            free_value(l); free_value(r);
            return make_num(n->kind == N_EQ ? eq : !eq);
        }
        case N_AND: {
            Value l = eval(n->a);
            int lt = value_truthy(l);
            free_value(l);
            if (!lt) return make_num(0);          /* short-circuit */
            Value r = eval(n->b);
            int rt = value_truthy(r);
            free_value(r);
            return make_num(rt);
        }
        case N_OR: {
            Value l = eval(n->a);
            int lt = value_truthy(l);
            free_value(l);
            if (lt) return make_num(1);           /* short-circuit */
            Value r = eval(n->b);
            int rt = value_truthy(r);
            free_value(r);
            return make_num(rt);
        }
        case N_NOT: {
            Value v = eval(n->a);
            int t = value_truthy(v);
            free_value(v);
            return make_num(!t);
        }
        case N_NEG: {
            Value v = eval(n->a);
            double d = want_num(v, "-");
            free_value(v);
            return make_num(-d);
        }
        case N_SQRT: {
            Value v = eval(n->a);
            double d = want_num(v, "borgomul");
            free_value(v);
            if (d < 0) runtime_error("borgomul of a negative number");
            return make_num(sqrt(d));
        }
        case N_ABS: {
            Value v = eval(n->a);
            double d = want_num(v, "poromman");
            free_value(v);
            return make_num(fabs(d));
        }
        case N_LEN: {
            Value v = eval(n->a);
            if (v.type != V_STR) runtime_error("dorgho needs a string");
            double len = (double)strlen(v.str);
            free_value(v);
            return make_num(len);
        }
        default:
            runtime_error("internal error: expression node %d", n->kind);
    }
    return make_num(0);
}

/* ================= Statement execution ================= */

void exec(Node *n) {
    if (!n) return;
    switch (n->kind) {
        case N_SEQ:
            exec(n->a);
            exec(n->b);
            break;
        case N_ASSIGN: {
            Value v = eval(n->a);
            sym_set(n->str, v);   /* symbol table owns v now */
            break;
        }
        case N_PRINT: {
            int first = 1;
            for (Node *arg = n->a; arg; arg = arg->b) {
                if (!first) putchar(' ');
                Value v = eval(arg->a);
                print_value(v);
                free_value(v);
                first = 0;
            }
            putchar('\n');
            break;
        }
        case N_IF: {
            Value c = eval(n->a);
            int t = value_truthy(c);
            free_value(c);
            if (t) exec(n->b);
            else   exec(n->c);
            break;
        }
        case N_WHILE:
            for (;;) {
                Value c = eval(n->a);
                int t = value_truthy(c);
                free_value(c);
                if (!t) break;
                exec(n->b);
            }
            break;
        case N_FOR:
            exec(n->a);                       /* init */
            for (;;) {
                Value c = eval(n->b);         /* condition */
                int t = value_truthy(c);
                free_value(c);
                if (!t) break;
                exec(n->d);                   /* body */
                exec(n->c);                   /* step */
            }
            break;
        default:
            runtime_error("internal error: statement node %d", n->kind);
    }
}
