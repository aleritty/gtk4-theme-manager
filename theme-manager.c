#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glib/gstdio.h>

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

// --- Helper: Recursively delete a directory, robust version ---
gboolean remove_directory(const char *path, GError **error);

// --- Modern CSS Styling ---
static void load_custom_css(void)
{
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css =
        ".drag-overlay-label {"
        "  font-size: 22px;"
        "  color: #222;"
        "  background: rgba(255,255,255,0.8);"
        "  border-radius: 16px;"
        "  padding: 24px 48px;"
        "  box-shadow: 0 2px 16px rgba(0,0,0,0.12);"
        "}"
        "button.suggested-action {"
        "  background: #3584e4;"
        "  color: #fff;"
        "  border-radius: 8px;"
        "  font-weight: bold;"
        "}"
        "button.delete-action {"
        "  background: #e43c3c;"
        "  color: #fff;"
        "  border-radius: 8px;"
        "  font-weight: bold;"
        "}"
        ".dim-label { color: #666; }"
        ".title-1 { font-size: 28px; font-weight: 600; margin-bottom: 8px; }";
#if GTK_CHECK_VERSION(4, 8, 0)
    gtk_css_provider_load_from_string(provider, css);
#else
    gtk_css_provider_load_from_data(provider, css, -1);
#endif
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

// --- Info Dialog Helper ---
static void show_info_dialog(GtkWindow *parent, const char *message)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Info",
        parent,
        GTK_DIALOG_MODAL,
        (const char *)"OK", GTK_RESPONSE_OK,
        NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new(message);
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_box_append(GTK_BOX(content), label);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_widget_show(dialog);
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
}

// --- Confirmation Dialog Helper ---
struct DeleteThemeCtx
{
    GtkWindow *parent;
    gchar *theme_dir;
    AppWidgets *widgets;
};

static void on_delete_theme_confirm(GtkDialog *dialog, int response, gpointer user_data)
{
    struct DeleteThemeCtx *ctx = user_data;
    if (response == GTK_RESPONSE_ACCEPT)
    {
        GError *error = NULL;
        if (remove_directory(ctx->theme_dir, &error))
        {
            // Clear the main view
            if (ctx->widgets && ctx->widgets->main_area)
            {
                GtkWidget *parent_box = gtk_widget_get_parent(ctx->widgets->main_area);
                if (parent_box)
                {
                    gtk_box_remove(GTK_BOX(parent_box), ctx->widgets->main_area);
                    GtkWidget *empty_label = gtk_label_new("Select a theme to view details");
                    gtk_box_append(GTK_BOX(parent_box), empty_label);
                    ctx->widgets->main_area = empty_label;
                }
            }
            show_info_dialog(ctx->parent, "Theme deleted successfully!");
        }
        else
        {
            if (error)
            {
                show_info_dialog(ctx->parent, error->message);
                g_error_free(error);
            }
            else
            {
                show_info_dialog(ctx->parent, "Failed to delete theme!");
            }
        }
    }
    g_free(ctx->theme_dir);
    g_free(ctx);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_delete_theme_clicked(GtkButton *button, gpointer user_data)
{
    const char *theme_name = (const char *)user_data;
    gchar *theme_dir = g_build_filename(g_get_home_dir(), ".themes", theme_name, NULL);
    GtkWindow *parent = GTK_WINDOW(gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW));
    gchar *msg = g_strdup_printf("Are you sure you want to delete the theme '%s'?", theme_name);
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Delete Theme",
        parent,
        GTK_DIALOG_MODAL,
        (const char *)"Cancel", GTK_RESPONSE_CANCEL,
        (const char *)"Delete", GTK_RESPONSE_ACCEPT,
        NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new(msg);
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_box_append(GTK_BOX(content), label);
    g_free(msg);
    struct DeleteThemeCtx *ctx = g_new0(struct DeleteThemeCtx, 1);
    ctx->parent = parent;
    ctx->theme_dir = theme_dir;
    // Find widgets pointer (from button ancestry)
    GtkWidget *ancestor = GTK_WIDGET(button);
    while (ancestor && !g_object_get_data(G_OBJECT(ancestor), "app_widgets"))
    {
        ancestor = gtk_widget_get_parent(ancestor);
    }
    ctx->widgets = ancestor ? g_object_get_data(G_OBJECT(ancestor), "app_widgets") : NULL;
    g_signal_connect(dialog, "response", G_CALLBACK(on_delete_theme_confirm), ctx);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_widget_show(dialog);
}

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
            gtk_widget_set_halign(name_label, GTK_ALIGN_CENTER);
            GtkWidget *loc_label = gtk_label_new(theme_manager_path);
            gtk_widget_set_halign(loc_label, GTK_ALIGN_CENTER);
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
            gtk_widget_set_halign(name_label, GTK_ALIGN_CENTER);
            GtkWidget *loc_label = gtk_label_new("/usr/share/themes");
            gtk_widget_set_halign(loc_label, GTK_ALIGN_CENTER);
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

