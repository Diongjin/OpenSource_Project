#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <locale.h>
#include <sys/stat.h>

#define PORT   3490
#define MAXBUF 4096

static GtkWidget *entry_ip;
static GtkWidget *entry_nick;
static GtkWidget *entry_room;
static GtkWidget *textview_chat;
static GtkWidget *entry_msg;
static GtkWidget *btn_connect;
static GtkWidget *btn_send;
static GtkWidget *btn_file;

static int sockfd = -1;
static GIOChannel *sock_channel = NULL;

/* 파일 수신 상태 */
static gboolean receiving_file = FALSE;
static long recv_file_remaining = 0;
static FILE *recv_fp = NULL;
static char recv_filename[256] = {0};

/* 채팅창에 문자열 추가 */
static void append_chat_text(const char *msg)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview_chat));
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    gtk_text_buffer_insert(buffer, &end_iter, msg, -1);
    gtk_text_buffer_insert(buffer, &end_iter, "\n", -1);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(textview_chat),
                                 &end_iter, 0.0, FALSE, 0, 0);
}

/* 소켓에서 데이터 들어올 때 콜백 */
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
        gtk_widget_set_sensitive(btn_file, FALSE);
        return FALSE;
    }

    char buf[MAXBUF];
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
        gtk_widget_set_sensitive(btn_file, FALSE);
        return FALSE;
    }

    /* 파일 데이터 수신 중이면 그대로 파일에 기록 */
    if (receiving_file && recv_fp != NULL) {
        long to_write = (recv_file_remaining < n) ? recv_file_remaining : n;
        fwrite(buf, 1, to_write, recv_fp);
        recv_file_remaining -= to_write;

        if (recv_file_remaining <= 0) {
            fclose(recv_fp);
            recv_fp = NULL;
            receiving_file = FALSE;

            char msg[512];
            snprintf(msg, sizeof(msg),
                     "** 파일 수신 완료: %s **", recv_filename);
            append_chat_text(msg);
        }
        return TRUE;
    }

    /* 평상시: 텍스트 처리 (FILE 헤더 포함) */
    buf[n] = '\0';
    char *p = buf;
    while (*p) {
        char *nl = strchr(p, '\n');
        if (nl) *nl = '\0';

        if (strncmp(p, "FILE ", 5) == 0) {
            // 헤더 형식: FILE <sender> <filename> <size>
            char sender[64], fname[256];
            long size = 0;
            if (sscanf(p, "FILE %63s %255s %ld", sender, fname, &size) == 3
                && size > 0) {
                char msg[512];
                snprintf(msg, sizeof(msg),
                         "** 파일 수신 시작: %s (%ld bytes) from %s **",
                         fname, size, sender);
                append_chat_text(msg);

                snprintf(recv_filename, sizeof(recv_filename),
                         "recv_%s", fname);
                recv_fp = fopen(recv_filename, "wb");
                if (!recv_fp) {
                    append_chat_text("** 파일 저장 실패 **");
                    receiving_file = FALSE;
                    recv_file_remaining = 0;
                } else {
                    receiving_file = TRUE;
                    recv_file_remaining = size;
                }
            } else {
                append_chat_text(p);
            }
        } else {
            append_chat_text(p);
        }

        if (!nl) break;
        p = nl + 1;
    }

    return TRUE;
}

/* 채팅 전송 버튼 / 엔터 */
static void on_send_clicked(GtkButton *button, gpointer user_data)
{
    if (sockfd < 0) return;

    const char *text = gtk_entry_get_text(GTK_ENTRY(entry_msg));
    if (text == NULL || *text == '\0') return;

    char buf[MAXBUF];
    snprintf(buf, sizeof(buf), "/msg %s\n", text);

    if (send(sockfd, buf, strlen(buf), 0) == -1) {
        append_chat_text("** send error **");
        return;
    }

    gtk_entry_set_text(GTK_ENTRY(entry_msg), "");
}

