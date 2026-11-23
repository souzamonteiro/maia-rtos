#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

#include <maia_shell/maia_shell.hpp>

enum class Scenario { Console, Redirect, Pipeline, AtCapacity, Overflow, PipelineOverflow, NoSink };

static Scenario scenario;
static std::string console_output;
static unsigned module_calls;

static void capture_console(const char *text) {
    if (text) console_output += text;
}

extern "C" int mnm_register_symbol(const char *, uintptr_t) {
    return 0;
}

extern "C" int mnm_get_registered_symbols(const char **, uintptr_t *, int) {
    return 0;
}

extern "C" int mnm_load_and_run(const void *elf, size_t elf_size,
                                 int, const char *const *,
                                 const maia_host_api_v1 *host, int *exit_code,
                                 void (*)(const char *)) {
    assert(elf != nullptr && elf_size != 0U);
    assert(host != nullptr && host->write_fd != nullptr && exit_code != nullptr);
    ++module_calls;

    if (scenario == Scenario::Console) {
        const char raw[] = {'a', 'b', 'c'};
        size_t before = console_output.size();
        assert(host->write_fd(1, raw, sizeof(raw)) == 3);
        assert(console_output.substr(before) == "abc");

        const std::string long_text(300, 'x');
        before = console_output.size();
        assert(host->write_fd(1, long_text.data(), long_text.size()) == 300);
        assert(console_output.substr(before) == long_text);
        before = console_output.size();
        assert(host->write_fd(2, long_text.data(), long_text.size()) == 300);
        assert(console_output.substr(before) == long_text);

        const char binary[] = {'a', '\0', 'b'};
        before = console_output.size();
        assert(host->write_fd(1, binary, sizeof(binary)) < 0);
        assert(host->write_fd(2, binary, sizeof(binary)) < 0);
        assert(host->write_fd(1, nullptr, 1) < 0);
        assert(host->write_fd(2, nullptr, 1) < 0);
        assert(host->write_fd(1, raw, UINT32_C(2147483648)) < 0);
        assert(host->write_fd(1, raw, UINT32_MAX) < 0);
        assert(host->write_fd(0, raw, sizeof(raw)) < 0);
        assert(host->write_fd(0, nullptr, 0) < 0);
        assert(host->write_fd(99, raw, sizeof(raw)) < 0);
        assert(host->write_fd(1, nullptr, 0) == 0);
        assert(host->write_fd(2, nullptr, 0) == 0);
        assert(console_output.size() == before);
    } else if (scenario == Scenario::Redirect) {
        assert(host->write_fd(1, "stdout", 6) == 6);
        assert(host->write_fd(2, "stderr", 6) == 6);
    } else if (scenario == Scenario::Pipeline) {
        assert(host->write_fd(1, "piped\n", 6) == 6);
    } else if (scenario == Scenario::AtCapacity) {
        const std::string full(65535, 'q');
        assert(host->write_fd(1, full.data(), full.size()) == 65535);
    } else if (scenario == Scenario::Overflow || scenario == Scenario::PipelineOverflow) {
        const std::string too_large(65536, 'z');
        assert(host->write_fd(1, too_large.data(), too_large.size()) < 0);
    } else {
        assert(host->write_fd(1, "x", 1) < 0);
        assert(host->write_fd(2, "x", 1) < 0);
        assert(host->write_fd(1, nullptr, 0) == 0);
    }

    *exit_code = 0;
    return MNM_LOAD_OK;
}

static std::string read_file(const std::string &path) {
    FILE *file = fopen(path.c_str(), "rb");
    assert(file != nullptr);
    char buffer[128];
    std::string result;
    size_t length;
    while ((length = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        result.append(buffer, length);
    }
    assert(!ferror(file));
    assert(fclose(file) == 0);
    return result;
}

int main() {
    MaiaShell shell;
    shell.init(capture_console);

    scenario = Scenario::Console;
    assert(shell.execute("runelf") == 0);
    assert(module_calls == 1U);

    char temp_dir[] = "/tmp/maia-mnm-write-XXXXXX";
    assert(mkdtemp(temp_dir) != nullptr);
    const std::string target = std::string(temp_dir) + "/output.txt";
    const std::string command = "runelf > " + target;

    console_output.clear();
    scenario = Scenario::Redirect;
    assert(shell.execute(command.c_str()) == 0);
    assert(read_file(target) == "stdout");
    assert(console_output == "stderr");

    console_output.clear();
    const std::string conditional = "if " + command + ";then;echo REDIR_OK;fi";
    assert(shell.execute(conditional.c_str()) == 0);
    assert(read_file(target) == "stdout");
    assert(console_output == "stderrREDIR_OK\n");

    console_output.clear();
    scenario = Scenario::Pipeline;
    assert(shell.execute("runelf | cat") == 0);
    assert(console_output == "piped\n");

    console_output.clear();
    scenario = Scenario::Overflow;
    assert(shell.execute(command.c_str()) == 1);
    assert(console_output.find("redirection buffer exhausted") != std::string::npos);
    assert(read_file(target) == "stdout");

    console_output.clear();
    scenario = Scenario::PipelineOverflow;
    assert(shell.execute("runelf | cat") == 1);
    assert(console_output.find("pipeline buffer exhausted") != std::string::npos);

    console_output.clear();
    scenario = Scenario::AtCapacity;
    assert(shell.execute(command.c_str()) == 0);
    assert(read_file(target) == std::string(65535, 'q'));

    assert(unlink(target.c_str()) == 0);
    assert(rmdir(temp_dir) == 0);

    MaiaShell no_sink;
    no_sink.init(nullptr);
    scenario = Scenario::NoSink;
    assert(no_sink.execute("runelf") == 0);
    assert(module_calls == 8U);
    return 0;
}
