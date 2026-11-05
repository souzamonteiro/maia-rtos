#include <maia_shell/host_v1.h>
#include <stdio.h>
#include <string.h>

#define FAKE_MAX_FDS   8U
#define FAKE_BUF_CAP   128U
#define FAKE_CWD_CAP   64U

typedef struct fake_fd_entry_t {
    int in_use;
    uint32_t flags;
    uint8_t data[FAKE_BUF_CAP];
    uint32_t length;
    uint32_t offset;
    int force_would_block;
} fake_fd_entry_t;

typedef struct fake_host_state_t {
    fake_fd_entry_t fds[FAKE_MAX_FDS];
    char cwd[FAKE_CWD_CAP];
    msh_job_t last_job;
    int32_t next_exit_status;
    int job_active;
} fake_host_state_t;

static void fake_host_reset(fake_host_state_t *st) {
    memset(st, 0, sizeof(*st));
    /* Reserve stdin(0), stdout(1), stderr(2) */
    for (uint32_t i = 0; i < 3U; ++i) {
        st->fds[i].in_use = 1;
        st->fds[i].flags = (i == 0U) ? (uint32_t)MSH_O_READ : (uint32_t)MSH_O_WRITE;
    }
    strcpy(st->cwd, "/");
}

static int32_t fake_read(void *user, msh_fd_t fd, void *dst, uint32_t n) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (st == NULL || (dst == NULL && n > 0U) || fd >= FAKE_MAX_FDS || !st->fds[fd].in_use) {
        return MSH_ERR_INVALID;
    }
    if ((st->fds[fd].flags & (uint32_t)MSH_O_READ) == 0U) {
        return MSH_ERR_PERM;
    }
    if (st->fds[fd].force_would_block) {
        return MSH_ERR_AGAIN;
    }
    uint32_t avail = st->fds[fd].length - st->fds[fd].offset;
    uint32_t count = (n < avail) ? n : avail;
    if (count > 0U) {
        memcpy(dst, st->fds[fd].data + st->fds[fd].offset, count);
        st->fds[fd].offset += count;
    }
    return (int32_t)count;
}

static int32_t fake_write(void *user, msh_fd_t fd, const void *src, uint32_t n) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (st == NULL || (src == NULL && n > 0U) || fd >= FAKE_MAX_FDS || !st->fds[fd].in_use) {
        return MSH_ERR_INVALID;
    }
    if ((st->fds[fd].flags & (uint32_t)MSH_O_WRITE) == 0U) {
        return MSH_ERR_PERM;
    }
    if (st->fds[fd].force_would_block) {
        return MSH_ERR_AGAIN;
    }
    if (n > FAKE_BUF_CAP - st->fds[fd].length) {
        return MSH_ERR_LIMIT;
    }
    if (n > 0U) {
        memcpy(st->fds[fd].data + st->fds[fd].length, src, n);
        st->fds[fd].length += n;
    }
    return (int32_t)n;
}

static int32_t fake_open(void *user, const char *path, uint32_t flags, msh_fd_t *out) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (st == NULL || path == NULL || path[0] != '/' || out == NULL) {
        return MSH_ERR_INVALID;
    }
    if (strcmp(path, "/missing") == 0 && (flags & (uint32_t)MSH_O_CREAT) == 0U) {
        return MSH_ERR_NOENT;
    }
    for (uint32_t i = 3U; i < FAKE_MAX_FDS; ++i) {
        if (!st->fds[i].in_use) {
            memset(&st->fds[i], 0, sizeof(st->fds[i]));
            st->fds[i].in_use = 1;
            st->fds[i].flags = flags;
            *out = i;
            return MSH_OK;
        }
    }
    return MSH_ERR_LIMIT;
}

static int32_t fake_close(void *user, msh_fd_t fd) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (st == NULL || fd >= FAKE_MAX_FDS || !st->fds[fd].in_use) {
        return MSH_ERR_INVALID;
    }
    st->fds[fd].in_use = 0;
    return MSH_OK;
}