/* 파일 전송 버튼 */
static void on_file_clicked(GtkButton *button, gpointer user_data)
{
    if (sockfd < 0) return;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "파일 선택",
        GTK_WINDOW(user_data),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_취소", GTK_RESPONSE_CANCEL,
        "_열기", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filepath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filepath) {
            FILE *fp = fopen(filepath, "rb");
            if (!fp) {
                append_chat_text("** 파일을 열 수 없습니다 **");
                g_free(filepath);
                gtk_widget_destroy(dialog);
                return;
            }

            // 파일 크기 구하기
            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            // 파일 이름만 추출
            char *basename = g_path_get_basename(filepath);

            // /file 헤더 전송
            char header[MAXBUF];
            snprintf(header, sizeof(header),
                     "/file %s %ld\n", basename, size);
            if (send(sockfd, header, strlen(header), 0) == -1) {
                append_chat_text("** /file 헤더 전송 실패 **");
                fclose(fp);
                g_free(filepath);
                g_free(basename);
                gtk_widget_destroy(dialog);
                return;
            }

            // 실제 파일 데이터 전송
            char buf[MAXBUF];
            size_t nread;
            while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0) {
                if (send(sockfd, buf, nread, 0) == -1) {
                    append_chat_text("** 파일 데이터 전송 실패 **");
                    break;
                }
            }

            fclose(fp);

            char msg[512];
            snprintf(msg, sizeof(msg),
                     "** 파일 전송 완료: %s (%ld bytes) **", basename, size);
            append_chat_text(msg);

            g_free(basename);
            g_free(filepath);
        }
    }

    gtk_widget_destroy(dialog);
}

/* 접속 버튼 */
static void on_connect_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *window = GTK_WIDGET(user_data);

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

    sock_channel = g_io_channel_unix_new(sockfd);
    g_io_channel_set_encoding(sock_channel, NULL, NULL);
    g_io_channel_set_flags(sock_channel,
                           g_io_channel_get_flags(sock_channel) | G_IO_FLAG_NONBLOCK,
                           NULL);

    g_io_add_watch(sock_channel,
                   G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
                   socket_io_cb, window);

    gtk_widget_set_sensitive(btn_connect, FALSE);
    gtk_widget_set_sensitive(btn_send, TRUE);
    gtk_widget_set_sensitive(btn_file, TRUE);
}

/* 창 닫기 */
static void on_window_destroy(GtkWidget *widget, gpointer data)
{
    if (sock_channel) {
        g_io_channel_shutdown(sock_channel, FALSE, NULL);
        g_io_channel_unref(sock_channel);
    }
    if (sockfd >= 0)
        close(sockfd);

    if (recv_fp) fclose(recv_fp);
    gtk_main_quit();
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK Chat Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 450, 320);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // 상단: IP, 닉네임, 방, 접속
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
                     G_CALLBACK(on_connect_clicked), window);

    // 가운데: 채팅창
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 5);

    textview_chat = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview_chat), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview_chat), GTK_WRAP_WORD_CHAR);
    gtk_container_add(GTK_CONTAINER(scrolled), textview_chat);

    // 하단: 입력창 + 전송 + 파일
    GtkWidget *hbox_bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_bottom, FALSE, FALSE, 5);

    entry_msg = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox_bottom), entry_msg, TRUE, TRUE, 0);

    btn_send = gtk_button_new_with_label("전송");
    gtk_widget_set_sensitive(btn_send, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox_bottom), btn_send, FALSE, FALSE, 0);

    btn_file = gtk_button_new_with_label("파일전송");
    gtk_widget_set_sensitive(btn_file, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox_bottom), btn_file, FALSE, FALSE, 0);

    g_signal_connect(btn_send, "clicked",
                     G_CALLBACK(on_send_clicked), NULL);
    g_signal_connect(entry_msg, "activate",
                     G_CALLBACK(on_send_clicked), NULL);

    g_signal_connect(btn_file, "clicked",
                     G_CALLBACK(on_file_clicked), window);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}