// Helper: Extract archive to ~/.themes
static gboolean extract_theme_archive(const char *filepath)
{
    const char *home = g_get_home_dir();
    gchar *themes_dir = g_build_filename(home, ".themes", NULL);
    g_mkdir_with_parents(themes_dir, 0755);

    gboolean success = FALSE;
    if (g_str_has_suffix(filepath, ".zip"))
    {
        gchar *cmd = g_strdup_printf("unzip -o '%s' -d '%s'", filepath, themes_dir);
        int status = system(cmd);
        success = (status == 0);
        g_free(cmd);
    }
    else if (g_str_has_suffix(filepath, ".tar.gz") || g_str_has_suffix(filepath, ".tgz"))
    {
        gchar *cmd = g_strdup_printf("tar -xzf '%s' -C '%s'", filepath, themes_dir);
        int status = system(cmd);
        success = (status == 0);
        g_free(cmd);
    }
    else if (g_str_has_suffix(filepath, ".tar.xz"))
    {
        gchar *cmd = g_strdup_printf("tar -xJf '%s' -C '%s'", filepath, themes_dir);
        int status = system(cmd);
        success = (status == 0);
        g_free(cmd);
    }
    g_free(themes_dir);
    return success;
}

// Handler for dropped files
static GtkWidget *drag_overlay = NULL;

static void show_drag_overlay(GtkWidget *window)
{
    if (!drag_overlay)
    {
        drag_overlay = gtk_overlay_new();
        GtkWidget *label = gtk_label_new("Drop archive to install theme");
        gtk_widget_add_css_class(label, "drag-overlay-label");
        gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
        gtk_overlay_set_child(GTK_OVERLAY(drag_overlay), label);
        gtk_widget_set_hexpand(drag_overlay, TRUE);
        gtk_widget_set_vexpand(drag_overlay, TRUE);
        gtk_widget_set_opacity(drag_overlay, 0.85);
    }
    if (!gtk_widget_get_parent(drag_overlay))
    {
        gtk_overlay_add_overlay(GTK_OVERLAY(window), drag_overlay);
    }
    gtk_widget_set_visible(drag_overlay, TRUE);
}

static void hide_drag_overlay(GtkWidget *window)
{
    if (drag_overlay && gtk_widget_get_parent(drag_overlay))
    {
        gtk_widget_set_visible(drag_overlay, FALSE);
    }
}

static void on_main_window_drop(GtkDropTarget *target, const GValue *value, double x, double y, gpointer user_data)
{
    AppWidgets *widgets = (AppWidgets *)user_data;
    GtkWidget *window = gtk_widget_get_ancestor(GTK_WIDGET(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(target))), GTK_TYPE_WINDOW);
    hide_drag_overlay(window);
    if (G_VALUE_HOLDS(value, G_TYPE_FILE))
    {
        GFile *file = g_value_get_object(value);
        char *path = g_file_get_path(file);
        if (path)
        {
            if (g_str_has_suffix(path, ".zip") || g_str_has_suffix(path, ".tar.gz") || g_str_has_suffix(path, ".tgz") || g_str_has_suffix(path, ".tar.xz"))
            {
                if (extract_theme_archive(path))
                {
                    show_info_dialog(GTK_WINDOW(window), "Theme installed successfully!");
                    // Refresh sidebar
                    GtkWidget *parent = gtk_widget_get_parent(widgets->sidebar);
                    gtk_box_remove(GTK_BOX(parent), widgets->sidebar);
                    widgets->sidebar = create_sidebar(widgets);
                    gtk_box_prepend(GTK_BOX(parent), widgets->sidebar);
                }
                else
                {
                    show_info_dialog(GTK_WINDOW(window), "Failed to install theme!");
                }
            }
            g_free(path);
        }
    }
}

static gboolean on_main_window_drag_enter(GtkDropTarget *target, double x, double y, gpointer user_data)
{
    AppWidgets *widgets = (AppWidgets *)user_data;
    GtkWidget *window = gtk_widget_get_ancestor(GTK_WIDGET(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(target))), GTK_TYPE_WINDOW);
    show_drag_overlay(window);
    return TRUE;
}

