#include <gtk/gtk.h>
#include <gio/gio.h>

typedef struct
{
    GtkStack *stack;
    GtkWidget *main_area;
    GtkWidget *sidebar;
} AppWidgets;

typedef struct
{
    char *theme_name;
    char *location;
} ThemeRow;

GtkWidget *create_theme_metadata_page(const char *theme_name);
GtkWidget *create_theme_preview_widget();

static void
on_sidebar_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
    AppWidgets *widgets = user_data;
    if (!row || !widgets->main_area)
        return;
    ThemeRow *data = g_object_get_data(G_OBJECT(row), "theme_row_data");
    if (!data)
        return;

    GtkWidget *new_view = create_theme_metadata_page(data->theme_name);
    GtkWidget *parent = gtk_widget_get_parent(widgets->main_area);
    gtk_box_remove(GTK_BOX(parent), widgets->main_area);
    widgets->main_area = new_view;
    gtk_box_append(GTK_BOX(parent), widgets->main_area);
}

static GtkWidget *
create_sidebar(AppWidgets *widgets)
{
    GtkWidget *listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox), GTK_SELECTION_BROWSE);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css, ".row.no-hover:hover { background-color: transparent; }");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    GtkWidget *user_label = gtk_label_new("User Themes");
    gtk_widget_set_margin_top(user_label, 8);
    gtk_widget_set_margin_bottom(user_label, 2);
    gtk_widget_set_halign(user_label, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(user_label, "heading");
    // Use GtkBox to hold label so it's not treated as a selectable row
    GtkWidget *user_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_append(GTK_BOX(user_box), user_label);
    GtkWidget *user_row = gtk_list_box_row_new();
    gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(user_row), FALSE);
    gtk_widget_add_css_class(user_row, "no-hover");
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(user_row), user_box);
    gtk_list_box_append(GTK_LIST_BOX(listbox), user_row);

    gchar *theme_manager_path = g_build_filename(g_get_home_dir(), ".themes", NULL);
    GDir *dir = g_dir_open(theme_manager_path, 0, NULL);
    GList *user_theme_list = NULL;
    const gchar *filename;
    if (dir)
    {
        while ((filename = g_dir_read_name(dir)) != NULL)
        {
            gchar *theme_dir = g_build_filename(theme_manager_path, filename, NULL);
            gchar *index_file = g_build_filename(theme_dir, "index.theme", NULL);
            if (g_file_test(theme_dir, G_FILE_TEST_IS_DIR) && g_file_test(index_file, G_FILE_TEST_EXISTS))
            {
                user_theme_list = g_list_prepend(user_theme_list, g_strdup(filename));
            }
            g_free(theme_dir);
            g_free(index_file);
        }
        user_theme_list = g_list_sort(user_theme_list, (GCompareFunc)g_ascii_strcasecmp);
        for (GList *l = user_theme_list; l != NULL; l = l->next)
        {
            GtkWidget *row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            GtkWidget *name_label = gtk_label_new((gchar *)l->data);
            gtk_widget_set_halign(name_label, GTK_ALIGN_START);
            GtkWidget *loc_label = gtk_label_new(theme_manager_path);
            gtk_widget_set_halign(loc_label, GTK_ALIGN_START);
            gtk_widget_add_css_class(loc_label, "dim-label");
            gtk_box_append(GTK_BOX(row), name_label);
            gtk_box_append(GTK_BOX(row), loc_label);
            GtkWidget *list_row = gtk_list_box_row_new();
            gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(list_row), row);
            ThemeRow *data = g_new0(ThemeRow, 1);
            data->theme_name = g_strdup((gchar *)l->data);
            data->location = g_strdup(theme_manager_path);
            g_object_set_data_full(G_OBJECT(list_row), "theme_row_data", data, (GDestroyNotify)g_free);
            // Highlight the currently active theme
            if (g_strcmp0(data->theme_name, "Currently Active Theme Name") == 0)
            {
                gtk_widget_add_css_class(list_row, "active-theme");
            }
            gtk_list_box_append(GTK_LIST_BOX(listbox), list_row);
            g_free(l->data);
        }
        g_list_free(user_theme_list);
    }
    g_free(theme_manager_path);
    g_dir_close(dir);

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_list_box_append(GTK_LIST_BOX(listbox), sep);
    GtkWidget *sep_row = gtk_widget_get_parent(sep);
    gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(sep_row), FALSE);

    GtkWidget *sys_label = gtk_label_new("System Themes");
    gtk_widget_set_margin_top(sys_label, 8);
    gtk_widget_set_margin_bottom(sys_label, 2);
    gtk_widget_set_halign(sys_label, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(sys_label, "heading");
    // Use GtkBox to hold label so it's not treated as a selectable row
    GtkWidget *sys_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_append(GTK_BOX(sys_box), sys_label);
    GtkWidget *sys_row = gtk_list_box_row_new();
    gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(sys_row), FALSE);
    gtk_widget_add_css_class(sys_row, "no-hover");
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(sys_row), sys_box);
    gtk_list_box_append(GTK_LIST_BOX(listbox), sys_row);

    dir = g_dir_open("/usr/share/themes", 0, NULL);
    GList *system_theme_list = NULL;
    if (dir)
    {
        while ((filename = g_dir_read_name(dir)) != NULL)
        {
            gchar *theme_dir = g_build_filename("/usr/share/themes", filename, NULL);
            gchar *index_file = g_build_filename(theme_dir, "index.theme", NULL);
            if (g_file_test(theme_dir, G_FILE_TEST_IS_DIR) && g_file_test(index_file, G_FILE_TEST_EXISTS))
            {
                system_theme_list = g_list_prepend(system_theme_list, g_strdup(filename));
            }
            g_free(theme_dir);
            g_free(index_file);
        }
        system_theme_list = g_list_sort(system_theme_list, (GCompareFunc)g_ascii_strcasecmp);
        for (GList *l = system_theme_list; l != NULL; l = l->next)
        {
            GtkWidget *row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            GtkWidget *name_label = gtk_label_new((gchar *)l->data);
            gtk_widget_set_halign(name_label, GTK_ALIGN_START);
            GtkWidget *loc_label = gtk_label_new("/usr/share/themes");
            gtk_widget_set_halign(loc_label, GTK_ALIGN_START);
            gtk_widget_add_css_class(loc_label, "dim-label");
            gtk_box_append(GTK_BOX(row), name_label);
            gtk_box_append(GTK_BOX(row), loc_label);
            GtkWidget *list_row = gtk_list_box_row_new();
            gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(list_row), row);
            ThemeRow *data = g_new0(ThemeRow, 1);
            data->theme_name = g_strdup((gchar *)l->data);
            data->location = g_strdup("/usr/share/themes");
            g_object_set_data_full(G_OBJECT(list_row), "theme_row_data", data, (GDestroyNotify)g_free);
            // Highlight the currently active theme
            if (g_strcmp0(data->theme_name, "Currently Active Theme Name") == 0)
            {
                gtk_widget_add_css_class(list_row, "active-theme");
            }
            gtk_list_box_append(GTK_LIST_BOX(listbox), list_row);
            g_free(l->data);
        }
        g_list_free(system_theme_list);
    }
    g_dir_close(dir);

    g_signal_connect(listbox, "row-selected", G_CALLBACK(on_sidebar_row_selected), widgets);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scrolled, 200, -1);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), listbox);
    g_object_set_data(G_OBJECT(scrolled), "listbox", listbox);
    widgets->sidebar = scrolled;
    return scrolled;
}

