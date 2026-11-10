/**
 * @file math.ms
 * @brief Biblioteca Matemática com Funções Científicas, Aritméticas e Trigonométricas para MaiaScript Native.
 * @author Roberto & MaiaOS Team
 */

#pragma once

extern f32 powf(f32 base, f32 exponent);

namespace math {
    f32 power(f32 base, f32 exponent) {
        return powf(base, exponent);
    }

    /**
     * @brief Retorna o valor absoluto de um inteiro.
     * @param {i32} v - Valor de entrada.
     * @return {i32} Valor positivo correspondente.
     */
    i32 abs(i32 v) {
        if (v < 0) {
            return -v;
        }
        return v;
    }

    /**
     * @brief Retorna o valor absoluto de um número em ponto flutuante.
     * @param {f32} v - Valor de entrada.
     * @return {f32} Valor positivo correspondente.
     */
    f32 fabs(f32 v) {
        if (v < 0.0) {
            return -v;
        }
        return v;
    }

    /**
     * @brief Retorna o menor entre dois números inteiros.
     * @param {i32} a - Primeiro valor.
     * @param {i32} b - Segundo valor.
     * @return {i32} O menor valor.
     */
    i32 min(i32 a, i32 b) {
        if (a < b) {
            return a;
        }
        return b;
    }

    /**
     * @brief Retorna o maior entre dois números inteiros.
     * @param {i32} a - Primeiro valor.
     * @param {i32} b - Segundo valor.
     * @return {i32} O maior valor.
     */
    i32 max(i32 a, i32 b) {
        if (a > b) {
            return a;
        }
        return b;
    }

    /**
     * @brief Restringe um valor inteiro dentro de um intervalo delimitado [lo, hi].
     * @param {i32} v - Valor de entrada.
     * @param {i32} lo - Limite inferior.
     * @param {i32} hi - Limite superior.
     * @return {i32} Valor restringido ao intervalo.
     */
    i32 clamp(i32 v, i32 lo, i32 hi) {
        if (v < lo) {
            return lo;
        }
        if (v > hi) {
            return hi;
        }
        return v;
    }

    /**
     * @brief Restringe um valor em ponto flutuante dentro do intervalo [lo, hi].
     * @param {f32} v - Valor de entrada.
     * @param {f32} lo - Limite inferior.
     * @param {f32} hi - Limite superior.
     * @return {f32} Valor restringido ao intervalo.
     */
    f32 fclamp(f32 v, f32 lo, f32 hi) {
        if (v < lo) {
            return lo;
        }
        if (v > hi) {
            return hi;
        }
        return v;
    }

    /**
     * @brief Calcula a raiz quadrada aproximada de um número em ponto flutuante pelo método de Newton-Raphson.
     * @param {f32} x - Valor de entrada (deve ser >= 0.0).
     * @return {f32} Raiz quadrada de x.
     */
    f32 sqrt(f32 x) {
        if (x <= 0.0) {
            return 0.0;
        }
        f32 guess = x / 2.0;
        for (i32 i = 0; i < 12; i++) {
            guess = (guess + x / guess) / 2.0;
        }
        return guess;
    }

    /**
     * @brief Eleva uma base inteira a uma potência inteira não-negativa.
     * @param {i32} base - Base inteira.
     * @param {i32} exp - Expoente.
     * @return {i32} Resultado de base^exp.
     */
    i32 pow(i32 base, i32 exp) {
        i32 res = 1;
        for (i32 i = 0; i < exp; i++) {
            res = res * base;
        }
        return res;
    }

    /**
     * @brief Eleva uma base em ponto flutuante a um expoente inteiro.
     * @param {f32} base - Base em ponto flutuante.
     * @param {i32} exp - Expoente inteiro.
     * @return {f32} Resultado de base^exp.
     */
    f32 fpow(f32 base, i32 exp) {
        f32 res = 1.0;
        i32 e = exp;
        if (e < 0) {
            e = 0 - e;
        }
        for (i32 i = 0; i < e; i++) {
            res = res * base;
        }
        if (exp < 0 && res != 0.0) {
            return 1.0 / res;
        }
        return res;
    }