static void on_main_window_drag_leave(GtkDropTarget *target, gpointer user_data)
{
    AppWidgets *widgets = (AppWidgets *)user_data;
    GtkWidget *window = gtk_widget_get_ancestor(GTK_WIDGET(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(target))), GTK_TYPE_WINDOW);
    hide_drag_overlay(window);
}

static void
on_set_theme_button_clicked(GtkButton *button, gpointer user_data)
{
    const char *theme_name = g_object_get_data(G_OBJECT(button), "theme_name");
    g_print("Setting theme: %s\n", theme_name);
    gchar *command = g_strdup_printf("gsettings set org.gnome.desktop.interface gtk-theme '%s'", theme_name);
    if (g_spawn_command_line_async(command, NULL))
    {
        g_print("Theme set successfully\n");
    }
    else
    {
        g_print("Failed to set theme\n");
    }
    g_free(command);

    // Set the shell theme using dconf
    gchar *dconf_command = g_strdup_printf("dconf write /org/gnome/shell/extensions/user-theme/name \"'%s'\"", theme_name);
    if (g_spawn_command_line_async(dconf_command, NULL))
    {
        g_print("Shell theme set successfully\n");
    }
    else
    {
        g_print("Failed to set shell theme\n");
    }
    g_free(dconf_command);
}

// --- Helper: Recursively delete a directory, robust version ---
gboolean remove_directory(const char *path, GError **error)
{
    GDir *dir = g_dir_open(path, 0, error);
    if (!dir)
    {
        if (error && *error == NULL)
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_ACCES, "Failed to open directory: %s", path);
        return FALSE;
    }
    gboolean ok = TRUE;
    const gchar *entry;
    while ((entry = g_dir_read_name(dir)) != NULL)
    {
        gchar *entry_path = g_build_filename(path, entry, NULL);
        struct stat st;
        if (lstat(entry_path, &st) == 0)
        {
            if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode))
            {
                if (!remove_directory(entry_path, error))
                {
                    ok = FALSE;
                }
            }
            else
            {
                if (g_remove(entry_path) != 0)
                {
                    ok = FALSE;
                    if (error && *error == NULL)
                        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_ACCES, "Failed to remove file: %s", entry_path);
                }
            }
        }
        else
        {
            ok = FALSE;
            if (error && *error == NULL)
                g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_ACCES, "Failed to stat: %s", entry_path);
        }
        g_free(entry_path);
    }
    g_dir_close(dir);
    if (rmdir(path) != 0)
    {
        ok = FALSE;
        if (error && *error == NULL)
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_ACCES, "Failed to remove directory: %s", path);
    }
    return ok;
}

static void
on_themes_dir_changed(GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, gpointer user_data)
{
    AppWidgets *widgets = (AppWidgets *)user_data;
    // Refresh sidebar
    if (widgets && widgets->sidebar)
    {
        GtkWidget *parent = gtk_widget_get_parent(widgets->sidebar);
        gtk_box_remove(GTK_BOX(parent), widgets->sidebar);
        widgets->sidebar = create_sidebar(widgets);
        gtk_box_prepend(GTK_BOX(parent), widgets->sidebar);
    }
}

