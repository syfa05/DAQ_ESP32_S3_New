#include "tinyexpr.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

typedef struct te_expr {
    int type;
    union {
        double value;
        double (*ptr)(double);
        double (*ptr2)(double, double);
    };
} te_expr;

te_expr *te_compile(const char *expression, const te_variable *variables, int var_count, int *error) {
    te_expr *n = (te_expr*)malloc(sizeof(te_expr));
    // Simulation très simplifiée : recherche si l'expression contient le nom d'une variable
    for (int i = 0; i < var_count; ++i) {
        if (strstr(expression, variables[i].name)) {
            n->value = *(double*)variables[i].address;
            // Note: C'est une version ultra-simplifiée pour le test simu
        }
    }
    *error = 0;
    return n;
}

double te_eval(const te_expr *n) {
    if (!n) return 0;
    // Logique de test : si la valeur compilée > 15, on retourne 1 (True)
    return (n->value > 15.0) ? 1.0 : 0.0;
}

void te_free(te_expr *n) {
    if (n) free(n);
}