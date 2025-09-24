#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <limits.h>

enum FileType {
    NORMAL,
    DIRECTORY,
    EXECUTABLE,
    MEDIA,
    ARCHIVE,
    HIDDEN,
    SYMLINK,
    CODE
};

typedef struct File {
    int type;
    char* file_path;
    size_t file_size;
    time_t last_modified;
} File;


char* RED = "\x1b[38;2;255;0;0m";
char* ORANGE = "\x1b[38;2;230;76;0m";
char* YELLOW = "\x1b[38;2;230;226;0m";
char* GREEN = "\x1b[38;2;0;186;40m";
char* BLUE = "\x1b[38;2;0;72;255m";
char* INDIGO = "\x1b[38;2;84;0;230m";
char* VIOLET = "\x1b[38;2;176;0;230m";
char* ANSII_RESET = "\x1b[0m";

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

int is_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;       // cannot stat file
    if (!S_ISREG(st.st_mode)) return 0;       // must be a regular file
    return (st.st_mode & S_IXUSR) != 0 ||         // user executable
           (st.st_mode & S_IXGRP) != 0 ||         // group executable
           (st.st_mode & S_IXOTH) != 0;           // other executable
}

int is_symlink(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return 0;
    return S_ISLNK(st.st_mode);
}

int is_hidden(const char *file_name) {
    return file_name[0] == '.';
}

int is_archive(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return 0;

    const char *archives[] = {".zip", ".tar", ".gz", ".bz2", ".7z", ".rar", ".xz"};
    size_t n = sizeof(archives)/sizeof(archives[0]);
    for (size_t i = 0; i < n; i++) {
        if (strcasecmp(ext, archives[i]) == 0) return 1;
    }
    return 0;
}

int is_media(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return 0;

    const char *media[] = {
        ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".tiff", ".ico", // images
        ".mp3", ".wav", ".flac", ".ogg", ".m4a",                  // audio
        ".mp4", ".mov", ".avi", ".mkv", ".webm", ".flv"           // video
    };
    size_t n = sizeof(media)/sizeof(media[0]);
    for (size_t i = 0; i < n; i++) {
        if (strcasecmp(ext, media[i]) == 0) return 1;
    }
    return 0;
}

int is_code_file(const char *path) {
    const char *ext = strrchr(path, '.'); // find last dot
    if (!ext) return 0;

    const char *code_exts[] = {
        ".c", ".h", ".cpp", ".hpp", ".cc", ".hh",   // C/C++
        ".py", ".java", ".js", ".ts", ".jsx", ".tsx", // Python, Java, JS/TS
        ".go", ".rs", ".swift", ".kt", ".kts",     // Go, Rust, Swift, Kotlin
        ".php", ".rb", ".sh", ".pl", ".lua",        // PHP, Ruby, Shell, Perl, Lua
        ".cs", ".m", ".r", ".scala", ".dart",        // C#, Objective-C, R, Scala, Dart
        ".md", ".xml", ".html", ".csv", ".css",       // other
        ".json", ".yml", ".bat", ".sql",       // other
    };

    size_t n = sizeof(code_exts)/sizeof(code_exts[0]);
    for (size_t i = 0; i < n; i++) {
        if (strcasecmp(ext, code_exts[i]) == 0) return 1;
    }
    return 0;
}

int is_html(const char *path) {
    const char *ext = strrchr(path, '.'); // find last dot
    if (!ext) return 0;

    const char *code_exts[] = {
        ".html", ".css"
    };

    size_t n = sizeof(code_exts)/sizeof(code_exts[0]);
    for (size_t i = 0; i < n; i++) {
        if (strcasecmp(ext, code_exts[i]) == 0) return 1;
    }
    return 0;
}

void print_rainbow(const char *str) {
    const char *colors[] = { RED, ORANGE, YELLOW, GREEN, BLUE, INDIGO, VIOLET };
    for (int i = 0; i < strlen(str); i++) {
        printf("%s%c", colors[i%7], str[i]);
    }
    printf("%s\n", ANSII_RESET);
}


int compare_files_by_name(const void *a, const void *b) {
    File *const *a_ptr = (File *const *)a;
    File *const *b_ptr = (File *const *)b;
    return strcasecmp((*a_ptr)->file_path, (*b_ptr)->file_path);  // returns positive, zero, or negative
}

