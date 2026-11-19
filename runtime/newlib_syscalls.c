#define _POSIX_C_SOURCE 200809L

#include <maia/arch/irq.h>
#include <maia/console.h>
#include <maia/drivers/serial_tx.h>
#include <maia/kernel.h>

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static int fail_with_errno(int error) {
    errno = error;
    return -1;
}

int _write(int fd, const char *buffer, int length) {
    if (fd != STDOUT_FILENO && fd != STDERR_FILENO) return fail_with_errno(EBADF);
    if (length < 0) return fail_with_errno(EINVAL);
    if (length == 0) return 0;
    if (buffer == NULL) return fail_with_errno(EFAULT);
    if (!maia_serial_tx_may_write(NULL)) return fail_with_errno(EBUSY);

    int written = 0;
    while (written < length) {
        int status = maia_serial_tx_try_put(NULL, MAIA_SERIAL_TX_USB,
                                           (uint8_t)buffer[written]);
        if (status != 0) {
            int error = status == -1 ? EAGAIN : status == -2 ? EBUSY : EIO;
            (void)maia_serial_tx_flush(NULL, MAIA_SERIAL_TX_USB);
            errno = error;
            return written != 0 ? written : -1;
        }
        ++written;
    }

    if (maia_serial_tx_flush(NULL, MAIA_SERIAL_TX_USB) != 0) {
        errno = EBUSY;
        return written != 0 ? written : -1;
    }
    return written;
}

int _read(int fd, char *buffer, int length) {
    if (fd != STDIN_FILENO) return fail_with_errno(EBADF);
    if (length < 0) return fail_with_errno(EINVAL);
    if (length == 0) return 0;
    if (buffer == NULL) return fail_with_errno(EFAULT);

    int received = 0;
    while (received < length) {
        int byte = maia_console_getc_nonblocking();
        if (byte < 0) {
            if (received == 0) return fail_with_errno(EAGAIN);
            break;
        }
        buffer[received++] = (char)(uint8_t)byte;
    }
    return received;
}

int clock_gettime(clockid_t clock_id, struct timespec *time) {
    if (time == NULL) return fail_with_errno(EFAULT);
    if (clock_id != CLOCK_MONOTONIC) return fail_with_errno(EINVAL);

    uint64_t elapsed_us = maia_time_us();
    time->tv_sec = (time_t)(elapsed_us / 1000000ULL);
    time->tv_nsec = (long)((elapsed_us % 1000000ULL) * 1000ULL);
    return 0;
}

int nanosleep(const struct timespec *request, struct timespec *remaining) {
    if (request == NULL) return fail_with_errno(EFAULT);
    if (request->tv_sec < 0 || request->tv_nsec < 0 || request->tv_nsec >= 1000000000L) {
        return fail_with_errno(EINVAL);
    }
    if (request->tv_sec == 0 && request->tv_nsec == 0) {
        if (remaining != NULL) {
            remaining->tv_sec = 0;
            remaining->tv_nsec = 0;
        }
        return 0;
    }
    if (maia_task_current_id() < 0) return fail_with_errno(ENOTSUP);

    uint64_t ticks = (uint64_t)request->tv_sec * 1000ULL;
    ticks += ((uint64_t)request->tv_nsec + 999999ULL) / 1000000ULL;
    if (ticks > INT32_MAX) return fail_with_errno(EINVAL);
    maia_task_delay_ticks((uint32_t)ticks);
    if (remaining != NULL) {
        remaining->tv_sec = 0;
        remaining->tv_nsec = 0;
    }
    return 0;
}

int _close(int fd) {
    (void)fd;
    return fail_with_errno(ENOSYS);
}

int _fstat(int fd, struct stat *st) {
    if (fd < STDIN_FILENO || fd > STDERR_FILENO) return fail_with_errno(EBADF);
    if (st == NULL) return fail_with_errno(EFAULT);
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFCHR;
    st->st_nlink = 1;
    st->st_blksize = 1;
    return 0;
}

off_t _lseek(int fd, off_t offset, int whence) {
    (void)offset;
    (void)whence;
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
        errno = ESPIPE;
        return (off_t)-1;
    }
    errno = EBADF;
    return (off_t)-1;
}

int _open(const char *path, int flags, int mode) {
    (void)path;
    (void)flags;
    (void)mode;
    return fail_with_errno(ENOSYS);
}

int _rename(const char *old_path, const char *new_path) {
    (void)old_path;
    (void)new_path;
    return fail_with_errno(ENOSYS);
}

int _kill(int pid, int signal) {
    (void)pid;
    (void)signal;
    return fail_with_errno(ENOSYS);
}

int _getpid(void) {
    return 1;
}

int _unlink(const char *path) {
    (void)path;
    return fail_with_errno(ENOSYS);
}

int _stat(const char *path, struct stat *st) {
    (void)path;
    (void)st;
    return fail_with_errno(ENOSYS);
}

int _isatty(int fd) {
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) return 1;
    errno = EBADF;
    return 0;
}

int mkdir(const char *path, mode_t mode) {
    (void)path;
    (void)mode;
    return fail_with_errno(ENOSYS);
}

int rmdir(const char *path) {
    (void)path;
    return fail_with_errno(ENOSYS);
}

DIR *opendir(const char *path) {
    (void)path;
    errno = ENOSYS;
    return NULL;
}

struct dirent *readdir(DIR *directory) {
    if (directory == NULL) errno = EBADF;
    else errno = ENOSYS;
    return NULL;
}

int closedir(DIR *directory) {
    if (directory == NULL) return fail_with_errno(EBADF);
    return fail_with_errno(ENOSYS);
}

__attribute__((noreturn)) void _exit(int status) {
    (void)status;
#if defined(__riscv)
    (void)maia_arch_irq_save();
    for (;;) __asm__ volatile("wfi");
#else
    __builtin_trap();
#endif
}
