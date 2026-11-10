i32 main() {
    onCalcKey(50);
    if (g_clear_on_next != 0) { return 10; }
    if (g_expr_len != 1) { return 11; }
    onCalcKey(43);
    if (g_expr_len != 2) { return 12; }
    onCalcKey(51);
    if (g_expr_len != 3) { return 13; }
    onCalcKey(42);
    if (g_expr_len != 4) { return 14; }
    onCalcKey(52);
    if (g_expr_len != 5) { return 15; }
    onCalcKey(61);
    if (g_expr_len != 5) { return 100 + g_expr_len; }
    if (g_clear_on_next != 1) { return 200 + g_clear_on_next; }
    if (g_res_buf[2] != 49 || g_res_buf[3] != 52 || g_res_buf[4] != 0) { return 1; }
    opClear();
    appendStr("2.5^2");
    g_clear_on_next = 0;
    opEquals();
    if (g_res_buf[2] != 54 || g_res_buf[3] != 46 || g_res_buf[4] != 50 || g_res_buf[5] != 53 || g_res_buf[6] != 0) { return 2; }
    opClear();
    appendStr("1/0");
    g_clear_on_next = 0;
    opEquals();
    if (g_parse_err != 2 || g_res_buf[2] != 69) { return 3; }
    return 42;
}
