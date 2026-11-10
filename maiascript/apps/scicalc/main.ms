/**
 * @file main.ms
 * @brief Calculadora Científica Maia (FPU RV32IMAF + Parser Descendente Recursivo).
 * @author Roberto & MaiaOS Team
 */

import maia.app;
import maia.ui;
import maia.math;

extern void play_beep(i32 freq, i32 ms);
extern void print(string text);
extern void println(string text);
extern void print_int(i32 value);
extern i32 printf(string fmt, ...);

string g_expr_buf = 0;
i32 g_expr_len = 0;
string g_res_buf = 0;
f32 g_last_val = 0.0;
i32 g_clear_on_next = 1;
i32 g_parse_pos = 0;
i32 g_parse_err = 0;

/**
 * @brief Garante que os buffers de expressão e resultado estejam alocados.
 */
void ensureBuffers() {
    if (g_expr_buf == 0) {
        g_expr_buf = malloc(128);
        g_expr_buf[0] = 48; // '0'
        g_expr_buf[1] = 0;
        g_expr_len = 1;
    }
    if (g_res_buf == 0) {
        g_res_buf = malloc(64);
        g_res_buf[0] = 61; // '='
        g_res_buf[1] = 32; // ' '
        g_res_buf[2] = 48; // '0'
        g_res_buf[3] = 0;
    }
}

/**
 * @brief Formata um valor float IEEE-754 em string ASCII dentro do buffer alvo.
 * @param {f32} v - Valor em ponto flutuante.
 * @param {string} out - Buffer de saída.
 * @param {i32} prefix_eq - Se 1, prefixa com "= ".
 * @return {i32} Comprimento escrito.
 */
i32 formatFloatToBuf(f32 v, string out, i32 prefix_eq) {
    i32 pos = 0;
    if (prefix_eq != 0) {
        out[pos] = 61; // '='
        pos = pos + 1;
        out[pos] = 32; // ' '
        pos = pos + 1;
    }

    if (v < 0.0) {
        out[pos] = 45; // '-'
        pos = pos + 1;
        v = 0.0 - v;
    }

    // Arredondamento na 4a casa decimal
    v = v + 0.00005;

    i32 int_part = v;
    f32 rem = v - int_part;
    if (rem < 0.0) {
        int_part = int_part - 1;
        rem = v - int_part;
    }
    f32 fractional_scaled = rem * 10000.0;
    i32 frac_part = fractional_scaled;
    // RV32 fcvt may round up; force truncation after the explicit decimal bias.
    if (frac_part > fractional_scaled) { frac_part = frac_part - 1; }

    if (int_part == 0) {
        out[pos] = 48;
        pos = pos + 1;
    } else {
        i32 tmp = int_part;
        i32 rev_start = pos;
        while (tmp > 0) {
            i32 dig = tmp % 10;
            out[pos] = 48 + dig;
            pos = pos + 1;
            tmp = tmp / 10;
        }
        i32 left = rev_start;
        i32 right = pos - 1;
        while (left < right) {
            i32 swap_c = out[left];
            out[left] = out[right];
            out[right] = swap_c;
            left = left + 1;
            right = right - 1;
        }
    }

    if (frac_part > 0) {
        out[pos] = 46; // '.'
        pos = pos + 1;

        i32 d1 = (frac_part / 1000) % 10;
        i32 d2 = (frac_part / 100) % 10;
        i32 d3 = (frac_part / 10) % 10;
        i32 d4 = frac_part % 10;

        out[pos] = 48 + d1;
        out[pos + 1] = 48 + d2;
        out[pos + 2] = 48 + d3;
        out[pos + 3] = 48 + d4;
        pos = pos + 4;

        while (pos > 0 && out[pos - 1] == 48) {
            pos = pos - 1;
        }
        if (pos > 0 && out[pos - 1] == 46) {
            pos = pos - 1;
        }
    }

    out[pos] = 0;
    return pos;
}

