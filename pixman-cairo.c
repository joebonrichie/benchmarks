// gcc pixman-cairo.c -o pixman-cairo -O3 `pkg-config --cflags --libs cairo`

#include <cairo/cairo.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

double get_time_in_seconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void benchmark_pixman_drawing(int width, int height, int iterations) {
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(surface);

    double start_time = get_time_in_seconds();

    for (int i = 0; i < iterations; i++) {
        // Clear surface
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_paint(cr);

        // Draw random rectangles
        for (int j = 0; j < 100; j++) {
            double x = rand() % width;
            double y = rand() % height;
            double rect_width = rand() % 100;
            double rect_height = rand() % 100;
            cairo_set_source_rgb(cr, (double)rand()/RAND_MAX, (double)rand()/RAND_MAX, (double)rand()/RAND_MAX);
            cairo_rectangle(cr, x, y, rect_width, rect_height);
            cairo_fill(cr);
        }

        // Draw random circles
        for (int j = 0; j < 100; j++) {
            double x = rand() % width;
            double y = rand() % height;
            double radius = rand() % 50;
            cairo_set_source_rgb(cr, (double)rand()/RAND_MAX, (double)rand()/RAND_MAX, (double)rand()/RAND_MAX);
            cairo_arc(cr, x, y, radius, 0, 2 * 3.14159);
            cairo_fill(cr);
        }

        // Flush to ensure rendering happens
        cairo_surface_flush(surface);
    }

    double end_time = get_time_in_seconds();
    double elapsed_time = end_time - start_time;

    printf("Cairo Benchmark - Width: %d, Height: %d, Iterations: %d\n", width, height, iterations);
    printf("Total Time: %.4f seconds\n", elapsed_time);
    printf("Average Time per Iteration: %.4f seconds\n", elapsed_time / iterations);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

int main(int argc, char *argv[]) {
    int width = 800;
    int height = 600;
    int iterations = 1000;

    if (argc == 4) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        iterations = atoi(argv[3]);
    }

    benchmark_pixman_drawing(width, height, iterations);

    return 0;
}
