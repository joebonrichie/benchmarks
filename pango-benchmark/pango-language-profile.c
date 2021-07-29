/* pango-language-profile - a benchmark for most of the languages supported by Pango
 *
 * Copyright (C) 2005 Federico Mena-Quintero
 * federico@novell.com
 *
 * Compile like this:
 *
 *   gcc -o pango-language-profile pango-language-profile.c `pkg-config --cflags --libs gtk+-3.0`
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/times.h>
#include <gtk/gtk.h>

#define ALL_LANGUAGES "ALL"
#define DEFAULT_DATA_DIR "po-data"
#define DEFAULT_BENCHMARK_NAME "Pango benchmark"
#define DEFAULT_NUM_ITERATIONS 20

static void
dummy_dialog (void)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Hello, world!");
	gtk_widget_show_now (dialog);
	gtk_widget_destroy (dialog);
}

static void
error_and_exit (const char *msg, ...)
{
	va_list args;

	va_start (args, msg);
	vfprintf (stderr, msg, args);
	va_end (args);
	fputs ("\n", stderr);
	exit (1);
}

typedef struct {
	clock_t start_utime;
} UserTimer;

UserTimer *
user_timer_new (void)
{
	UserTimer *utimer;
	struct tms tms;

	utimer = g_new0 (UserTimer, 1);
	times (&tms);
	utimer->start_utime = tms.tms_utime;

	return utimer;
}

double
user_timer_elapsed (UserTimer *utimer)
{
	static long clktck;
	struct tms tms;

	if (clktck == 0)
		clktck = sysconf (_SC_CLK_TCK);

	times (&tms);
	return (double) (tms.tms_utime - utimer->start_utime) / clktck;
}

void
user_timer_destroy (UserTimer *utimer)
{
	g_free (utimer);
}

typedef struct {
	char *str;
	int num_chars;
	int num_bytes;
	gboolean valid;
} String;

typedef struct {
	gsize num_strings;
	char *strings_raw;
	String *strings;
} StringSet;

typedef struct {
	double elapsed;
	long total_strings;
	long total_chars;
} LanguageResults;

static StringSet *
string_set_read (const char *filename)
{
	GError *error;
	gsize length;
	char *end;
	char *p, *string_start;
	gsize max_strings;
	char *strings_raw;
	gsize num_strings;
	String *strings;
	StringSet *set;

	error = NULL;
	if (!g_file_get_contents (filename, &strings_raw, &length, &error))
		error_and_exit ("Could not read the strings file %s: %s", filename, error->message);

	max_strings = 1024;
	num_strings = 0;
	strings = g_new (String, max_strings);

	string_start = strings_raw;
	end = strings_raw + length;

	for (p = strings_raw; p < end; p++)
		if (*p == 0) {
			if (num_strings == max_strings) {
				max_strings = max_strings * 2;
				strings = g_renew (String, strings, max_strings);
			}

			strings[num_strings].str = string_start;
			strings[num_strings].num_chars = g_utf8_strlen (string_start, -1);
			strings[num_strings].num_bytes = strlen (string_start);
			strings[num_strings].valid = (strstr (string_start, "POT-Creation") == 0
						      && g_utf8_validate (string_start, -1, NULL));
			string_start = p + 1;
			num_strings++;
		}

	set = g_new (StringSet, 1);
	set->num_strings = num_strings;
	set->strings_raw = strings_raw;
	set->strings = strings;

	return set;
}

static void
string_set_free (StringSet *set)
{
	g_free (set->strings);
	g_free (set->strings_raw);
	g_free (set);
}

static GtkWidget *window;

static void
measure_strings (StringSet *set, LanguageResults *results, int num_iters)
{
	PangoLayout *layout;
	int i, j;
	UserTimer *utimer;

	utimer = user_timer_new ();

	results->elapsed = 0.0;
	results->total_strings = 0;
	results->total_chars = 0;

	for (i = 0; i < num_iters; i++)
		for (j = 0; j < set->num_strings; j++) {
			PangoRectangle ink_rect, logical_rect;

			if (set->strings[j].valid) {
				layout = gtk_widget_create_pango_layout (window, set->strings[j].str);
				pango_layout_get_extents (layout, &ink_rect, &logical_rect);
				g_object_unref (layout);

				results->total_strings++;
				results->total_chars += set->strings[j].num_chars;
			}
		}

	results->elapsed = user_timer_elapsed (utimer);
	user_timer_destroy (utimer);
}

static char **option_langs;
static char *option_data_dir = DEFAULT_DATA_DIR;
static char *option_name = DEFAULT_BENCHMARK_NAME;
static char *option_output;
static int option_num_iterations = DEFAULT_NUM_ITERATIONS;

static FILE *output_file;

static GOptionEntry option_entries[] = {
	{ "lang", 'l', 0, G_OPTION_ARG_STRING_ARRAY, &option_langs,
	  "Specify language name (e.g. \"es\" for Spanish), or \"" ALL_LANGUAGES "\"", "string" },
	{ "data-dir", 'd', 0, G_OPTION_ARG_FILENAME, &option_data_dir,
	  "Directory where .dat files live", "dirname" },
	{ "name", 'n', 0, G_OPTION_ARG_STRING, &option_name,
	  "Name for benchmark", "string" },
	{ "output", 'o', 0, G_OPTION_ARG_FILENAME, &option_output,
	  "Output filename.  If not specified, standard output will be used.", "filename" },
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};

static void
run_one_language (const char *lang_name, const char *filename)
{
	StringSet *set;
	LanguageResults results;

	fprintf (stderr, "Processing %s\n", filename);

	set = string_set_read (filename);
	measure_strings (set, &results, option_num_iterations);
	string_set_free (set);

	fprintf (output_file, "  <language>\n");
	fprintf (output_file, "    <name>%s</name>\n", lang_name);
	fprintf (output_file, "    <elapsed>%f</elapsed>\n", results.elapsed);
	fprintf (output_file, "    <total_strings>%ld</total_strings>\n", results.total_strings);
	fprintf (output_file, "    <total_chars>%ld</total_chars>\n", results.total_chars);
	fprintf (output_file, "  </language>\n");
}

static void
run_all_languages (void)
{
	GDir *dir;
	GError *error;
	const char *entry;

	error = NULL;
	dir = g_dir_open (option_data_dir, 0, &error);
	if (!dir)
		error_and_exit ("Could not open directory: %s", error->message);

	while ((entry = g_dir_read_name (dir)) != NULL) {
		if (g_str_has_suffix (entry, ".dat")) {
			char *lang_name;
			char *filename;

			lang_name = g_strndup (entry, strlen (entry) - 4);
			filename = g_build_filename (option_data_dir, entry, NULL);

			run_one_language (lang_name, filename);

			g_free (lang_name);
			g_free (filename);
		}
	}

	g_dir_close (dir);
}

static void
run_some_languages (void)
{
	char **langs;

	for (langs = option_langs; *langs; langs++) {
		char *raw_filename;
		char *filename;

		raw_filename = g_strconcat (*langs, ".dat", NULL);
		filename = g_build_filename (option_data_dir, raw_filename, NULL);
		g_free (raw_filename);

		run_one_language (*langs, filename);
		g_free (filename);
	}
}

static gboolean
have_all_languages (char **langs)
{
	for (; *langs; langs++)
		if (strcmp (*langs, "ALL") == 0)
			return TRUE;

	return FALSE;
}

int
main (int argc, char **argv)
{
	GOptionContext *option_ctx;

	gtk_init (&argc, &argv);
	
	option_ctx = g_option_context_new ("Options");
	g_option_context_add_main_entries (option_ctx, option_entries, NULL);
	if (!g_option_context_parse (option_ctx, &argc, &argv, NULL)) {
		fprintf (stderr, "Invalid usage; type \"%s --help\" for instructions.\n", argv[0]);
		exit (EXIT_FAILURE);
	}

	dummy_dialog ();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_show_now (window);

	if (option_output) {
		output_file = fopen (option_output, "w");
		if (!output_file)
			error_and_exit ("Could not create output file %s", option_output);
	} else
		output_file = stdout;

	fputs ("<?xml version=\"1.0\"?>\n", output_file);
	fputs ("<pango-benchmark>\n", output_file);
	fprintf (output_file, "  <name>%s</name>\n", option_name);

	if (option_langs == NULL || have_all_languages (option_langs))
		run_all_languages ();
	else
		run_some_languages ();

	fputs ("</pango-benchmark>\n", output_file);

	if (output_file != stdout)
		fclose (output_file);

	gtk_widget_destroy (window);

	return 0;
}