/**
 * @brief Renderiza o visor LCD gráfico superior com a expressão algébrica e o resultado.
 */
void updateDisplay() {
    ensureBuffers();

    // Visor LCD Gráfico (x=296, y=151, w=688, h=76)
    fill_rect(296, 151, 688, 76, 0x0841);
    draw_rect(296, 151, 688, 76, 0x07ff);
    draw_rect(298, 153, 684, 72, 0x18e3);

    // Linha superior: Expressão algébrica completa (ex: 2+3*4 ou sin(0.5)+2^3)
    draw_text(310, 156, g_expr_buf, 0xffe0, 0x0841);

    // Linha inferior: Resultado avaliado em verde neon
    draw_text(310, 190, g_res_buf, 0x07e0, 0x0841);
    display_sync();
}

/* =========================================================================
 * Parser Descendente Recursivo com Precedência de Operadores:
 *   1. parseExpr    : Soma (+) e Subtração (-)          [menor precedência]
 *   2. parseTerm    : Multiplicação (*) e Divisão (/)   [média precedência]
 *   3. parsePower   : Potenciação (^)                   [alta precedência, assoc. direita]
 *   4. parseUnary   : Operadores unários (+x, -x)
 *   5. parsePrimary : Parênteses (...), Funções sin/cos/tan/sqrt/ln/exp, pi e Números
 * ========================================================================= */

void parseSkipSpaces() {
    while (g_expr_buf[g_parse_pos] == 32) {
        g_parse_pos = g_parse_pos + 1;
    }
}

f32 parseNumber() {
    parseSkipSpaces();
    f32 val = 0.0;
    i32 has_digits = 0;

    while (g_expr_buf[g_parse_pos] >= 48 && g_expr_buf[g_parse_pos] <= 57) {
        i32 d = g_expr_buf[g_parse_pos] - 48;
        val = val * 10.0 + d;
        g_parse_pos = g_parse_pos + 1;
        has_digits = 1;
    }

    if (g_expr_buf[g_parse_pos] == 46) {
        g_parse_pos = g_parse_pos + 1;
        f32 frac = 0.1;
        while (g_expr_buf[g_parse_pos] >= 48 && g_expr_buf[g_parse_pos] <= 57) {
            i32 d = g_expr_buf[g_parse_pos] - 48;
            val = val + d * frac;
            frac = frac * 0.1;
            g_parse_pos = g_parse_pos + 1;
            has_digits = 1;
        }
    }

    if (has_digits == 0) {
        g_parse_err = 1;
        return 0.0;
    }
    return val;
}

