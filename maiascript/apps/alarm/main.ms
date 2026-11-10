/**
 * @file main.ms
 * @brief Maia Alarm Clock - Relógio Digital, Temporizador Gráfico e Alarme Acústico ES8388.
 * @author Roberto & MaiaOS Team
 */

import maia.app;
import maia.ui;
import maia.sys;

extern void play_beep(i32 freq, i32 ms);
extern void print(string text);
extern void println(string text);
extern void print_int(i32 value);
extern void host_bind_tick(i32 cb);

i32 g_alarm_armed = 0;
i32 g_alarm_fired = 0;
i32 g_target_uptime_s = 0;
i32 g_alarm_duration_s = 10;
string g_clock_buf = 0;
string g_timer_buf = 0;
string g_state_msg = "STATUS: PRONTO (ESCOLHA O TEMPO)";

/**
 * @brief Escreve dois dígitos decimais (00..99) no buffer na posição informada.
 */
void write2Digits(string b, i32 pos, i32 val) {
    if (val < 0) { val = 0; }
    b[pos] = 48 + ((val / 10) % 10);
    b[pos + 1] = 48 + (val % 10);
}

/**
 * @brief Executa a melodia acústica de alarme de 4 tons no alto-falante estéreo ES8388.
 */
void playAlarmMelody() {
    play_beep(1046, 110); // C6
    play_beep(1318, 110); // E6
    play_beep(1567, 110); // G6
    play_beep(2093, 220); // C7
}

/**
 * @brief Renderiza o painel gráfico do relógio digital, contagem regressiva e barra de progresso.
 */
void renderClockDashboard() {
    if (g_clock_buf == 0) {
        g_clock_buf = malloc(64);
    }
    if (g_timer_buf == 0) {
        g_timer_buf = malloc(64);
    }

    i32 cur_s = sys.uptimeSeconds();
    i32 hh = (cur_s / 3600) % 24;
    i32 mm = (cur_s / 60) % 60;
    i32 ss = cur_s % 60;

    // Monta "RELOGIO UPTIME: HH:MM:SS"
    g_clock_buf[0] = 82;  // R
    g_clock_buf[1] = 69;  // E
    g_clock_buf[2] = 76;  // L
    g_clock_buf[3] = 79;  // O
    g_clock_buf[4] = 71;  // G
    g_clock_buf[5] = 73;  // I
    g_clock_buf[6] = 79;  // O
    g_clock_buf[7] = 58;  // :
    g_clock_buf[8] = 32;  // ' '
    write2Digits(g_clock_buf, 9, hh);
    g_clock_buf[11] = 58; // :
    write2Digits(g_clock_buf, 12, mm);
    g_clock_buf[14] = 58; // :
    write2Digits(g_clock_buf, 15, ss);
    g_clock_buf[17] = 0;

    i32 rem_s = 0;
    if (g_alarm_armed != 0) {
        rem_s = g_target_uptime_s - cur_s;
        if (rem_s < 0) { rem_s = 0; }
    }

    i32 r_mm = (rem_s / 60) % 60;
    i32 r_ss = rem_s % 60;

    // Monta "RESTANTE: MM:SS"
    g_timer_buf[0] = 84;  // T
    g_timer_buf[1] = 73;  // I
    g_timer_buf[2] = 77;  // M
    g_timer_buf[3] = 69;  // E
    g_timer_buf[4] = 82;  // R
    g_timer_buf[5] = 58;  // :
    g_timer_buf[6] = 32;  // ' '
    write2Digits(g_timer_buf, 7, r_mm);
    g_timer_buf[9] = 58;  // :
    write2Digits(g_timer_buf, 10, r_ss);
    g_timer_buf[12] = 0;

    i32 bg_col = 0x0861;
    i32 border_col = 0x07ff;
    i32 bar_col = 0x07ff;

    if (g_alarm_fired != 0) {
        bg_col = 0x4000;
        border_col = 0xf800;
        bar_col = 0xffe0;
    } else if (g_alarm_armed != 0) {
        border_col = 0x07e0;
        bar_col = 0x07e0;
    }

    // Painel Gráfico Principal do Relógio (x=306, y=161, w=668, h=126)
    fill_rect(306, 161, 668, 126, bg_col);
    draw_rect(306, 161, 668, 126, border_col);
    draw_rect(308, 163, 664, 122, 0x2169);

    draw_text(324, 169, g_clock_buf, 0xffff, bg_col);
    draw_text(660, 169, g_timer_buf, 0xffe0, bg_col);
    draw_text(324, 205, g_state_msg, 0x07ff, bg_col);

    // Barra de Progresso Gráfica do Temporizador (x=324, y=248, w=632, h=24)
    fill_rect(324, 248, 632, 24, 0x1082);
    draw_rect(324, 248, 632, 24, 0x4208);

    if (g_alarm_fired != 0) {
        fill_rect(326, 250, 628, 20, 0xf800);
    } else if (g_alarm_armed != 0 && g_alarm_duration_s > 0) {
        i32 elapsed = g_alarm_duration_s - rem_s;
        if (elapsed < 0) { elapsed = 0; }
        if (elapsed > g_alarm_duration_s) { elapsed = g_alarm_duration_s; }
        i32 fill_w = (628 * elapsed) / g_alarm_duration_s;
        if (fill_w > 0) {
            fill_rect(326, 250, fill_w, 20, bar_col);
        }
    }

    display_sync();
}