static void
activate(GtkApplication *app, gpointer user_data)
{
    load_custom_css();

    AppWidgets *widgets = g_new0(AppWidgets, 1);

    GtkWidget *window = gtk_application_window_new(app);
    g_object_set_data(G_OBJECT(window), "app_widgets", widgets);
    gtk_window_set_title(GTK_WINDOW(window), "Theme Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    gtk_window_set_icon_name(GTK_WINDOW(window), "your-icon-name");

    // Enable drag-and-drop for archive files
    GtkDropTarget *drop_target = gtk_drop_target_new(G_TYPE_FILE, GDK_ACTION_COPY);
    g_signal_connect(drop_target, "drop", G_CALLBACK(on_main_window_drop), widgets);
    g_signal_connect(drop_target, "enter", G_CALLBACK(on_main_window_drag_enter), widgets);
    g_signal_connect(drop_target, "leave", G_CALLBACK(on_main_window_drag_leave), widgets);
    gtk_widget_add_controller(window, GTK_EVENT_CONTROLLER(drop_target));

    // Setup GFileMonitor for ~/.themes
    const char *home = g_get_home_dir();
    gchar *themes_dir = g_build_filename(home, ".themes", NULL);
    GFile *themes_gfile = g_file_new_for_path(themes_dir);
    GFileMonitor *themes_monitor = g_file_monitor_directory(themes_gfile, G_FILE_MONITOR_NONE, NULL, NULL);
    g_signal_connect(themes_monitor, "changed", G_CALLBACK(on_themes_dir_changed), widgets);
    g_object_unref(themes_gfile);
    g_free(themes_dir);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    GtkWidget *sidebar = create_sidebar(widgets);
    gtk_box_append(GTK_BOX(main_box), sidebar);

    // Create a vertical box to hold the main view
    GtkWidget *main_view_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(main_box), main_view_box);

    widgets->main_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_hexpand(widgets->main_area, TRUE);
    gtk_widget_set_vexpand(widgets->main_area, TRUE);
    gtk_box_append(GTK_BOX(main_view_box), widgets->main_area);

    // Ensure the main view starts empty
    GtkWidget *empty_label = gtk_label_new("Select a theme to view details");
    gtk_box_append(GTK_BOX(widgets->main_area), empty_label);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("net.aleritty.ThemeManager", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
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

    // Move comment below the title
    if (error == NULL)
    {
        gchar *comment = g_key_file_get_string(key_file, "Desktop Entry", "Comment", NULL);
        if (comment && *comment)
        {
            GtkWidget *comment_label = gtk_label_new(comment);
            gtk_label_set_xalign(GTK_LABEL(comment_label), 0.0f);
            gtk_widget_add_css_class(comment_label, "dim-label");
            gtk_box_append(GTK_BOX(box), comment_label);
        }
        g_free(comment);
    }

    GtkWidget *preview = create_theme_preview_widget();
    gtk_box_append(GTK_BOX(box), preview);

    // --- BUTTON BAR: Set Theme + Delete Theme ---
    GtkWidget *button_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(button_bar, 12);
    gtk_widget_set_halign(button_bar, GTK_ALIGN_CENTER);

    GtkWidget *set_theme_button = gtk_button_new_with_label("Set Theme");
    gtk_widget_add_css_class(set_theme_button, "suggested-action");
    g_object_set_data(G_OBJECT(set_theme_button), "theme_name", g_strdup(theme_name));
    g_signal_connect(set_theme_button, "clicked", G_CALLBACK(on_set_theme_button_clicked), NULL);

    GtkWidget *delete_theme_button = gtk_button_new_with_label("Delete Theme");
    gtk_widget_add_css_class(delete_theme_button, "delete-action");
    g_object_set_data(G_OBJECT(delete_theme_button), "theme_name", g_strdup(theme_name));
    g_signal_connect(delete_theme_button, "clicked", G_CALLBACK(on_delete_theme_clicked), (gpointer)theme_name);

    gtk_box_append(GTK_BOX(button_bar), set_theme_button);
    gtk_box_append(GTK_BOX(button_bar), delete_theme_button);
    gtk_box_append(GTK_BOX(box), button_bar);

    if (error == NULL)
    {
        gchar **keys = g_key_file_get_keys(key_file, "Desktop Entry", NULL, NULL);
        if (keys)
        {
            for (int i = 0; keys[i] != NULL; i++)
            {
                if (g_strcmp0(keys[i], "Type") == 0 || g_strcmp0(keys[i], "Encoding") == 0 || g_strcmp0(keys[i], "Name") == 0 || g_strcmp0(keys[i], "Comment") == 0)
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

        gchar **meta_keys = g_key_file_get_keys(key_file, "X-GNOME-Metatheme", NULL, NULL);
        if (meta_keys)
        {
            GtkWidget *suggested_frame = gtk_frame_new("Suggested options");
            GtkWidget *suggested_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
            gtk_widget_set_margin_top(suggested_box, 8);
            gtk_widget_set_margin_bottom(suggested_box, 8);
            gtk_widget_set_margin_start(suggested_box, 8);
            gtk_widget_set_margin_end(suggested_box, 8);
            for (int i = 0; meta_keys[i] != NULL; i++)
            {
                gchar *meta_value = g_key_file_get_string(key_file, "X-GNOME-Metatheme", meta_keys[i], NULL);
                if (meta_value)
                {
                    gchar *meta_line = g_strdup_printf("%s: %s", meta_keys[i], meta_value);
                    GtkWidget *meta_label = gtk_label_new(meta_line);
                    gtk_label_set_xalign(GTK_LABEL(meta_label), 0.0f);
                    gtk_box_append(GTK_BOX(suggested_box), meta_label);
                    g_free(meta_line);
                    g_free(meta_value);
                }
            }
            g_strfreev(meta_keys);
            gtk_frame_set_child(GTK_FRAME(suggested_frame), suggested_box);
            gtk_box_append(GTK_BOX(box), suggested_frame);
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
