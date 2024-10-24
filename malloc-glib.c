// gcc -O3 malloc-glib.c `pkg-config --cflags --libs glib-2.0` -o malloc-glib
// 30% runtime of this bench is due to malloc

#include <glib.h>
#include <stdio.h>
#include <time.h>

gint compare_ints(gconstpointer a, gconstpointer b) {
    return (*(const int*)a - *(const int*)b);
}

void garray_bench() {
    GArray *array;
    int i;
    const int num_entries = 100000000;

    // Initialize the GArray
    array = g_array_new(FALSE, FALSE, sizeof(int));

    // Timing the insertion (append)
    clock_t start = clock();
    for (i = 0; i < num_entries; i++) {
        g_array_append_val(array, i);
    }
    clock_t end = clock();
    printf("GArray: Insertion (append) time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    // Timing the deletion (removing elements from the end)
    start = clock();
    for (i = num_entries - 1; i >= 0; i--) {
        g_array_remove_index(array, i);
    }
    end = clock();
    printf("GArray: Deletion time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    // Free the array
    g_array_free(array, TRUE);
}

void glist_bench() {
    GList *list = NULL;
    int i;
    const int num_entries = 100000000;

    // Timing the insertion (prepend)
    clock_t start = clock();
    for (i = 0; i < num_entries; i++) {
        int *value = g_malloc(sizeof(int));
        *value = i;
        list = g_list_prepend(list, value); // Inserting at the head
    }
    clock_t end = clock();
    printf("GList: Insertion (prepend) time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    // Timing the lookup (traverse the list)
    start = clock();
    GList *l;
    for (l = list; l != NULL; l = l->next) {
        int *value = (int *)l->data;
    }
    end = clock();
    printf("GList: Lookup (traverse) time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    // Timing the deletion
    start = clock();
    while (list != NULL) {
        g_free(list->data); // Free the data
        list = g_list_delete_link(list, list); // Remove the node
    }
    end = clock();
    printf("GList: Deletion time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
}

void ghashtable_bench() {
    GHashTable *hash_table;
    int i;
    const int num_entries = 100000000;

    // Initialize the hash table
    hash_table = g_hash_table_new(g_int_hash, g_int_equal);

    // Timing the insertion
    clock_t start = clock();
    for (i = 0; i < num_entries; i++) {
        int *key = g_malloc(sizeof(int));
        int *value = g_malloc(sizeof(int));
        *key = i;
        *value = i * 2;
        g_hash_table_insert(hash_table, key, value);
    }
    clock_t end = clock();
    printf("GHashTable: Insertion time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    // Timing the lookup
    start = clock();
    for (i = 0; i < num_entries; i++) {
        int key = i;
        g_hash_table_lookup(hash_table, &key);
    }
    end = clock();
    printf("GHashTable: Lookup time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    // Free the memory
    g_hash_table_destroy(hash_table);
}

void gtree_bench() {
    GTree *tree;
    int i;
    const int num_entries = 50000000;

    // Initialize the GTree
    tree = g_tree_new(compare_ints);

    // Timing the insertion
    clock_t start = clock();
    for (i = 0; i < num_entries; i++) {
        int *key = g_malloc(sizeof(int));
        int *value = g_malloc(sizeof(int));
        *key = i;
        *value = i * 2;
        g_tree_insert(tree, key, value);
    }
    clock_t end = clock();
    printf("GTree: Insertion time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    // Timing the lookup
    start = clock();
    for (i = 0; i < num_entries; i++) {
        int key = i;
        g_tree_lookup(tree, &key);
    }
    end = clock();
    printf("GTree: Lookup time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    // Timing the deletion
    start = clock();
    for (i = 0; i < num_entries; i++) {
        int key = i;
        g_tree_remove(tree, &key);
    }
    end = clock();
    printf("GTree: Deletion time: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    // Destroy the tree
    g_tree_destroy(tree);
}

int main() {

    garray_bench();
    glist_bench();
    ghashtable_bench();
    //gtree_bench();

    return 0;
}