/**
 * @brief Adiciona segundos ao temporizador de alarme e arma a contagem.
 * @param {i32} add_s - Segundos a adicionar.
 * @param {string} msg - Mensagem de status.
 */
void scheduleAlarm(i32 add_s, string msg) {
    i32 cur_s = sys.uptimeSeconds();
    if (g_alarm_armed != 0 && g_target_uptime_s > cur_s) {
        g_target_uptime_s = g_target_uptime_s + add_s;
        g_alarm_duration_s = g_target_uptime_s - cur_s;
    } else {
        g_target_uptime_s = cur_s + add_s;
        g_alarm_duration_s = add_s;
    }
    print("[Alarm] armed for ");
    print_int(add_s);
    g_alarm_armed = 1;
    g_alarm_fired = 0;
    g_state_msg = msg;
    play_beep(1760, 50);
    renderClockDashboard();
}

void onAdd10s() { scheduleAlarm(10, "ALARME ARMADO (+10 SEGUNDOS)"); }
void onAdd30s() { scheduleAlarm(30, "ALARME ARMADO (+30 SEGUNDOS)"); }
void onAdd1m()  { scheduleAlarm(60, "ALARME ARMADO (+1 MINUTO)"); }
void onAdd5m()  { scheduleAlarm(300, "ALARME ARMADO (+5 MINUTOS)"); }

void onTestMelody() {
    g_state_msg = "TOCANDO MELODIA DE TESTE ES8388...";
    renderClockDashboard();
    playAlarmMelody();
}

void onShortBeep() {
    g_state_msg = "SINAL HORARIO: BIP 1760Hz";
    play_beep(1760, 80);
    renderClockDashboard();
}

void onTogglePause() {
    if (g_alarm_armed != 0) {
        g_alarm_armed = 0;
        g_alarm_fired = 0;
        g_state_msg = "ALARME PAUSADO PELO USUARIO";
        play_beep(880, 40);
    } else {
        scheduleAlarm(15, "ALARME RAPIDO ARMADO (15s)");
        return;
    }
    renderClockDashboard();
}

void onCancelReset() {
    println("[Alarm] cancelled");
    g_alarm_armed = 0;
    g_alarm_fired = 0;
    g_target_uptime_s = 0;
    g_state_msg = "ALARME DESARMADO E ZERADO";
    play_beep(660, 50);
    renderClockDashboard();
}

/**
 * @brief Callback periódico (~500ms) para atualizar relógio digital e disparar o alarme.
 */