f32 parsePrimary() {
    parseSkipSpaces();
    if (g_parse_err != 0) {
        return 0.0;
    }

    i32 c0 = g_expr_buf[g_parse_pos];
    if (c0 == 0) {
        g_parse_err = 1;
        return 0.0;
    }

    // Sub-expressão entre parênteses: ( expr )
    if (c0 == 40) {
        g_parse_pos = g_parse_pos + 1;
        f32 val = parseExpr();
        parseSkipSpaces();
        if (g_expr_buf[g_parse_pos] == 41) {
            g_parse_pos = g_parse_pos + 1;
        }
        return val;
    }

    // Constante pi: "pi"
    if (c0 == 112 && g_expr_buf[g_parse_pos + 1] == 105) {
        g_parse_pos = g_parse_pos + 2;
        return math.pi();
    }

    // Funções científicas: sin(...), sqrt(...)
    if (c0 == 115) {
        if (g_expr_buf[g_parse_pos + 1] == 105 && g_expr_buf[g_parse_pos + 2] == 110 && g_expr_buf[g_parse_pos + 3] == 40) {
            g_parse_pos = g_parse_pos + 4;
            f32 arg = parseExpr();
            parseSkipSpaces();
            if (g_expr_buf[g_parse_pos] == 41) { g_parse_pos = g_parse_pos + 1; }
            return math.sin(arg);
        }
        if (g_expr_buf[g_parse_pos + 1] == 113 && g_expr_buf[g_parse_pos + 2] == 114 && g_expr_buf[g_parse_pos + 3] == 116 && g_expr_buf[g_parse_pos + 4] == 40) {
            g_parse_pos = g_parse_pos + 5;
            f32 arg = parseExpr();
            parseSkipSpaces();
            if (g_expr_buf[g_parse_pos] == 41) { g_parse_pos = g_parse_pos + 1; }
            if (arg < 0.0) {
                g_parse_err = 2;
                return 0.0;
            }
            return math.sqrt(arg);
        }
    }

    // cos(...)
    if (c0 == 99 && g_expr_buf[g_parse_pos + 1] == 111 && g_expr_buf[g_parse_pos + 2] == 115 && g_expr_buf[g_parse_pos + 3] == 40) {
        g_parse_pos = g_parse_pos + 4;
        f32 arg = parseExpr();
        parseSkipSpaces();
        if (g_expr_buf[g_parse_pos] == 41) { g_parse_pos = g_parse_pos + 1; }
        return math.cos(arg);
    }

    // tan(...)
    if (c0 == 116 && g_expr_buf[g_parse_pos + 1] == 97 && g_expr_buf[g_parse_pos + 2] == 110 && g_expr_buf[g_parse_pos + 3] == 40) {
        g_parse_pos = g_parse_pos + 4;
        f32 arg = parseExpr();
        parseSkipSpaces();
        if (g_expr_buf[g_parse_pos] == 41) { g_parse_pos = g_parse_pos + 1; }
        return math.tan(arg);
    }

    // ln(...)
    if (c0 == 108 && g_expr_buf[g_parse_pos + 1] == 110 && g_expr_buf[g_parse_pos + 2] == 40) {
        g_parse_pos = g_parse_pos + 3;
        f32 arg = parseExpr();
        parseSkipSpaces();
        if (g_expr_buf[g_parse_pos] == 41) { g_parse_pos = g_parse_pos + 1; }
        if (arg <= 0.0) {
            g_parse_err = 2;
            return 0.0;
        }
        return math.ln(arg);
    }

    // exp(...)
    if (c0 == 101 && g_expr_buf[g_parse_pos + 1] == 120 && g_expr_buf[g_parse_pos + 2] == 112 && g_expr_buf[g_parse_pos + 3] == 40) {
        g_parse_pos = g_parse_pos + 4;
        f32 arg = parseExpr();
        parseSkipSpaces();
        if (g_expr_buf[g_parse_pos] == 41) { g_parse_pos = g_parse_pos + 1; }
        return math.exp(arg);
    }

    return parseNumber();
}

f32 parseUnary() {
    parseSkipSpaces();
    if (g_expr_buf[g_parse_pos] == 43) {
        g_parse_pos = g_parse_pos + 1;
        return parseUnary();
    }
    if (g_expr_buf[g_parse_pos] == 45) {
        g_parse_pos = g_parse_pos + 1;
        return 0.0 - parseUnary();
    }
    return parsePrimary();
}

f32 parsePower() {
    f32 base = parseUnary();
    parseSkipSpaces();
    if (g_parse_err == 0 && g_expr_buf[g_parse_pos] == 94) {
        g_parse_pos = g_parse_pos + 1;
        f32 exp_val = parsePower();
        return math.power(base, exp_val);
    }
    return base;
}

f32 parseTerm() {
    f32 left = parsePower();
    while (g_parse_err == 0) {
        parseSkipSpaces();
        i32 op = g_expr_buf[g_parse_pos];
        if (op == 42) {
            g_parse_pos = g_parse_pos + 1;
            f32 right = parsePower();
            left = left * right;
        } else if (op == 47) {
            g_parse_pos = g_parse_pos + 1;
            f32 right = parsePower();
            if (math.fabs(right) < 0.000001) {
                g_parse_err = 2;
                return 0.0;
            }
            left = left / right;
        } else {
            return left;
        }
    }
    return left;
}

