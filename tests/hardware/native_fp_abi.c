/* Separate translation unit forces a real ILP32F argument/result boundary. */
float maia_probe_fp_add(float left, float right);
float maia_probe_fp_add(float left, float right) {
    return left + right;
}
