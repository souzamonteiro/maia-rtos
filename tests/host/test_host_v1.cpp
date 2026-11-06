#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <maia_shell/host_v1.h>

typedef struct fake_host_state {
    char last_written[256];
    char current_dir[128];
    int spawn_count;
} fake_host_state_t;

static int32_t fake_write(void *user, msh_fd_t fd, const void *src, uint32_t n) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (!st || !src) return MSH_ERR_INVALID;
    uint32_t c = (n < sizeof(st->last_written) - 1) ? n : (sizeof(st->last_written) - 1);
    memcpy(st->last_written, src, c);
    st->last_written[c] = '\0';
    return (int32_t)c;
}

static int32_t fake_cwd_get(void *user, char *dst, uint32_t cap) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (!st || !dst || cap == 0) return MSH_ERR_INVALID;
    strncpy(dst, st->current_dir, cap - 1);
    dst[cap - 1] = '\0';
    return MSH_OK;
}

static int32_t fake_cwd_set(void *user, const char *path) {
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (!st || !path) return MSH_ERR_INVALID;
    strncpy(st->current_dir, path, sizeof(st->current_dir) - 1);
    st->current_dir[sizeof(st->current_dir) - 1] = '\0';
    return MSH_OK;
}

static int32_t fake_spawn(void *user, const char *path, const char *const *argv,
                          msh_fd_t in, msh_fd_t out, msh_fd_t err, msh_job_t *job) {
    (void)argv; (void)in; (void)out; (void)err;
    fake_host_state_t *st = (fake_host_state_t *)user;
    if (!st || !path || !job) return MSH_ERR_INVALID;
    st->spawn_count++;
    *job = (msh_job_t)st->spawn_count;
    return MSH_OK;
}

static int32_t fake_wait(void *user, msh_job_t job, int32_t *status) {
    (void)user; (void)job;
    if (!status) return MSH_ERR_INVALID;
    *status = 0;
    return MSH_OK;
}

int main(void) {
    printf("=== Running MaiaShellHost v1 Contract Unit Tests ===\n");

    fake_host_state_t state;
    memset(&state, 0, sizeof(state));
    strcpy(state.current_dir, "/sd/modules");

    msh_host_v1 host;
    memset(&host, 0, sizeof(host));
    host.struct_size = sizeof(msh_host_v1);
    host.version = MSH_HOST_VERSION_1;
    host.user = &state;
    host.write = fake_write;
    host.cwd_get = fake_cwd_get;
    host.cwd_set = fake_cwd_set;
    host.spawn = fake_spawn;
    host.wait = fake_wait;

    // Test 1: Size and Version
    assert(host.struct_size == sizeof(msh_host_v1));
    assert(host.version == 0x00010000U);
    printf("[PASS] Test 1: Struct size (%u bytes) and version verified\n", (unsigned int)host.struct_size);

    // Test 2: Write callback
    const char *msg = "MaiaShell Native Host Test\n";
    int32_t wrote = host.write(host.user, MSH_STDOUT_FD, msg, (uint32_t)strlen(msg));
    assert(wrote == (int32_t)strlen(msg));
    assert(strcmp(state.last_written, msg) == 0);
    printf("[PASS] Test 2: Output write dispatch verified: '%s'", state.last_written);

    // Test 3: CWD operations
    char cwd_buf[64];
    int32_t rc = host.cwd_get(host.user, cwd_buf, sizeof(cwd_buf));
    assert(rc == MSH_OK);
    assert(strcmp(cwd_buf, "/sd/modules") == 0);

    rc = host.cwd_set(host.user, "/sd/bin");
    assert(rc == MSH_OK);
    assert(strcmp(state.current_dir, "/sd/bin") == 0);
    printf("[PASS] Test 3: CWD get and set verified\n");

    // Test 4: Spawn and Wait
    msh_job_t job_id = 0;
    rc = host.spawn(host.user, "/sd/calc.mobj", NULL, MSH_STDIN_FD, MSH_STDOUT_FD, MSH_STDERR_FD, &job_id);
    assert(rc == MSH_OK);
    assert(job_id == 1);
    assert(state.spawn_count == 1);

    int32_t exit_status = -1;
    rc = host.wait(host.user, job_id, &exit_status);
    assert(rc == MSH_OK);
    assert(exit_status == 0);
    printf("[PASS] Test 4: Spawn and wait lifecycle verified (job_id: %u, status: %d)\n", job_id, exit_status);

    printf("=== ALL MaiaShellHost v1 TESTS PASSED SUCCESSFULLY! ===\n");
    return 0;
}