f32 parseExpr() {
    f32 left = parseTerm();
    while (g_parse_err == 0) {
        parseSkipSpaces();
        i32 op = g_expr_buf[g_parse_pos];
        if (op == 43) {
            g_parse_pos = g_parse_pos + 1;
            f32 right = parseTerm();
            left = left + right;
        } else if (op == 45) {
            g_parse_pos = g_parse_pos + 1;
            f32 right = parseTerm();
            left = left - right;
        } else {
            return left;
        }
    }
    return left;
}

/* =========================================================================
 * Manipuladores de Entrada da Expressão
 * ========================================================================= */

void appendChar(i32 ch) {
    ensureBuffers();
    if (g_expr_len < 38) {
        g_expr_buf[g_expr_len] = ch;
        g_expr_len = g_expr_len + 1;
        g_expr_buf[g_expr_len] = 0;
    }
}

void appendStr(string s) {
    ensureBuffers();
    i32 i = 0;
    while (s[i] != 0 && g_expr_len < 38) {
        g_expr_buf[g_expr_len] = s[i];
        g_expr_len = g_expr_len + 1;
        i = i + 1;
    }
    g_expr_buf[g_expr_len] = 0;
}

void pushDigit(i32 d) {
    ensureBuffers();
    if (g_clear_on_next != 0) {
        g_expr_len = 0;
        g_clear_on_next = 0;
    } else if (g_expr_len == 1 && g_expr_buf[0] == 48) {
        g_expr_len = 0;
    }
    appendChar(48 + d);
    play_beep(1200 + d * 60, 20);
    updateDisplay();
}

void onDigit0() { pushDigit(0); }
void onDigit1() { pushDigit(1); }
void onDigit2() { pushDigit(2); }
void onDigit3() { pushDigit(3); }
void onDigit4() { pushDigit(4); }
void onDigit5() { pushDigit(5); }
void onDigit6() { pushDigit(6); }
void onDigit7() { pushDigit(7); }
void onDigit8() { pushDigit(8); }
void onDigit9() { pushDigit(9); }

void onDecimal() {
    ensureBuffers();
    if (g_clear_on_next != 0) {
        g_expr_buf[0] = 48;
        g_expr_buf[1] = 0;
        g_expr_len = 1;
        g_clear_on_next = 0;
    }
    appendChar(46);
    play_beep(1400, 20);
    updateDisplay();
}

void pushOpChar(i32 op_ch) {
    ensureBuffers();
    if (g_clear_on_next != 0) {
        g_expr_len = formatFloatToBuf(g_last_val, g_expr_buf, 0);
        g_clear_on_next = 0;
    }
    if (g_expr_len > 0) {
        i32 last = g_expr_buf[g_expr_len - 1];
        if (last == 43 || last == 45 || last == 42 || last == 47 || last == 94) {
            g_expr_buf[g_expr_len - 1] = op_ch;
            play_beep(1600, 25);
            updateDisplay();
            return;
        }
    }
    appendChar(op_ch);
    play_beep(1600, 25);
    updateDisplay();
}

void opAdd() { pushOpChar(43); } // '+'
void opSub() { pushOpChar(45); } // '-'
void opMul() { pushOpChar(42); } // '*'
void opDiv() { pushOpChar(47); } // '/'
void opPow() { pushOpChar(94); } // '^'

void onParenOpen() {
    ensureBuffers();
    if (g_clear_on_next != 0) {
        g_expr_len = 0;
        g_clear_on_next = 0;
    } else if (g_expr_len == 1 && g_expr_buf[0] == 48) {
        g_expr_len = 0;
    }
    appendChar(40); // '('
    play_beep(1500, 25);
    updateDisplay();
}

