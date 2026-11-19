#define _POSIX_C_SOURCE 200809L

#include <maia/console.h>
#include <maia/drivers/serial_tx.h>
#include <maia/kernel.h>

#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static char output[64];
static size_t output_length;
static const char input[] = {'m', 'a', 'i', 'a'};
static size_t input_offset;
static int tx_writable = 1;
static int tx_busy_after = -1;
static uint64_t monotonic_us = 1234567ULL;
static maia_task_id_t current_task_id = 1;
static uint32_t delayed_ticks;

int _write(int fd, const char *buffer, int length);
int _read(int fd, char *buffer, int length);
int _fstat(int fd, struct stat *st);
off_t _lseek(int fd, off_t offset, int whence);
int _open(const char *path, int flags, int mode);
int _stat(const char *path, struct stat *st);
int _unlink(const char *path);
int _rename(const char *old_path, const char *new_path);
int _isatty(int fd);
int _getpid(void);
int _kill(int pid, int signal);

int maia_serial_tx_may_write(const void *owner) {
    (void)owner;
    return tx_writable;
}

maia_task_id_t maia_task_current_id(void) {
    return current_task_id;
}

void maia_task_delay_ticks(uint32_t ticks) {
    delayed_ticks = ticks;
}

int maia_serial_tx_try_put(const void *owner, maia_serial_tx_sink_t sink, uint8_t byte) {
    (void)owner;
    if (sink != MAIA_SERIAL_TX_USB) return -3;
    if (tx_busy_after >= 0 && output_length >= (size_t)tx_busy_after) return -1;
    output[output_length++] = (char)byte;
    return 0;
}

int maia_serial_tx_flush(const void *owner, maia_serial_tx_sink_t sink) {
    (void)owner;
    return sink == MAIA_SERIAL_TX_USB ? 0 : -3;
}

int maia_console_getc_nonblocking(void) {
    if (input_offset >= sizeof(input)) return -1;
    return (unsigned char)input[input_offset++];
}

uint64_t maia_time_us(void) {
    return monotonic_us;
}

int main(void) {
    struct timespec time;
    if (clock_gettime(CLOCK_MONOTONIC, &time) != 0 ||
        time.tv_sec != 1 || time.tv_nsec != 234567000L) return 1;
    if (clock_gettime(CLOCK_REALTIME, &time) != -1 || errno != EINVAL) return 2;
    if (clock_gettime(CLOCK_MONOTONIC, NULL) != -1 || errno != EFAULT) return 3;
    struct timespec request = {0, 1000001L};
    struct timespec remaining = {1, 1};
    if (nanosleep(&request, &remaining) != 0 || delayed_ticks != 2U ||
        remaining.tv_sec != 0 || remaining.tv_nsec != 0) return 4;
    request.tv_nsec = 1000000000L;
    if (nanosleep(&request, NULL) != -1 || errno != EINVAL) return 5;
    request.tv_nsec = 1L;
    current_task_id = -1;
    if (nanosleep(&request, NULL) != -1 || errno != ENOTSUP) return 6;
    current_task_id = 1;

    if (_write(STDOUT_FILENO, "out", 3) != 3 ||
        output_length != 3U || memcmp(output, "out", 3U) != 0) return 7;
    tx_writable = 0;
    if (_write(STDERR_FILENO, "x", 1) != -1 || errno != EBUSY) return 8;
    tx_writable = 1;
    tx_busy_after = 4;
    if (_write(STDOUT_FILENO, "more", 4) != 1 ||
        errno != EAGAIN || output_length != 4U || output[3] != 'm') return 9;
    if (_write(3, "x", 1) != -1 || errno != EBADF) return 10;
    if (_write(STDOUT_FILENO, NULL, 0) != 0) return 11;
    if (_write(STDOUT_FILENO, NULL, 1) != -1 || errno != EFAULT) return 12;

    char buffer[8] = {0};
    if (_read(STDIN_FILENO, buffer, sizeof buffer) != 4 ||
        memcmp(buffer, input, sizeof input) != 0) return 13;
    if (_read(STDIN_FILENO, buffer, sizeof buffer) != -1 || errno != EAGAIN) return 14;
    if (_read(1, buffer, 1) != -1 || errno != EBADF) return 15;

    struct stat st;
    if (_fstat(STDOUT_FILENO, &st) != 0 || !S_ISCHR(st.st_mode)) return 16;
    if (_fstat(3, &st) != -1 || errno != EBADF) return 17;
    if (_lseek(STDIN_FILENO, 0, 0) != (off_t)-1 || errno != ESPIPE) return 18;
    if (_open("/sd/file", 0, 0) != -1 || errno != ENOSYS) return 19;
    if (_stat("/sd", &st) != -1 || errno != ENOSYS) return 20;
    if (_unlink("/sd/file") != -1 || errno != ENOSYS) return 21;
    if (_rename("/a", "/b") != -1 || errno != ENOSYS) return 22;
    if (mkdir("/sd/new", 0) != -1 || errno != ENOSYS) return 23;
    if (rmdir("/sd/new") != -1 || errno != ENOSYS) return 24;
    if (opendir("/sd") != NULL || errno != ENOSYS) return 25;
    if (readdir(NULL) != NULL || errno != EBADF) return 26;
    if (closedir(NULL) != -1 || errno != EBADF) return 27;
    if (_isatty(STDIN_FILENO) != 1 || _isatty(3) != 0 || errno != EBADF) return 28;
    if (_getpid() != 1 || _kill(1, 0) != -1 || errno != ENOSYS) return 29;
    return 0;
}
