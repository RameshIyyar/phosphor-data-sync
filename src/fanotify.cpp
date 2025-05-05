#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/fanotify.h>
#include <sys/stat.h>
#include <limits.h>
#include <cstring>

int setup_fanotify() {
    int fd = fanotify_init(FAN_CLASS_NOTIF | FAN_NONBLOCK, O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        perror("fanotify_init");
        exit(EXIT_FAILURE);
    }
    return fd;
}

void add_mount_watch(int fan_fd, const char* mount_path) {
    uint64_t mask = FAN_CREATE | FAN_DELETE |
                    FAN_MOVED_FROM | FAN_MOVED_TO |  FAN_CLOSE_WRITE;

    if (fanotify_mark(fan_fd, FAN_MARK_ADD | FAN_MARK_MOUNT, mask, AT_FDCWD, mount_path) == -1) {
        perror("fanotify_mark");
        exit(EXIT_FAILURE);
    }
    std::cout << "Watching entire mount: " << mount_path << std::endl;
}

void event_loop(int fan_fd) {
    constexpr size_t buf_size = 4096;
    char buffer[buf_size];

    while (true) {
        ssize_t len = read(fan_fd, buffer, buf_size);
        if (len == -1) {
            if (errno == EAGAIN) continue;
            perror("read");
            break;
        }

        ssize_t offset = 0;
        while (offset < len) {
            auto* metadata = reinterpret_cast<fanotify_event_metadata*>(&buffer[offset]);

            if (metadata->vers != FANOTIFY_METADATA_VERSION) {
                std::cerr << "fanotify version mismatch\n";
                return;
            }

            if (metadata->fd >= 0) {
                char path[PATH_MAX];
                snprintf(path, sizeof(path), "/proc/self/fd/%d", metadata->fd);
                char resolved[PATH_MAX];
                ssize_t rlen = readlink(path, resolved, sizeof(resolved) - 1);
                if (rlen != -1) {
                    resolved[rlen] = '\0';
                    std::cout << "[mount-event] " << resolved << std::endl;
                }
                close(metadata->fd);
            }

            offset += FAN_EVENT_METADATA_LEN;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <mount_point>\n";
        return EXIT_FAILURE;
    }

    const char* mount_point = argv[1];
    int fan_fd = setup_fanotify();
    add_mount_watch(fan_fd, mount_point);
    event_loop(fan_fd);
    close(fan_fd);
    return 0;
}