static int32_t fake_pipe_create(void *user, msh_fd_t *read_end, msh_fd_t *write_end) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (st == NULL || read_end == NULL || write_end == NULL) {
        return MSH_ERR_INVALID;
    }
    msh_fd_t r = MSH_FD_INVALID;
    msh_fd_t w = MSH_FD_INVALID;
    for (uint32_t i = 3U; i < FAKE_MAX_FDS; ++i) {
        if (!st->fds[i].in_use) {
            if (r == MSH_FD_INVALID) {
                r = i;
            } else {
                w = i;
                break;
            }
        }
    }
    if (r == MSH_FD_INVALID || w == MSH_FD_INVALID) {
        return MSH_ERR_LIMIT;
    }
    memset(&st->fds[r], 0, sizeof(st->fds[r]));
    memset(&st->fds[w], 0, sizeof(st->fds[w]));
    st->fds[r].in_use = 1;
    st->fds[r].flags = MSH_O_READ;
    st->fds[w].in_use = 1;
    st->fds[w].flags = MSH_O_WRITE;
    *read_end = r;
    *write_end = w;
    return MSH_OK;
}

static int32_t fake_fd_dup(void *user, msh_fd_t src, msh_fd_t *dst) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (st == NULL || dst == NULL || src >= FAKE_MAX_FDS || !st->fds[src].in_use) {
        return MSH_ERR_INVALID;
    }
    for (uint32_t i = 3U; i < FAKE_MAX_FDS; ++i) {
        if (!st->fds[i].in_use) {
            st->fds[i] = st->fds[src];
            *dst = i;
            return MSH_OK;
        }
    }
    return MSH_ERR_LIMIT;
}

static int32_t fake_spawn(void *user, const char *path, const char *const *argv,
                          msh_fd_t in, msh_fd_t out, msh_fd_t err, msh_job_t *job) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (st == NULL || path == NULL || argv == NULL || argv[0] == NULL || job == NULL) {
        return MSH_ERR_INVALID;
    }
    if (in >= FAKE_MAX_FDS || out >= FAKE_MAX_FDS || err >= FAKE_MAX_FDS ||
        !st->fds[in].in_use || !st->fds[out].in_use || !st->fds[err].in_use) {
        return MSH_ERR_INVALID;
    }
    if (strcmp(path, "/bin/unknown") == 0) {
        return MSH_ERR_NOENT;
    }
    /* Simulate writing first operand to stdout if present */
    if (argv[1] != NULL) {
        int32_t wr = fake_write(user, out, argv[1], (uint32_t)strlen(argv[1]));
        if (wr < 0) {
            return wr;
        }
    }
    st->last_job += 1U;
    st->job_active = 1;
    *job = st->last_job;
    return MSH_OK;
}

static int32_t fake_wait(void *user, msh_job_t job, int32_t *status) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (st == NULL || status == NULL || job == MSH_JOB_INVALID || !st->job_active || job != st->last_job) {
        return MSH_ERR_INVALID;
    }
    st->job_active = 0;
    *status = st->next_exit_status;
    return MSH_OK;
}

static int32_t fake_cwd_get(void *user, char *dst, uint32_t cap) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (st == NULL || dst == NULL || cap == 0U) {
        return MSH_ERR_INVALID;
    }
    uint32_t len = (uint32_t)strlen(st->cwd);
    if (len + 1U > cap) {
        return MSH_ERR_LIMIT;
    }
    memcpy(dst, st->cwd, len + 1U);
    return (int32_t)len;
}

static int32_t fake_cwd_set(void *user, const char *path) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (st == NULL || path == NULL || path[0] != '/') {
        return MSH_ERR_INVALID;
    }
    if (strcmp(path, "/notadir") == 0) {
        return MSH_ERR_NOTDIR;
    }
    uint32_t len = (uint32_t)strlen(path);
    if (len + 1U > FAKE_CWD_CAP) {
        return MSH_ERR_LIMIT;
    }
    memcpy(st->cwd, path, len + 1U);
    return MSH_OK;
}

