#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>   // strchr, strcmp 등

static GtkWidget *entry;      // 표시창
static double stored_value = 0.0;   // (지금은 안 써도 됨)
static char current_op = 0;   // '+', '-', '*', '/', 0
static gboolean new_input = TRUE; // 새로 입력 시작?

// 숫자 버튼 클릭
static void
on_digit_clicked(GtkButton *button, gpointer user_data)
{
    const char *digit = gtk_button_get_label(button);
    const char *cur = gtk_entry_get_text(GTK_ENTRY(entry));
    char buf[256];

    if (new_input || (cur[0] == '0' && cur[1] == '\0')) {
        snprintf(buf, sizeof(buf), "%s", digit);
        new_input = FALSE;
    } else {
        snprintf(buf, sizeof(buf), "%s%s", cur, digit);
    }
    gtk_entry_set_text(GTK_ENTRY(entry), buf);
}

// 연산자 버튼 클릭 (+ - * /)
static void
on_op_clicked(GtkButton *button, gpointer user_data)
{
    const char *cur = gtk_entry_get_text(GTK_ENTRY(entry));
    const char *op_label = gtk_button_get_label(button);
    char buf[256];

    // 이미 연산자가 들어가 있으면 더 이상 추가하지 않음 (간단 버전)
    if (strchr(cur, '+') || strchr(cur, '-') ||
        strchr(cur, '*') || strchr(cur, '/')) {
        return;
    }

    current_op = op_label[0];

    // "12" -> "12+" 이런 식으로 표시 (공백 없음)
    snprintf(buf, sizeof(buf), "%s%c", cur, current_op);
    gtk_entry_set_text(GTK_ENTRY(entry), buf);

    // 뒤에 오는 숫자는 이어서 입력
    new_input = FALSE;
}

// = 버튼 클릭
static void
on_equal_clicked(GtkButton *button, gpointer user_data)
{
    const char *cur = gtk_entry_get_text(GTK_ENTRY(entry));
    char buf[256];

    // 식에서 연산자 위치 찾기 (첫 번째 + - * /)
    int op_pos = -1;
    char op = 0;
    for (int i = 0; cur[i] != '\0'; i++) {
        if (cur[i] == '+' || cur[i] == '-' ||
            cur[i] == '*' || cur[i] == '/') {
            op = cur[i];
            op_pos = i;
            break;
        }
    }

    // 연산자가 없거나, 양쪽 피연산자가 비정상이면 무시
    if (op_pos <= 0 || cur[op_pos + 1] == '\0') {
        return;
    }

    char left[128], right[128];

    // 왼쪽 피연산자
    strncpy(left, cur, op_pos);
    left[op_pos] = '\0';

    // 오른쪽 피연산자
    strncpy(right, cur + op_pos + 1, sizeof(right) - 1);
    right[sizeof(right) - 1] = '\0';

    double a = atof(left);
    double b = atof(right);
    double result = 0.0;

    switch (op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/':
            if (b == 0.0) {
                gtk_entry_set_text(GTK_ENTRY(entry), "Error");
                new_input = TRUE;
                current_op = 0;
                return;
            }
            result = a / b;
            break;
        default:
            return;
    }

    // 결과를 6자리 소수로 출력 → 8.000000, 32.000000 처럼
    snprintf(buf, sizeof(buf), "%.6f", result);
    gtk_entry_set_text(GTK_ENTRY(entry), buf);

    new_input = TRUE;
    current_op = 0;
}

// C 버튼 클릭 (초기화)
static void
on_clear_clicked(GtkButton *button, gpointer user_data)
{
    stored_value = 0.0;
    current_op = 0;
    new_input = TRUE;
    gtk_entry_set_text(GTK_ENTRY(entry), "0");
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK Calculator");
    gtk_window_set_default_size(GTK_WINDOW(window), 220, 200);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // 세로 박스 (위: entry, 아래: 버튼 그리드)
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // 표시창
    entry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_entry_set_alignment(GTK_ENTRY(entry), 1.0); // 오른쪽 정렬
    gtk_entry_set_text(GTK_ENTRY(entry), "0");
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 5);

    // 버튼 배치용 grid
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 5);

    // 숫자/연산자 버튼 생성
    const char *btn_labels[4][4] = {
        {"7", "8", "9", "+"},
        {"4", "5", "6", "-"},
        {"1", "2", "3", "*"},
        {"C", "0", "=", "/"}
    };

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            GtkWidget *btn = gtk_button_new_with_label(btn_labels[r][c]);
            gtk_widget_set_hexpand(btn, TRUE);
            gtk_widget_set_vexpand(btn, TRUE);
            gtk_grid_attach(GTK_GRID(grid), btn, c, r, 1, 1);

            const char *label = btn_labels[r][c];

            if (g_ascii_isdigit(label[0])) {
                g_signal_connect(btn, "clicked",
                                 G_CALLBACK(on_digit_clicked), NULL);
            } else if (strcmp(label, "C") == 0) {
                g_signal_connect(btn, "clicked",
                                 G_CALLBACK(on_clear_clicked), NULL);
            } else if (strcmp(label, "=") == 0) {
                g_signal_connect(btn, "clicked",
                                 G_CALLBACK(on_equal_clicked), NULL);
            } else { // + - * /
                g_signal_connect(btn, "clicked",
                                 G_CALLBACK(on_op_clicked), NULL);
            }
        }
    }

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
