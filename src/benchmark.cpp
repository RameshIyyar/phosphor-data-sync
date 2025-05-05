#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <array>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>
#include <cstdio>
#include <sys/resource.h>

extern char **environ;

using Clock = std::chrono::steady_clock;

void make_fd_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1)
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// ========== fork + exec ==========
void fork_exec() {
    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/true", "true", nullptr);
        _exit(127);
    }
    waitpid(pid, nullptr, 0);
}

// ========== vfork + exec ==========
void vfork_exec() {
    pid_t pid = vfork();
    if (pid == 0) {
        execl("/bin/true", "true", nullptr);
        _exit(127);
    }
    waitpid(pid, nullptr, 0);
}

// ========== posix_spawn ==========
void posix_spawn_exec() {
    pid_t pid;
    const char* argv[] = {"/bin/true", nullptr};
    posix_spawn(&pid, "/bin/true", nullptr, nullptr, const_cast<char**>(argv), environ);
    waitpid(pid, nullptr, 0);
}

// ========== popen ==========
void popen_exec() {
    FILE* pipe = popen("/bin/true", "r");
    if (pipe) {
        int fd = fileno(pipe);
        make_fd_nonblocking(fd);
        pclose(pipe);
    }
}

#if 0
template<typename Func>
void benchmark(const std::string& label, Func func, int iterations = 1000) {
    auto start = Clock::now();
    for (int i = 0; i < iterations; ++i)
        func();
    auto end = Clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << label << ": " << duration << " ms\n";
}
#else
template<typename Func>
void benchmark(const std::string& label, Func func, int iterations = 1000) {
    // Clear previous usage stats
    struct rusage usage_start{};
    getrusage(RUSAGE_CHILDREN, &usage_start);

    auto start = Clock::now();
    for (int i = 0; i < iterations; ++i)
        func();
    auto end = Clock::now();

    struct rusage usage_end{};
    getrusage(RUSAGE_CHILDREN, &usage_end);

    // Compute elapsed time
    auto duration = std::chrono::duration<double, std::milli>(end - start).count();

    // Compute memory usage difference (in kilobytes)
    long rss_diff = usage_end.ru_maxrss - usage_start.ru_maxrss;
    double avg_rss_per_process = static_cast<double>(rss_diff) / iterations;

    std::cout << label << ": " << duration << " ms"
              << ", Avg RSS: " << avg_rss_per_process << " KB\n";
}
#endif

int main() {
    constexpr int runs = 1;
    benchmark("fork        ", fork_exec, runs);
    benchmark("vfork       ", vfork_exec, runs);
    benchmark("posix_spawn ", posix_spawn_exec, runs);
    benchmark("popen       ", popen_exec, runs);
    return 0;
}