static int32_t fake_glob(void *user, const char *pattern, void *sink_ptr) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    const msh_glob_sink_t *sink = (const msh_glob_sink_t *)sink_ptr;
    if (st == NULL || pattern == NULL || sink == NULL || sink->on_match == NULL) {
        return MSH_ERR_INVALID;
    }
    static const char *const kEntries[] = { "alpha.txt", "beta.txt", "sub" };
    static const uint32_t kTypes[] = { MSH_ENTRY_FILE, MSH_ENTRY_FILE, MSH_ENTRY_DIR };
    for (size_t i = 0; i < 3U; ++i) {
        int32_t rc = sink->on_match(sink->ctx, kEntries[i], (uint32_t)strlen(kEntries[i]), kTypes[i]);
        if (rc < 0) {
            return rc;
        }
    }
    return 3;
}

static void init_valid_host(msh_host_v1 *host, fake_host_state_t *state) {
    host->struct_size = (uint32_t)sizeof(msh_host_v1);
    host->version = MSH_HOST_V1_VERSION;
    host->user = state;
    host->read = fake_read;
    host->write = fake_write;
    host->open = fake_open;
    host->close = fake_close;
    host->pipe_create = fake_pipe_create;
    host->fd_dup = fake_fd_dup;
    host->spawn = fake_spawn;
    host->wait = fake_wait;
    host->cwd_get = fake_cwd_get;
    host->cwd_set = fake_cwd_set;
    host->glob = fake_glob;
}

typedef struct glob_collector_t {
    uint32_t count;
    uint32_t max_allowed;
} glob_collector_t;

static int32_t collect_glob_match(void *ctx, const char *path, uint32_t length, uint32_t entry_type) {
    glob_collector_t *c = (glob_collector_t *)ctx;
    if (c == NULL || path == NULL || length == 0U ||
        (entry_type != MSH_ENTRY_FILE && entry_type != MSH_ENTRY_DIR)) {
        return MSH_ERR_INVALID;
    }
    if (c->count >= c->max_allowed) {
        return MSH_ERR_LIMIT;
    }
    c->count += 1U;
    return MSH_OK;
}