GtkWidget *
create_theme_metadata_page(const char *theme_name)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_start(box, 12);

    gchar *theme_dir = g_build_filename(g_get_home_dir(), ".themes", theme_name, NULL);
    gchar *index_file = g_build_filename(theme_dir, "index.theme", NULL);

    GKeyFile *key_file = g_key_file_new();
    GError *error = NULL;
    gchar *theme_display_name = NULL;
    if (g_key_file_load_from_file(key_file, index_file, G_KEY_FILE_NONE, &error))
        theme_display_name = g_key_file_get_string(key_file, "Desktop Entry", "Name", NULL);
    if (!theme_display_name)
        theme_display_name = g_strdup(theme_name);

    GtkWidget *name_label = gtk_label_new(theme_display_name);
    gtk_widget_set_halign(name_label, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(name_label, "title-1");
    gtk_widget_set_margin_bottom(name_label, 8);
    gtk_box_append(GTK_BOX(box), name_label);
    g_free(theme_display_name);

    GtkWidget *preview = create_theme_preview_widget();
    gtk_box_append(GTK_BOX(box), preview);

    if (error == NULL)
    {
        gchar **keys = g_key_file_get_keys(key_file, "Desktop Entry", NULL, NULL);
        if (keys)
        {
            for (int i = 0; keys[i] != NULL; i++)
            {
                if (g_strcmp0(keys[i], "Type") == 0 || g_strcmp0(keys[i], "Encoding") == 0 || g_strcmp0(keys[i], "Name") == 0)
                    continue;
                gchar *value = g_key_file_get_string(key_file, "Desktop Entry", keys[i], NULL);
                if (value)
                {
                    gchar *line = g_strdup_printf("%s: %s", keys[i], value);
                    GtkWidget *label = gtk_label_new(line);
                    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
                    gtk_box_append(GTK_BOX(box), label);
                    g_free(line);
                    g_free(value);
                }
            }
            g_strfreev(keys);
        }
    }
    g_clear_error(&error);
    g_key_file_unref(key_file);
    g_free(theme_dir);
    g_free(index_file);

    return box;
}

