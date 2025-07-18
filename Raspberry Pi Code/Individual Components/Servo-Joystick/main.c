#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <errno.h>



int main() {
    const char *device = "/dev/input/event2"; // Change if needed
    struct libevdev *dev = NULL;
    int fd;

    fd = open(device, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open input device");
        return 1;
    }

    if (libevdev_new_from_fd(fd, &dev) < 0) {
        fprintf(stderr, "Failed to initialize libevdev from file descriptor\n");
        return 1;
    }

    printf("Reading from: %s\n", libevdev_get_name(dev));
    printf("Input device ID: bus %#x vendor %#x product %#x\n",
           libevdev_get_id_bustype(dev),
           libevdev_get_id_vendor(dev),
           libevdev_get_id_product(dev));

    struct input_event ev;
    while (1) {
        int rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == 0) {
            if (ev.type == EV_ABS) {
                printf("[AXIS ] Code: %d, Value: %d\n", ev.code, ev.value);
            } else if (ev.type == EV_KEY) {
                printf("[BUTTON] Code: %d, State: %s\n", ev.code,
                       ev.value ? "Pressed" : "Released");
            }
        } else if (rc != -EAGAIN) {
            fprintf(stderr, "Error reading event: %d\n", rc);
        }
        usleep(1000); // small delay to avoid 100% CPU usage
    }

    libevdev_free(dev);
    close(fd);
    return 0;
}