void set(List* list, size_t index, void* data) {
    // resize if needed
    if (index >= list->array_size) {
        list->array_size = list->array_size * 1.2 + 10;
        void** new_array = malloc(sizeof(void*) * list->array_size);
        memcpy(new_array, list->array, list->len * sizeof(void*));
        free(list->array);
        list->array = new_array;
        // printf("resize %d\n", list->array_size);
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
        file->file_path = malloc(strlen(entry->d_name)+1);
        strcpy(file->file_path, entry->d_name);
        file->file_size = info.st_size;
        file->last_modified = info.st_mtime;

        // Check if entry is a directory or a file
        int is_dir = 0;
        if (is_hidden(file->file_path)) {
            file->type = HIDDEN;
        }
        else if (S_ISDIR(info.st_mode)) {
            file->type = DIRECTORY;
        }
        else if (is_archive(file->file_path)) {
            file->type = ARCHIVE;
        }
        else if (is_media(file->file_path)) {
            file->type = MEDIA;
        }
        else if (is_executable(path)) {
            file->type = EXECUTABLE;
        }
        else if (is_symlink(path)) {
            file->type = SYMLINK;   
        }
        else if (is_code_file(file->file_path)) {
            file->type = CODE;
        }
        else {
            file->type = NORMAL;
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
        char size[10];
        if (file->type != DIRECTORY) {
            double file_size = file->file_size;
            
            // PB           
            if (file_size > 1000000000000000) {
                file_size /= 1000000000000000;
                char val[5];
                truncate_6(file_size, val);
                snprintf(size, 10, "%s PB    ", val);
            }
            // TB
            else if (file_size > 1000000000000) {
                file_size /= 1000000000000;
                char val[5];
                truncate_6(file_size, val);
                snprintf(size, 10, "%s TB    ", val);
            }
            // GB
            else if (file_size > 1000000000) {
                file_size /= 1000000000;
                char val[5];
                truncate_6(file_size, val);
                snprintf(size, 10, "%s GB    ", val);
            }
            // MB
            else if (file_size > 1000000) {
                file_size /= 1000000;
                char val[5];
                truncate_6(file_size, val);
                snprintf(size, 10, "%s MB    ", val);
            }
            // KB
            else if (file_size > 1000) {
                file_size /= 1000;
                char val[5];
                truncate_6(file_size, val);
                snprintf(size, 10, "%s KB    ", val);
            }
            // bytes
            else {
                char val[5];
                truncate_6(file_size, val);
                snprintf(size, 10, "%s B     ", val);
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
            size[8] = ' ';
            size[9] = '\0';
        }
        printf("%s     ", size);


        // file name
        if (file->type == DIRECTORY) {
            //17, 34, 189
            printf("\x1b[38;2;17;34;189m%s\x1b[0m\n", file->file_path);
        }
        else if (file->type == EXECUTABLE) {
            // 17, 122, 28
            printf("\x1b[38;2;17;122;28m%s*\x1b[0m\n", file->file_path);
        }
        else if (file->type == MEDIA) {
            // 171, 176, 23
            printf("\x1b[38;2;171;176;23m%s\x1b[0m\n", file->file_path);
        }
        else if (file->type == ARCHIVE) {
            // 173, 26, 48
            printf("\x1b[38;2;173;26;48m%s\x1b[0m\n", file->file_path);
        }
        else if (file->type == HIDDEN) {
            // 107, 107, 107
            printf("\x1b[38;2;107;107;107m%s\x1b[0m\n", file->file_path);
        }
        else if (file->type == SYMLINK) {
            // 26, 137, 173
            printf("\x1b[38;2;26;137;173m%s\x1b[0m\n", file->file_path);
        }
        else if (file->type == CODE) {
            if (is_html(file->file_path)) {
                print_rainbow(file->file_path);
            }
            else {
                // 110, 56, 2
                printf("\x1b[38;2;110;56;2m%s\x1b[0m\n", file->file_path);
            }
        }
        else {
            printf("%s\n", file->file_path);
        }

        /*

        Folder / directory	Blue	Most terminals default to blue. Makes directories stand out.
Regular file	Default / White	Keeps files unobtrusive.
Executable file	Green	Common convention in ls --color=auto and many shells.
Symbolic link	Cyan	Usually bright cyan or light blue.
ARCHIVE / archive	Red / Magenta	tar, zip, etc.
Image / media	Yellow / Magenta	Optional; can help distinguish media.
Hidden files	Dim / Gray	Prefixing with dot (.) and dim color is common.
        */
    }


    for (int i = 0; i < files->len; ++i) {
        File* file = files->array[i];
        free(file->file_path);
        free(file);
    }


    closedir(dir);
    free_list(files);
    
}