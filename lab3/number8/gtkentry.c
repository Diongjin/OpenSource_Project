#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
const char *password = "hidden";
void quit(GtkWidget *window, gpointer data)
{
    gtk_main_quit();
}

void input_completed(GtkWidget *entry, gpointer data)
{
    const char *text = gtk_entry_get_text(GTK_ENTRY((GtkWidget *)entry));
    if (strcmp(text, password) == 0)
        gtk_label_set_text(GTK_LABEL((GtkWidget *)data), "로긴이 허용됩니다!\n");
    else
        gtk_label_set_text(GTK_LABEL((GtkWidget *)data), "입력 오류입니다!\n");
}

int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *user_label, *passwd_label;
    GtkWidget *user_entry, *passwd_entry;
    GtkWidget *message_label;
    GtkWidget *user_hbox, *passwd_hbox;
    GtkWidget *vbox;
    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "사용자 입력창");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 150);
    g_signal_connect(window, "destroy", G_CALLBACK(quit), NULL);
    user_label = gtk_label_new("사용자:");
    passwd_label = gtk_label_new("패스워드:");
    user_entry = gtk_entry_new();
    passwd_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(passwd_entry), FALSE);
    message_label = gtk_label_new("이름과 패스워드를 입력하세요.");
    user_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    passwd_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(user_hbox), user_label, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(user_hbox), user_entry, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(passwd_hbox), passwd_label, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(passwd_hbox), passwd_entry, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), user_hbox, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), passwd_hbox, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), message_label, FALSE, FALSE, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