void onParenClose() {
    ensureBuffers();
    if (g_clear_on_next == 0 && g_expr_len > 0) {
        appendChar(41); // ')'
    }
    play_beep(1500, 25);
    updateDisplay();
}

void onDel() {
    ensureBuffers();
    if (g_clear_on_next != 0 || g_expr_len <= 1) {
        g_expr_buf[0] = 48;
        g_expr_buf[1] = 0;
        g_expr_len = 1;
        g_clear_on_next = 1;
    } else {
        g_expr_len = g_expr_len - 1;
        g_expr_buf[g_expr_len] = 0;
    }
    play_beep(900, 25);
    updateDisplay();
}

void opClear() {
    ensureBuffers();
    g_expr_buf[0] = 48;
    g_expr_buf[1] = 0;
    g_expr_len = 1;
    g_last_val = 0.0;
    g_clear_on_next = 1;
    formatFloatToBuf(0.0, g_res_buf, 1);
    play_beep(880, 40);
    updateDisplay();
}

void opEquals() {
    ensureBuffers();
    g_parse_pos = 0;
    g_parse_err = 0;
    f32 res = parseExpr();
    parseSkipSpaces();
    if (g_expr_buf[g_parse_pos] != 0) {
        g_parse_err = 1;
    }

    if (g_parse_err == 2) {
        g_res_buf[0] = 61; // '='
        g_res_buf[1] = 32; // ' '
        g_res_buf[2] = 69; // 'E'
        g_res_buf[3] = 82; // 'R'
        g_res_buf[4] = 82; // 'R'
        g_res_buf[5] = 79; // 'O'
        g_res_buf[6] = 32; // ' '
        g_res_buf[7] = 68; // 'D'
        g_res_buf[8] = 79; // 'O'
        g_res_buf[9] = 77; // 'M'
        g_res_buf[10] = 0;
    } else if (g_parse_err != 0) {
        g_res_buf[0] = 61; // '='
        g_res_buf[1] = 32; // ' '
        g_res_buf[2] = 69; // 'E'
        g_res_buf[3] = 82; // 'R'
        g_res_buf[4] = 82; // 'R'
        g_res_buf[5] = 79; // 'O'
        g_res_buf[6] = 0;
    } else {
        g_last_val = res;
        formatFloatToBuf(res, g_res_buf, 1);
    }
    print("[SciCalc] ");
    print(g_expr_buf);
    print(" ");
    println(g_res_buf);
    g_clear_on_next = 1;
    play_beep(2200, 50);
    updateDisplay();
}

/**
 * @brief Insere função científica: se estiver após operador/início, adiciona "fn(";
 *        se já houver número/expressão digitada sem operador pendente, envolve "fn(expr)" e calcula.
 */
void pushSciFunc(string fn_prefix) {
    ensureBuffers();
    if (g_clear_on_next != 0) {
        // Se acabou de limpar e o visor é "0", inicia "fn(" para digitação algébrica
        if (g_expr_len == 1 && g_expr_buf[0] == 48 && math.fabs(g_last_val) < 0.000001) {
            g_expr_len = 0;
            g_clear_on_next = 0;
            appendStr(fn_prefix);
            play_beep(1760, 35);
            updateDisplay();
            return;
        }
        // Caso contrário envolve o valor atual e calcula imediatamente
        string tmp = malloc(64);
        i32 n = formatFloatToBuf(g_last_val, tmp, 0);
        g_expr_len = 0;
        appendStr(fn_prefix);
        appendStr(tmp);
        appendChar(41);
        opEquals();
        return;
    }

    if (g_expr_len > 0) {
        i32 last = g_expr_buf[g_expr_len - 1];
        if (last == 43 || last == 45 || last == 42 || last == 47 || last == 94 || last == 40) {
            appendStr(fn_prefix);
            play_beep(1760, 35);
            updateDisplay();
            return;
        }
    }

    // Se havia apenas um número ou expressão fechada, envolve com fn(...) e avalia
    string old_expr = malloc(64);
    i32 i = 0;
    while (i < g_expr_len) {
        old_expr[i] = g_expr_buf[i];
        i = i + 1;
    }
    old_expr[i] = 0;
    g_expr_len = 0;
    appendStr(fn_prefix);
    appendStr(old_expr);
    appendChar(41);
    opEquals();
}