int main(void) {
    fake_host_state_t state;
    msh_host_v1 host;

    /* 1. Verify open flags match shell.cpp constants (1, 14, 22) */
    if ((uint32_t)MSH_O_READ != 1U) {
        return 1;
    }
    if (((uint32_t)MSH_O_WRITE | (uint32_t)MSH_O_CREAT | (uint32_t)MSH_O_TRUNC) != 14U) {
        return 2;
    }
    if (((uint32_t)MSH_O_WRITE | (uint32_t)MSH_O_CREAT | (uint32_t)MSH_O_APPEND) != 22U) {
        return 3;
    }

    /* 2. Positive & negative ABI validation tests */
    fake_host_reset(&state);
    init_valid_host(&host, &state);
    if (msh_host_v1_validate(&host) != MSH_OK) {
        return 4;
    }
    if (msh_host_v1_validate(NULL) != MSH_ERR_INVALID) {
        return 5;
    }

    host.struct_size = (uint32_t)sizeof(msh_host_v1) - 4U;
    if (msh_host_v1_validate(&host) != MSH_ERR_ABI) {
        return 6;
    }
    host.struct_size = (uint32_t)sizeof(msh_host_v1);

    host.version = (2U << 16U) | 0U;
    if (msh_host_v1_validate(&host) != MSH_ERR_ABI) {
        return 7;
    }
    /* Minor version bump must remain compatible */
    host.version = (MSH_HOST_V1_VERSION_MAJOR << 16U) | 7U;
    if (msh_host_v1_validate(&host) != MSH_OK) {
        return 8;
    }
    host.version = MSH_HOST_V1_VERSION;

    /* Negative test: NULL callback rejection */
    host.spawn = NULL;
    if (msh_host_v1_validate(&host) != MSH_ERR_INVALID) {
        return 9;
    }
    host.spawn = fake_spawn;

    /* 3. VFS open/write/read/close + error propagation */
    msh_fd_t fd = MSH_FD_INVALID;
    if (host.open(host.user, "/missing", MSH_O_READ, &fd) != MSH_ERR_NOENT) {
        return 10;
    }
    if (host.open(host.user, "relative_rejected", MSH_O_READ, &fd) != MSH_ERR_INVALID) {
        return 11;
    }
    if (host.open(host.user, "/hello.txt", MSH_O_READ | MSH_O_WRITE | MSH_O_CREAT | MSH_O_TRUNC, &fd) != MSH_OK) {
        return 12;
    }
    if (host.write(host.user, fd, "maia", 4U) != 4) {
        return 13;
    }
    /* Would-block (-6) propagation check */
    state.fds[fd].force_would_block = 1;
    char buf[16];
    if (host.read(host.user, fd, buf, sizeof(buf)) != MSH_ERR_AGAIN) {
        return 14;
    }
    state.fds[fd].force_would_block = 0;
    if (host.read(host.user, fd, buf, sizeof(buf)) != 4 || memcmp(buf, "maia", 4U) != 0) {
        return 15;
    }

    /* Duplicate FD and close both */
    msh_fd_t dup_fd = MSH_FD_INVALID;
    if (host.fd_dup(host.user, fd, &dup_fd) != MSH_OK) {
        return 16;
    }
    if (host.close(host.user, fd) != MSH_OK || host.close(host.user, dup_fd) != MSH_OK) {
        return 17;
    }
    if (host.close(host.user, fd) != MSH_ERR_INVALID) {
        return 18;
    }

    /* 4. Pipe creation + spawn + wait contract */
    msh_fd_t pipe_r = MSH_FD_INVALID;
    msh_fd_t pipe_w = MSH_FD_INVALID;
    if (host.pipe_create(host.user, &pipe_r, &pipe_w) != MSH_OK) {
        return 19;
    }
    const char *const argv_ok[] = { "echo", "piped", NULL };
    msh_job_t job = MSH_JOB_INVALID;
    if (host.spawn(host.user, "/bin/unknown", argv_ok, MSH_FD_STDIN, pipe_w, MSH_FD_STDERR, &job) != MSH_ERR_NOENT) {
        return 20;
    }
    state.next_exit_status = 0;
    if (host.spawn(host.user, "/bin/echo", argv_ok, MSH_FD_STDIN, pipe_w, MSH_FD_STDERR, &job) != MSH_OK) {
        return 21;
    }
    int32_t exit_status = -1;
    if (host.wait(host.user, job, &exit_status) != MSH_OK || exit_status != 0) {
        return 22;
    }
    /* Waiting a second time on a reaped job must fail */
    if (host.wait(host.user, job, &exit_status) != MSH_ERR_INVALID) {
        return 23;
    }
    if (host.close(host.user, pipe_r) != MSH_OK || host.close(host.user, pipe_w) != MSH_OK) {
        return 24;
    }

    /* 5. CWD get/set positive and negative tests */
    if (host.cwd_set(host.user, "/notadir") != MSH_ERR_NOTDIR) {
        return 25;
    }
    if (host.cwd_set(host.user, "/home") != MSH_OK) {
        return 26;
    }
    char cwd_buf[8];
    if (host.cwd_get(host.user, cwd_buf, 3U) != MSH_ERR_LIMIT) {
        return 27;
    }
    if (host.cwd_get(host.user, cwd_buf, sizeof(cwd_buf)) != 5 || strcmp(cwd_buf, "/home") != 0) {
        return 28;
    }

    /* 6. Glob sink enumeration and limit abort */
    glob_collector_t collector = { 0U, 10U };
    msh_glob_sink_t sink = { &collector, collect_glob_match };
    if (host.glob(host.user, "*.txt", &sink) != 3 || collector.count != 3U) {
        return 29;
    }
    collector.count = 0U;
    collector.max_allowed = 2U;
    if (host.glob(host.user, "*", &sink) != MSH_ERR_LIMIT || collector.count != 2U) {
        return 30;
    }

    puts("MaiaShellHost v1: host contract PASS (ABI, FD, pipe, spawn/wait, CWD, glob)");
    return 0;
}

