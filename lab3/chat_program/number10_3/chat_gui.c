// chat_gui.c : GTK+3 채팅 클라이언트
// gcc chat_gui.c -o chat_gui `pkg-config --cflags --libs gtk+-3.0`

#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <locale.h>   // ← 추가

#define PORT 3490
#define MAXBUF 512

static GtkWidget *entry_ip;
static GtkWidget *entry_nick;
static GtkWidget *entry_room;
static GtkWidget *textview_chat;
static GtkWidget *entry_msg;
static GtkWidget *btn_connect;
static GtkWidget *btn_send;

static int sockfd = -1;
static GIOChannel *sock_channel = NULL;

/* 채팅창에 문자열 추가 */
static void append_chat_text(const char *msg)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview_chat));
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    gtk_text_buffer_insert(buffer, &end_iter, msg, -1);
    gtk_text_buffer_insert(buffer, &end_iter, "\n", -1);

    // 맨 아래로 스크롤
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(textview_chat),
                                 &end_iter, 0.0, FALSE, 0, 0);
}

/* 소켓에서 데이터 들어올 때 호출되는 콜백 */
static gboolean socket_io_cb(GIOChannel *source,
                             GIOCondition condition,
                             gpointer data)
{
    if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
        append_chat_text("** disconnected from server **");
        if (sock_channel) {
            g_io_channel_shutdown(sock_channel, FALSE, NULL);
            g_io_channel_unref(sock_channel);
            sock_channel = NULL;
        }
        if (sockfd >= 0) {
            close(sockfd);
            sockfd = -1;
        }
        gtk_widget_set_sensitive(btn_connect, TRUE);
        gtk_widget_set_sensitive(btn_send, FALSE);
        return FALSE;  // watch 삭제
    }

    char buf[MAXBUF + 1];
    ssize_t n = recv(sockfd, buf, MAXBUF, 0);
    if (n <= 0) {
        append_chat_text("** server closed connection **");
        if (sock_channel) {
            g_io_channel_shutdown(sock_channel, FALSE, NULL);
            g_io_channel_unref(sock_channel);
            sock_channel = NULL;
        }
        if (sockfd >= 0) {
            close(sockfd);
            sockfd = -1;
        }
        gtk_widget_set_sensitive(btn_connect, TRUE);
        gtk_widget_set_sensitive(btn_send, FALSE);
        return FALSE;
    }

    buf[n] = '\0';

    // 여러 줄이 올 수 있으니 그대로 추가
    char *p = buf;
    while (*p) {
        char *nl = strchr(p, '\n');
        if (nl) *nl = '\0';
        append_chat_text(p);
        if (!nl) break;
        p = nl + 1;
    }

    return TRUE;  // 계속 감시
}

/* 전송 버튼 / 엔터 눌렀을 때 */
static void on_send_clicked(GtkButton *button, gpointer user_data)
{
    if (sockfd < 0) return;

    const char *text = gtk_entry_get_text(GTK_ENTRY(entry_msg));
    if (text == NULL || *text == '\0') return;

    char buf[MAXBUF + 10];
    snprintf(buf, sizeof(buf), "/msg %s\n", text);

    if (send(sockfd, buf, strlen(buf), 0) == -1) {
        append_chat_text("** send error **");
        return;
    }

    gtk_entry_set_text(GTK_ENTRY(entry_msg), "");
}

/* 접속 버튼 */
static void on_connect_clicked(GtkButton *button, gpointer user_data)
{
    const char *ip   = gtk_entry_get_text(GTK_ENTRY(entry_ip));
    const char *nick = gtk_entry_get_text(GTK_ENTRY(entry_nick));
    const char *room = gtk_entry_get_text(GTK_ENTRY(entry_room));

    if (!ip || !*ip || !nick || !*nick || !room || !*room) {
        append_chat_text("** IP / 닉네임 / 방 이름을 모두 입력하세요 **");
        return;
    }

    if (sockfd >= 0) {
        append_chat_text("** 이미 서버에 접속 중입니다 **");
        return;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        append_chat_text("** socket() 실패 **");
        return;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        append_chat_text("** connect() 실패 **");
        close(sockfd);
        sockfd = -1;
        return;
    }

    // /join 전송
    char joinmsg[MAXBUF];
    snprintf(joinmsg, sizeof(joinmsg), "/join %s %s\n", nick, room);
    if (send(sockfd, joinmsg, strlen(joinmsg), 0) == -1) {
        append_chat_text("** /join 전송 실패 **");
        close(sockfd);
        sockfd = -1;
        return;
    }

    append_chat_text("** 서버에 접속 완료 **");

    // 소켓을 GLib GIOChannel로 감싸고 watch 등록
    sock_channel = g_io_channel_unix_new(sockfd);
    g_io_channel_set_encoding(sock_channel, NULL, NULL); // raw 바이트
    g_io_channel_set_flags(sock_channel,
                           g_io_channel_get_flags(sock_channel) | G_IO_FLAG_NONBLOCK,
                           NULL);

    g_io_add_watch(sock_channel,
                   G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
                   socket_io_cb, NULL);

    gtk_widget_set_sensitive(btn_connect, FALSE);
    gtk_widget_set_sensitive(btn_send, TRUE);
}

/* 창 닫을 때 */
static void on_window_destroy(GtkWidget *widget, gpointer data)
{
    if (sock_channel) {
        g_io_channel_shutdown(sock_channel, FALSE, NULL);
        g_io_channel_unref(sock_channel);
    }
    if (sockfd >= 0) {
        close(sockfd);
    }
    gtk_main_quit();
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");  // ← 현재 로케일 사용 (GTK는 기본적으로 UTF-8 기대)
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK Chat Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    // 전체 레이아웃: vbox
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // 상단: 서버 IP, 닉네임, 방, 접속 버튼
    GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_top, FALSE, FALSE, 5);

    entry_ip   = gtk_entry_new();
    entry_nick = gtk_entry_new();
    entry_room = gtk_entry_new();

    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_ip),   "서버 IP (예: 127.0.0.1)");
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_nick), "닉네임");
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_room), "방 이름");

    gtk_box_pack_start(GTK_BOX(hbox_top), entry_ip,   TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), entry_nick, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), entry_room, TRUE, TRUE, 0);

    btn_connect = gtk_button_new_with_label("접속");
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_connect, FALSE, FALSE, 0);
    g_signal_connect(btn_connect, "clicked",
                     G_CALLBACK(on_connect_clicked), NULL);

    // 가운데: 채팅창 (TextView + ScrolledWindow)
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 5);

    textview_chat = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview_chat), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview_chat), GTK_WRAP_WORD_CHAR);
    gtk_container_add(GTK_CONTAINER(scrolled), textview_chat);

    // 하단: 메시지 입력 + 전송 버튼
    GtkWidget *hbox_bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_bottom, FALSE, FALSE, 5);

    entry_msg = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox_bottom), entry_msg, TRUE, TRUE, 0);

    btn_send = gtk_button_new_with_label("전송");
    gtk_widget_set_sensitive(btn_send, FALSE); // 접속 전에는 disable
    gtk_box_pack_start(GTK_BOX(hbox_bottom), btn_send, FALSE, FALSE, 0);

    g_signal_connect(btn_send, "clicked",
                     G_CALLBACK(on_send_clicked), NULL);
    // 엔터 키로도 전송
    g_signal_connect(entry_msg, "activate",
                     G_CALLBACK(on_send_clicked), NULL);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
