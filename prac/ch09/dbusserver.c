#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

static uint32_t count = 0;

static int hello(sd_bus_message *m, void *userdata, sd_bus_error *error)
{
	const char *client;
	char reply[40];
	int ret = 0;
	ret = sd_bus_message_read(m, "s", &client);
	if (ret < 0) {
		fprintf(stderr, "Failed to parse arguments\n");
		return ret;
	}
	sprintf(reply, "Hello %s! I have been called %d times.",client, ++count);
	return sd_bus_reply_method_return(m, "s", reply);
}

static int get_count(sd_bus *bus, const char *path, const char *interface,
const char *property, sd_bus_message *reply,
void *userdata, sd_bus_error *error)
{
	uint32_t *pcount = (uint32_t *)userdata;
	return sd_bus_message_append(reply, "u", *pcount);
}

/*
* 인터페이스를 만드는 매크로
* 참고:
https://www.freedesktop.org/software/systemd/man/sd_bus_add
_object.html
*/

static const sd_bus_vtable dbusexmaple_vtable[] = {
	SD_BUS_VTABLE_START(0), /* 구조체 시작 매크로 */
	SD_BUS_METHOD_WITH_ARGS( /* method 인터페이스 */
			"Hello", /* method 이름 */
			SD_BUS_ARGS("s", client), /* 인자(ex."s"=string) */
			SD_BUS_RESULT("s", reply), /* 결과값 */
			hello, /* 콜백 함수 */
			SD_BUS_VTABLE_UNPRIVILEGED), /* interface 공개 */
	SD_BUS_PROPERTY("Count", "u", get_count, 0,
			SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_SIGNAL("Tick", "u", 0),
	SD_BUS_VTABLE_END /* 구조체 종료 매크로 */
};

int main()
{
	sd_bus_slot *slot = NULL;
	sd_bus *bus = NULL;
	int ret = 0;
	uint32_t tickcount = 0;
	/* session 버스에 연결 */
	ret = sd_bus_default_user(&bus);
	if (ret < 0) {
		fprintf(stderr, "Failed to connect to dbus-daemon\n");
		goto finish;
	}

	/* Bus Name 요청 */
	ret = sd_bus_request_name(bus, "org.example.MyService", 0);
	if (ret < 0) {
			fprintf(stderr, "Failed to acquire name\n");
			goto finish;
	}

	/* Object Path와 Interface 등록 */
	ret = sd_bus_add_object_vtable(bus, &slot,
			"/org/example/MyService", /* 객체 경로 */
			"org.example.MyService", /* 인터페이스 이름 */
			dbusexmaple_vtable, &count);
	if (ret < 0) {
		fprintf(stderr, "Failed to add object interface\n");
		goto finish;
	}

	for (;;) {
		ret = sd_bus_process(bus, NULL);
		if (ret < 0) {
			fprintf(stderr, "Failed to process bus\n");
			break;
		}

		if (ret == 0) {
			ret = sd_bus_wait(bus, (uint64_t)(1000 * 1000)); //1s
		}

		static time_t last = 0;
		time_t now = time(NULL);
		if (last == 0 || now - last >= 1) {
			tickcount++;
			ret = sd_bus_emit_signal(bus,
					"/org/example/MyService",
					"org.example.MyService", "Tick", "u", tickcount);
			if (ret < 0) {
				fprintf(stderr, "Failed to emit signal: %s\n",
						strerror(-ret));
				goto finish;
			}
			fprintf(stderr, "[server] Tick(count=%u)\n",tickcount);
			last = now;
		}

	}

finish:
	sd_bus_slot_unref(slot);
	sd_bus_unref(bus);
	return 0;
}
