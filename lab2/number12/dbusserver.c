/* dbusserver.c */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <systemd/sd-bus.h>

static uint32_t g_count = 0;

/* -----------------------------
   Hello 메서드
------------------------------ */
static int method_hello(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    const char *client;
    int r;

    r = sd_bus_message_read(m, "s", &client);
    if (r < 0) {
        fprintf(stderr, "Failed to parse arguments: %s\n", strerror(-r));
        return r;
    }

    printf("[server] Hello() from client=%s\n", client);

    char reply[128];
    snprintf(reply, sizeof(reply), "Hello %s! I have been called %u times.", client, g_count);

    return sd_bus_reply_method_return(m, "s", reply);
}

/* -----------------------------
   Count 프로퍼티 Getter
------------------------------ */
static int get_count(sd_bus *bus,
                     const char *path,
                     const char *interface,
                     const char *property,
                     sd_bus_message *reply,
                     void *userdata,
                     sd_bus_error *ret_error)
{
    uint32_t *p = (uint32_t *)userdata;
    return sd_bus_message_append(reply, "u", *p);
}

/* -----------------------------
   인터페이스 테이블 정의
------------------------------ */
static const sd_bus_vtable notify_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Hello", "s", "s", method_hello, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_PROPERTY("Count", "u", get_count, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_SIGNAL("Tick", "u", 0),
    SD_BUS_VTABLE_END
};

/* -----------------------------
   메인 서버 로직
------------------------------ */
int main()
{
    sd_bus *bus = NULL;
    sd_bus_slot *slot = NULL;
    int r;

    r = sd_bus_default_user(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to bus: %s\n", strerror(-r));
        return 1;
    }

    r = sd_bus_request_name(bus, "org.example.NotifyService", 0);
    if (r < 0) {
        fprintf(stderr, "Failed to request name: %s\n", strerror(-r));
        return 1;
    }

    r = sd_bus_add_object_vtable(bus, &slot,
                                 "/org/example/NotifyService",
                                 "org.example.Notify",
                                 notify_vtable,
                                 &g_count);

    if (r < 0) {
        fprintf(stderr, "Failed to add object: %s\n", strerror(-r));
        return 1;
    }

    printf("[server] NotifyService started.\n");

    time_t last = 0;

    while (1) {
        r = sd_bus_process(bus, NULL);
        if (r < 0) break;
        if (r > 0) continue;

        time_t now = time(NULL);
        if (now != last) {
            last = now;
            g_count++;
            printf("[server] Tick(count=%u)\n", g_count);

            sd_bus_emit_signal(bus,
                               "/org/example/NotifyService",
                               "org.example.Notify",
                               "Tick",
                               "u", g_count);
        }

        sd_bus_wait(bus, (uint64_t)(1000 * 1000));
    }

    sd_bus_slot_unref(slot);
    sd_bus_unref(bus);
    return 0;
}
