/* dbusclient.c */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <systemd/sd-bus.h>

static int on_tick(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    uint32_t count;
    int r = sd_bus_message_read(m, "u", &count);
    if (r < 0) return r;

    printf("[client] Tick(count=%u)\n", count);
    return 0;
}

int main()
{
    sd_bus *bus = NULL;
    sd_bus_slot *slot = NULL;
    sd_bus_message *reply = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;

    int r;
    r = sd_bus_default_user(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to bus\n");
        return 1;
    }

    /* Hello 호출 */
    r = sd_bus_call_method(bus,
                           "org.example.NotifyService",
                           "/org/example/NotifyService",
                           "org.example.Notify",
                           "Hello",
                           &error,
                           &reply,
                           "s",
                           "client");
    if (r < 0) {
        fprintf(stderr, "Failed Hello call: %s\n", strerror(-r));
        return 1;
    }

    const char *resp;
    sd_bus_message_read(reply, "s", &resp);
    printf("[client] Hello response: %s\n", resp);
    sd_bus_message_unref(reply);

    /* Tick 신호 구독 */
    r = sd_bus_add_match(bus,
                         &slot,
                         "type='signal',interface='org.example.Notify',member='Tick'",
                         on_tick,
                         NULL);

    if (r < 0) {
        fprintf(stderr, "Failed to add match: %s\n", strerror(-r));
        return 1;
    }

    printf("[client] Listening Tick signals for 10 seconds...\n");

    for (int i = 0; i < 10; i++) {
        r = sd_bus_process(bus, NULL);
        if (r < 0) break;

        sd_bus_wait(bus, (uint64_t)(1000 * 1000));
    }

    sd_bus_slot_unref(slot);
    sd_bus_unref(bus);
    sd_bus_error_free(&error);
    return 0;
}