void opSin()  { pushSciFunc("sin("); }
void opCos()  { pushSciFunc("cos("); }
void opTan()  { pushSciFunc("tan("); }
void opSqrt() { pushSciFunc("sqrt("); }
void opLn()   { pushSciFunc("ln("); }
void opExp()  { pushSciFunc("exp("); }

void opPi() {
    ensureBuffers();
    if (g_clear_on_next != 0) {
        g_expr_len = 0;
        g_clear_on_next = 0;
    } else if (g_expr_len == 1 && g_expr_buf[0] == 48) {
        g_expr_len = 0;
    }
    appendStr("pi");
    play_beep(1760, 35);
    updateDisplay();
}

void opSquare() {
    ensureBuffers();
    if (g_clear_on_next != 0) {
        g_expr_len = formatFloatToBuf(g_last_val, g_expr_buf, 0);
        g_clear_on_next = 0;
    }
    appendStr("^2");
    opEquals();
}

/**
 * @brief Suporte ao teclado físico I2C para digitação algébrica direta.
 * @param {i32} k - Código ASCII da tecla.
 */
void onCalcKey(i32 k) {
    if (k >= 48 && k <= 57) {
        pushDigit(k - 48);
    } else if (k == 46 || k == 44) {
        onDecimal();
    } else if (k == 43) {
        opAdd();
    } else if (k == 45) {
        opSub();
    } else if (k == 42) {
        opMul();
    } else if (k == 47) {
        opDiv();
    } else if (k == 94) {
        opPow();
    } else if (k == 40) {
        onParenOpen();
    } else if (k == 41) {
        onParenClose();
    } else if (k == 8 || k == 127) {
        onDel();
    } else if (k == 61 || k == 13 || k == 10) {
        opEquals();
    } else if (k == 99 || k == 67) {
        opClear();
    } else if (k == 115 || k == 83) {
        opSin();
    } else if (k == 111 || k == 79) {
        opCos();
    } else if (k == 116 || k == 84) {
        opTan();
    } else if (k == 114 || k == 82) {
        opSqrt();
    } else if (k == 108 || k == 76) {
        opLn();
    } else if (k == 101 || k == 69) {
        opExp();
    } else if (k == 112 || k == 80) {
        opPi();
    }
}

/**
 * @brief Ponto de entrada da Calculadora Científica Maia.
 * @return {i32} Código de saída.
 */
