#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../../include/maia_shell/maia_shell.hpp"

static char s_last_out[512];
static void mock_out(const char *s) {
    strncat(s_last_out, s, sizeof(s_last_out) - strlen(s_last_out) - 1);
}

int main() {
    MaiaShell shell;
    shell.init(mock_out);

    // Test 1: Multi-token expr
    s_last_out[0] = '\0';
    int st = shell.execute("expr 2 + 3 '*' 4");
    assert(st == 0);
    assert(strstr(s_last_out, "14\n") != nullptr);

    // Test 2: Parentheses precedence
    s_last_out[0] = '\0';
    st = shell.execute("expr '(' 2 + 3 ')' '*' 4");
    if (st != 0) fprintf(stderr, "parenthesized expr status=%d output=%s\n", st, s_last_out);
    assert(st == 0);
    assert(strstr(s_last_out, "20\n") != nullptr);

    // Test 3: Division by zero
    s_last_out[0] = '\0';
    st = shell.execute("expr 10 / 0");
    assert(st == 2);
    assert(strstr(s_last_out, "division by zero") != nullptr);

    // Test 4: Expr 0 return code
    s_last_out[0] = '\0';
    st = shell.execute("expr 0");
    assert(st == 1);
    assert(strstr(s_last_out, "0\n") != nullptr);

    // Test 5: Command Substitution in assignment
    s_last_out[0] = '\0';
    st = shell.execute("N=$(expr 2 + 3)");
    assert(st == 0);
    assert(strcmp(shell.get_var("N"), "5") == 0);

    // Test 6: Command substitution with expr 0 exit status
    s_last_out[0] = '\0';
    st = shell.execute("N=$(expr 0)");
    assert(st == 1);
    assert(strcmp(shell.get_var("N"), "0") == 0);
    assert(strcmp(shell.get_var("?"), "1") == 0);

    // Test 7: While loop with expr substitution
    s_last_out[0] = '\0';
    st = shell.execute("N=0; while [ $N -lt 5 ]; do N=$(expr $N + 1); done");
    assert(st == 0);
    assert(strcmp(shell.get_var("N"), "5") == 0);

    // Test 8: If/elif/else branching
    s_last_out[0] = '\0';
    shell.execute("VAL=2; if [ $VAL -eq 1 ]; then echo ONE; elif [ $VAL -eq 2 ]; then echo TWO; else echo OTHER; fi");
    assert(strstr(s_last_out, "TWO\n") != nullptr);

    // Test 9: For in loop with break
    s_last_out[0] = '\0';
    shell.execute("for color in red green blue yellow; do if [ $color = blue ]; then echo FOUND_BLUE; break; fi; done");
    assert(strstr(s_last_out, "FOUND_BLUE\n") != nullptr);

    // Test 10: For in loop with if condition match
    s_last_out[0] = '\0';
    shell.execute("for item in foo bar baz; do if [ $item = bar ]; then echo MATCH $item; fi; done");
    printf("Test 10 out: '%s'\n", s_last_out);
    assert(strstr(s_last_out, "MATCH bar\n") != nullptr);

    printf("ALL 10 MaiaShell expr and control flow contract tests passed!\n");
    return 0;
}