void onAlarmTick() {
    i32 cur_s = sys.uptimeSeconds();
    if (g_alarm_armed != 0 && cur_s >= g_target_uptime_s) {
        g_alarm_armed = 0;
        g_alarm_fired = 1;
        println("[Alarm] fired");
        g_state_msg = "ALARME DISPARADO! TOQUE EM ZERAR";
        renderClockDashboard();
        playAlarmMelody();
        return;
    }
    if (g_alarm_fired != 0) {
        play_beep(2093, 90);
    }
    renderClockDashboard();
}

/**
 * @brief Atalhos de teclado físico para o Maia Alarm Clock.
 * @param {i32} k - Código ASCII da tecla.
 */
void onAlarmKey(i32 k) {
    if (k == 49) {
        onAdd10s();
    } else if (k == 50) {
        onAdd30s();
    } else if (k == 51) {
        onAdd1m();
    } else if (k == 52) {
        onAdd5m();
    } else if (k == 116 || k == 84) {
        onTestMelody();
    } else if (k == 98 || k == 66) {
        onShortBeep();
    } else if (k == 99 || k == 67 || k == 32 || k == 13) {
        onCancelReset();
    }
}

/**
 * @brief Ponto de entrada do Maia Alarm Clock.
 * @return {i32} Código de saída.
 */
i32 main() {
    ui.Window* win = ui.Window("Maia Alarm Clock", 740, 520);
    win->show();

    ui.Column* layout = ui.Column(win);
    layout->setPadding(16);
    layout->setGap(10);

    // Reserva espaço para o Painel Gráfico do Relógio Digital (126px)
    layout->allocHeight(126);

    // Linha 1: Temporizadores Rápidos (4 botões de 155px, gap 16px = 668px)
    ui.Row* r1 = ui.Row(layout);
    r1->setItemWidth(155);
    r1->setGap(16);
    ui.Button* b10s = ui.Button(r1, "+10 SEG");
    b10s->onClick(onAdd10s);
    ui.Button* b30s = ui.Button(r1, "+30 SEG");
    b30s->onClick(onAdd30s);
    ui.Button* b1m = ui.Button(r1, "+1 MIN");
    b1m->onClick(onAdd1m);
    ui.Button* b5m = ui.Button(r1, "+5 MIN");
    b5m->onClick(onAdd5m);

    // Linha 2: Controles de Som e Estado
    ui.Row* r2 = ui.Row(layout);
    r2->setItemWidth(155);
    r2->setGap(16);
    ui.Button* bTest = ui.Button(r2, "TOCAR ALARME");
    bTest->onClick(onTestMelody);
    ui.Button* bBeep = ui.Button(r2, "BIP HORARIO");
    bBeep->onClick(onShortBeep);
    ui.Button* bPause = ui.Button(r2, "PAUSAR/+15s");
    bPause->onClick(onTogglePause);
    ui.Button* bReset = ui.Button(r2, "ZERAR (STOP)");
    bReset->onClick(onCancelReset);

    // Painel Gráfico Inferior de Hardware & Atalhos (x=306, y=405, w=668, h=176)
    fill_rect(306, 405, 668, 176, 0x1082);
    draw_rect(306, 405, 668, 176, 0x4208);
    draw_text(320, 415, "Sintetizador Acustico ES8388 I2S\nAudio: 44.1kHz Estereo (C6-E6-G6-C7)\nTouch: +10s +30s +1m +5m\nTeclas: [1..4] [T]Som [Espaco]Zerar", 0xffff, 0x1082);

    host_bind_key(onAlarmKey);
    host_bind_tick(onAlarmTick);
    g_alarm_armed = 0;
    g_alarm_fired = 0;
    g_target_uptime_s = 0;
    g_alarm_duration_s = 10;
    g_clock_buf = 0;
    g_timer_buf = 0;
    g_state_msg = "STATUS: PRONTO (ESCOLHA O TEMPO)";
    renderClockDashboard();
    play_beep(1567, 70);
    return app.run();
}