    /**
     * @brief Retorna o valor da constante matemática Pi (3.14159265).
     * @return {f32} Valor de Pi em precisão simples.
     */
    f32 pi() {
        return 3.14159265;
    }

    /**
     * @brief Retorna o valor da constante de Euler (2.71828182).
     * @return {f32} Valor de E em precisão simples.
     */
    f32 e() {
        return 2.71828182;
    }

    /**
     * @brief Reduz um ângulo em radianos para o intervalo [-PI, PI].
     * @param {f32} rad - Ângulo em radianos.
     * @return {f32} Ângulo reduzido.
     */
    f32 wrapAngle(f32 rad) {
        f32 two_pi = 6.2831853;
        while (rad > 3.14159265) {
            rad = rad - two_pi;
        }
        while (rad < -3.14159265) {
            rad = rad + two_pi;
        }
        return rad;
    }

    /**
     * @brief Calcula o seno de um ângulo em radianos por expansão em Série de Taylor.
     * @param {f32} rad - Ângulo em radianos.
     * @return {f32} Seno do ângulo.
     */
    f32 sin(f32 rad) {
        f32 x = wrapAngle(rad);
        f32 x2 = x * x;
        f32 x3 = x * x2;
        f32 x5 = x3 * x2;
        f32 x7 = x5 * x2;
        f32 x9 = x7 * x2;
        // x - x^3/6 + x^5/120 - x^7/5040 + x^9/362880
        return x - (x3 / 6.0) + (x5 / 120.0) - (x7 / 5040.0) + (x9 / 362880.0);
    }

    /**
     * @brief Calcula o cosseno de um ângulo em radianos por expansão em Série de Taylor.
     * @param {f32} rad - Ângulo em radianos.
     * @return {f32} Cosseno do ângulo.
     */
    f32 cos(f32 rad) {
        f32 x = wrapAngle(rad);
        f32 x2 = x * x;
        f32 x4 = x2 * x2;
        f32 x6 = x4 * x2;
        f32 x8 = x6 * x2;
        // 1 - x^2/2 + x^4/24 - x^6/720 + x^8/40320
        return 1.0 - (x2 / 2.0) + (x4 / 24.0) - (x6 / 720.0) + (x8 / 40320.0);
    }

    /**
     * @brief Calcula a tangente de um ângulo em radianos (sin/cos).
     * @param {f32} rad - Ângulo em radianos.
     * @return {f32} Tangente do ângulo.
     */
    f32 tan(f32 rad) {
        f32 c = cos(rad);
        if (fabs(c) < 0.00001) {
            return 999999.0;
        }
        return sin(rad) / c;
    }

    /**
     * @brief Calcula a função exponencial e^x por expansão em Série de Taylor.
     * @param {f32} x - Expoente.
     * @return {f32} Valor de e^x.
     */
    f32 exp(f32 x) {
        if (x > 15.0) {
            x = 15.0;
        }
        if (x < -15.0) {
            return 0.0;
        }
        f32 term = 1.0;
        f32 sum = 1.0;
        for (i32 i = 1; i <= 14; i++) {
            term = term * (x / (i + 0.0));
            sum = sum + term;
        }
        return sum;
    }

    /**
     * @brief Calcula o logaritmo natural ln(x) para x > 0.
     * @param {f32} x - Valor positivo.
     * @return {f32} Logaritmo natural de x.
     */
    f32 ln(f32 x) {
        if (x <= 0.0) {
            return -999999.0;
        }
        f32 k = 0.0;
        while (x > 2.0) {
            x = x / 2.71828182;
            k = k + 1.0;
        }
        while (x < 0.5) {
            x = x * 2.71828182;
            k = k - 1.0;
        }
        f32 y = (x - 1.0) / (x + 1.0);
        f32 y2 = y * y;
        f32 term = y;
        f32 sum = y;
        i32 i = 3;
        while (i <= 13) {
            term = term * y2;
            sum = sum + (term / (i + 0.0));
            i = i + 2;
        }
        return (2.0 * sum) + k;
    }

    /**
     * @brief Calcula o logaritmo na base 10 de x.
     * @param {f32} x - Valor positivo.
     * @return {f32} log10(x).
     */
    f32 log10(f32 x) {
        return ln(x) / 2.30258509;
    }
}