GtkWidget *
create_theme_preview_widget(void)
{
    GtkWidget *desktop_bg = gtk_frame_new(NULL);
    gtk_widget_set_size_request(desktop_bg, 400, 240);
    gtk_widget_add_css_class(desktop_bg, "frame");

    GtkWidget *overlay = gtk_overlay_new();
    gtk_frame_set_child(GTK_FRAME(desktop_bg), overlay);

    GtkWidget *window_frame = gtk_frame_new(NULL);
    gtk_widget_set_size_request(window_frame, 260, 140);
    gtk_widget_set_margin_top(window_frame, 30);
    gtk_widget_set_margin_start(window_frame, 60);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), window_frame);

    GtkWidget *window_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_frame_set_child(GTK_FRAME(window_frame), window_box);

    GtkWidget *header = gtk_header_bar_new();
    gtk_box_append(GTK_BOX(window_box), header);

    GtkWidget *content = gtk_label_new("Window Content");
    gtk_label_set_xalign(GTK_LABEL(content), 0.0f);
    gtk_box_append(GTK_BOX(window_box), content);

    return desktop_bg;
}

static void
on_set_theme_button_clicked(GtkButton *button, gpointer user_data)
{
    AppWidgets *widgets = (AppWidgets *)user_data;
    GtkListBox *listbox = GTK_LIST_BOX(g_object_get_data(G_OBJECT(widgets->sidebar), "listbox"));
    GtkListBoxRow *selected_row = gtk_list_box_get_selected_row(listbox);
    if (selected_row)
    {
        ThemeRow *data = g_object_get_data(G_OBJECT(selected_row), "theme_row_data");
        if (data)
        {
            g_print("Setting theme: %s\n", data->theme_name);
            gchar *command = g_strdup_printf("gsettings set org.gnome.desktop.interface gtk-theme '%s'", data->theme_name);
            if (g_spawn_command_line_async(command, NULL)) {
                g_print("Theme set successfully\n");
            } else {
                g_print("Failed to set theme\n");
            }
            g_free(command);

            // Set the shell theme using dconf
            gchar *dconf_command = g_strdup_printf("dconf write /org/gnome/shell/extensions/user-theme/name \"'%s'\"", data->theme_name);
            if (g_spawn_command_line_async(dconf_command, NULL)) {
                g_print("Shell theme set successfully\n");
            } else {
                g_print("Failed to set shell theme\n");
            }
            g_free(dconf_command);
        }
    }
}

static void
activate(GtkApplication *app, gpointer user_data)
{
    AppWidgets *widgets = g_new0(AppWidgets, 1);

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Theme Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    gtk_window_set_icon_name(GTK_WINDOW(window), "your-icon-name");

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    GtkWidget *sidebar = create_sidebar(widgets);
    gtk_box_append(GTK_BOX(main_box), sidebar);

    // Create a vertical box to hold the main view and the button
    GtkWidget *main_view_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(main_box), main_view_box);

    widgets->main_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(widgets->main_area, TRUE);
    gtk_widget_set_vexpand(widgets->main_area, TRUE);
    gtk_box_append(GTK_BOX(main_view_box), widgets->main_area);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(main_view_box), button_box);
    GtkWidget *set_theme_button = gtk_button_new_with_label("Set Theme");
    gtk_box_append(GTK_BOX(button_box), set_theme_button);
    g_signal_connect(set_theme_button, "clicked", G_CALLBACK(on_set_theme_button_clicked), widgets);

    // Ensure the main view starts empty
    GtkWidget *empty_label = gtk_label_new("Select a theme to view details");
    gtk_box_append(GTK_BOX(widgets->main_area), empty_label);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("com.example.ThemeManager", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