i32 main() {
    ui.Window* win = ui.Window("Calculadora Cientifica Maia", 760, 540);
    win->show();

    ui.Column* layout = ui.Column(win);
    layout->setPadding(16);
    layout->setGap(8);

    // Reserva espaço para o Painel LCD Gráfico Superior (76px)
    layout->allocHeight(76);

    // Linha 1: Funções Científicas (6 botões de 104px, gap 12px = 684px)
    ui.Row* r1 = ui.Row(layout);
    r1->setItemWidth(104);
    r1->setGap(12);
    ui.Button* bSin = ui.Button(r1, "sin");
    bSin->onClick(opSin);
    ui.Button* bCos = ui.Button(r1, "cos");
    bCos->onClick(opCos);
    ui.Button* bTan = ui.Button(r1, "tan");
    bTan->onClick(opTan);
    ui.Button* bSqrt = ui.Button(r1, "sqrt");
    bSqrt->onClick(opSqrt);
    ui.Button* bLn = ui.Button(r1, "ln");
    bLn->onClick(opLn);
    ui.Button* bExp = ui.Button(r1, "exp");
    bExp->onClick(opExp);

    // Linha 2: 7, 8, 9, /, x^y, CLEAR
    ui.Row* r2 = ui.Row(layout);
    r2->setItemWidth(104);
    r2->setGap(12);
    ui.Button* b7 = ui.Button(r2, "7");
    b7->onClick(onDigit7);
    ui.Button* b8 = ui.Button(r2, "8");
    b8->onClick(onDigit8);
    ui.Button* b9 = ui.Button(r2, "9");
    b9->onClick(onDigit9);
    ui.Button* bDiv = ui.Button(r2, "/");
    bDiv->onClick(opDiv);
    ui.Button* bPow = ui.Button(r2, "x^y");
    bPow->onClick(opPow);
    ui.Button* bClr = ui.Button(r2, "CLEAR");
    bClr->onClick(opClear);

    // Linha 3: 4, 5, 6, *, (, )
    ui.Row* r3 = ui.Row(layout);
    r3->setItemWidth(104);
    r3->setGap(12);
    ui.Button* b4 = ui.Button(r3, "4");
    b4->onClick(onDigit4);
    ui.Button* b5 = ui.Button(r3, "5");
    b5->onClick(onDigit5);
    ui.Button* b6 = ui.Button(r3, "6");
    b6->onClick(onDigit6);
    ui.Button* bMul = ui.Button(r3, "*");
    bMul->onClick(opMul);
    ui.Button* bPo = ui.Button(r3, "(");
    bPo->onClick(onParenOpen);
    ui.Button* bPc = ui.Button(r3, ")");
    bPc->onClick(onParenClose);

    // Linha 4: 1, 2, 3, -, x^2, pi
    ui.Row* r4 = ui.Row(layout);
    r4->setItemWidth(104);
    r4->setGap(12);
    ui.Button* b1 = ui.Button(r4, "1");
    b1->onClick(onDigit1);
    ui.Button* b2 = ui.Button(r4, "2");
    b2->onClick(onDigit2);
    ui.Button* b3 = ui.Button(r4, "3");
    b3->onClick(onDigit3);
    ui.Button* bSub = ui.Button(r4, "-");
    bSub->onClick(opSub);
    ui.Button* bSq = ui.Button(r4, "x^2");
    bSq->onClick(opSquare);
    ui.Button* bPi = ui.Button(r4, "pi");
    bPi->onClick(opPi);

    // Linha 5: 0, ., DEL, +, = (CALCULAR)
    ui.Row* r5 = ui.Row(layout);
    r5->setItemWidth(104);
    r5->setGap(12);
    ui.Button* b0 = ui.Button(r5, "0");
    b0->onClick(onDigit0);
    ui.Button* bDot = ui.Button(r5, ".");
    bDot->onClick(onDecimal);
    ui.Button* bDel = ui.Button(r5, "DEL");
    bDel->onClick(onDel);
    ui.Button* bAdd = ui.Button(r5, "+");
    bAdd->onClick(opAdd);
    r5->setItemWidth(220);
    ui.Button* bEq = ui.Button(r5, "CALCULAR (=)");
    bEq->onClick(opEquals);

    // Painel Informativo Inferior de Atalhos (x=296, y=483, w=688, h=110)
    fill_rect(296, 483, 688, 110, 0x1082);
    draw_rect(296, 483, 688, 110, 0x4208);
    draw_text(310, 491, "Parser Descendente Recursivo FPU\nPrecedencia: () fn > ^ > */ > +-\n[S]sin [O]cos [T]tan [R]sqrt [P]pi", 0xffff, 0x1082);

    host_bind_key(onCalcKey);
    g_expr_buf = 0;
    g_expr_len = 0;
    g_res_buf = 0;
    g_last_val = 0.0;
    g_clear_on_next = 1;
    ensureBuffers();
    updateDisplay();
    play_beep(1760, 60);
    return app.run();
}
