#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <limits.h>


typedef struct File {
    int is_dir;
    char* file_path;
    size_t file_size;
    time_t last_modified;
} File;


typedef struct List {
    void** array;
    size_t array_size;
    size_t len;
} List;

List* list() {
    List* list = malloc(sizeof(List));
    list->array = malloc(sizeof(void*) * 10);
    list->array_size = 10;
    list->len = 0;

    return list;
}

int compare_files_by_name(const void *a, const void *b) {
    File *const *a_ptr = (File *const *)a;
    File *const *b_ptr = (File *const *)b;
    return strcasecmp((*a_ptr)->file_path, (*b_ptr)->file_path);  // returns positive, zero, or negative
}

void set(List* list, size_t index, void* data) {
    // resize if needed
    if (index >= list->array_size) {
        list->array_size *= 2;
        void** new_array = malloc(sizeof(void*) * list->array_size);
        memcpy(new_array, list->array, list->len * sizeof(void*));
        free(list->array);
        list->array = new_array;
    }

    // set
    list->array[index] = data;

    if (list->len == index) {
        ++list->len;
    }
}

void free_list(List* list) {
    free(list->array);
    free(list);
}

void truncate_6(double d, char* dest) {
    snprintf(dest, 7, "%.2f    ", d);
}


int main(int argc, char *argv[]) {
    char current_working_dir[PATH_MAX]; // buffer to hold the directory path

    if (getcwd(current_working_dir, sizeof(current_working_dir)) == NULL) {
        perror("getcwd() error");
        return 1;
    }

    // char* current_working_dir = "/home/brandon/Documents";


    // collect files and dirs
    List* files = list();
    DIR *dir = opendir(current_working_dir);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Build full path for stat()
        char path[PATH_MAX];
        int ret = snprintf(path, PATH_MAX, "%s/%s", current_working_dir, entry->d_name);


        struct stat info;
        stat(path, &info);


        File* file = malloc(sizeof(File));
        file->file_path = malloc(strlen(entry->d_name));
        strcpy(file->file_path, &entry->d_name);
        file->file_size = info.st_size;
        file->last_modified = info.st_mtime;

        // Check if entry is a directory or a file
        int is_dir = 0;
        if (S_ISDIR(info.st_mode)) {
            file->is_dir = 1;
        }
        else {
            file->is_dir = 0;
        }
        set(files, files->len, file);

    }


    // sort by name
    qsort(files->array, files->len, sizeof(File*), compare_files_by_name);


    // print out
    for(int i = 0; i < files->len; ++i) {
        File* file = files->array[i];

        // last modified
        struct tm *local_time = localtime(&file->last_modified);
        char time_str[100];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
        printf("\x1b[38;2;60;60;60m%s\x1b[0m     ", time_str);


        // size
        char size[9];
        if (!file->is_dir) {
            double file_size = file->file_size;
            
            // PB           
            if (file_size > 1000000000000000) {
                file_size /= 1000000000000000;
                char val[5];
                truncate_6(file_size, val);
                snprintf(size, 9, "%s PB    ", val);
            }
            // TB
            if (file_size > 1000000000000) {
                file_size /= 1000000000000;
                char val[5];
                truncate_6(file_size, val);
                snprintf(size, 9, "%s TB    ", val);
            }
            // GB
            if (file_size > 1000000000) {
                file_size /= 1000000000;
                char val[5];
                truncate_6(file_size, val);
                snprintf(size, 9, "%s GB    ", val);
            }
            // MB
            if (file_size > 1000000) {
                file_size /= 1000000;
                char val[5];
                truncate_6(file_size, val);
                snprintf(size, 9, "%s MB    ", val);
            }
            // KB
            else if (file_size > 1000) {
                file_size /= 1000;
                char val[5];
                truncate_6(file_size, val);
                snprintf(size, 9, "%s KB    ", val);
            }
            // bytes
            else {
                char val[5];
                truncate_6(file_size, val);
                snprintf(size, 9, "%s B     ", val);
            }
        }
        else {
            size[0] = '-';
            size[1] = '-';
            size[2] = ' ';
            size[3] = ' ';
            size[4] = ' ';
            size[5] = ' ';
            size[6] = ' ';
            size[7] = ' ';
            size[8] = '\0';
        }
        printf("%s     ", size);


        // file name
        if (file->is_dir) {
            //12, 179, 9
            printf("\x1b[38;2;12;179;9m%s\x1b[0m\n", file->file_path);
        }
        else {
            printf("%s\n", file->file_path);
        }

    }


    for (int i = 0; i < files->len; ++i) {
        File* file = files->array[i];
        free(file->file_path);
        free(file);
    }


    closedir(dir);
    free_list(files);
    
}