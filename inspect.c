#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>

char *format_permissions(mode_t mode);
char *format_time(time_t time_val, int human_readable);
char *format_size(off_t size, int human_readable);
void print_inode_info_json(const char *file_path, struct stat *fileInfo, int human_readable);
void print_inode_info_text(const char *file_path, struct stat *fileInfo, int human_readable);
void display_help(const char *program_name);
void log_operation(const char *log_file, const char *operation);
void process_directory(const char *directory_path, int json_output, int human_readable, const char *log_file, int recursive);

int main(int argc, char *argv[]) {
    int opt;
    int human_readable = 0;
    int json_output = 0; 
    char *file_path = NULL;
    char *log_file = NULL;
    char *directory_path = NULL;
    int recursive = 0;

    while ((opt = getopt(argc, argv, "i:ha:rf:l:?")) != -1) {
        switch (opt) {
            case 'i':
                file_path = optarg;
                break;
            case 'h':
                human_readable = 1;
                break;
            case 'a':
                directory_path = optarg;
                break;
            case 'r':
                recursive = 1;
                break;
            case 'f':
                json_output = (strcmp(optarg, "json") == 0);
                break;
            case 'l':
                log_file = optarg;
                break;
            case '?':
                display_help(argv[0]);
                exit(EXIT_SUCCESS);
            default:
                fprintf(stderr, "Usage: %s -i <file_path> [-h] [-f json|text] [-l log_file]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!file_path && !directory_path) {
        display_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (file_path && directory_path) {
        fprintf(stderr, "Error: Both file and directory paths cannot be specified together.\n");
        display_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (file_path) {
        struct stat fileInfo;
        if (stat(file_path, &fileInfo) != 0) {
            fprintf(stderr, "Error getting file info for %s: %s\n", file_path, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (json_output) {
            print_inode_info_json(file_path, &fileInfo, human_readable);
        } else {
            print_inode_info_text(file_path, &fileInfo, human_readable);
        }
    } else if (directory_path) {
        process_directory(directory_path, json_output, human_readable, log_file, recursive);
    }

    if (log_file) {
        log_operation(log_file, "Completed operation");
    }

    return 0;
}

void display_help(const char *program_name) {
    fprintf(stdout, "Usage: %s -i <file_path> [-h] [-f json|text] [-l log_file]\n", program_name);
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "  -i, --inode <file_path>      Display detailed inode information for the specified file.\n");
    fprintf(stdout, "  -a, --all [directory_path]   Display inode information for all files within the specified directory.\n");
    fprintf(stdout, "  -r, --recursive              Recursive listing.\n");
    fprintf(stdout, "  -h, --human                  Output sizes and dates in a human-readable form.\n");
    fprintf(stdout, "  -f, --format [text|json]     Specify the output format. This option is required.\n");
    fprintf(stdout, "  -l, --log <log_file>         Log operations to a specified file.\n");
    fprintf(stdout, "  -?, --help                   Display this help and exit.\n");
}

void log_operation(const char *log_file, const char *operation) {
    FILE *log_fp = fopen(log_file, "a");
    if (!log_fp) {
        fprintf(stderr, "Error opening log file %s: %s\n", log_file, strerror(errno));
        exit(EXIT_FAILURE);
    }
    time_t current_time;
    char *time_str;
    time(&current_time);
    time_str = ctime(&current_time);
    time_str[strlen(time_str) - 1] = '\0'; 
    fprintf(log_fp, "[%s] %s\n", time_str, operation);
    fclose(log_fp);
}

void process_directory(const char *directory_path, int json_output, int human_readable, const char *log_file, int recursive) {
    DIR *dir;
    struct dirent *entry;
    if (!(dir = opendir(directory_path))) {
        fprintf(stderr, "Error: Unable to open directory %s: %s\n", directory_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", directory_path, entry->d_name);
        struct stat fileInfo;
        if (stat(path, &fileInfo) != 0) {
            fprintf(stderr, "Error getting file info for %s: %s\n", path, strerror(errno));
            continue;
        }

        if (json_output) {
            print_inode_info_json(path, &fileInfo, human_readable);
        } else {
            print_inode_info_text(path, &fileInfo, human_readable);
        }

        if (recursive && S_ISDIR(fileInfo.st_mode)) {
            process_directory(path, json_output, human_readable, log_file, recursive);
        }
    }
    closedir(dir);
}

char *format_permissions(mode_t mode) {
    static char perms[11];
    snprintf(perms, sizeof(perms), "%c%c%c%c%c%c%c%c%c%c",
             S_ISDIR(mode)    ? 'd'
                               : S_ISLNK(mode)  ? 'l'
                                                : S_ISREG(mode)  ? '-'
                                                                 : S_ISCHR(mode)  ? 'c'
                                                                                  : S_ISBLK(mode)  ? 'b'
                                                                                                   : S_ISFIFO(mode) ? 'p'
                                                                                                                    : S_ISSOCK(mode) ? 's'
                                                                                                                                     : '?',
             mode & S_IRUSR ? 'r' : '-', mode & S_IWUSR ? 'w' : '-',
             mode & S_IXUSR ? 'x' : '-', mode & S_IRGRP ? 'r' : '-',
             mode & S_IWGRP ? 'w' : '-', mode & S_IXGRP ? 'x' : '-',
             mode & S_IROTH ? 'r' : '-', mode & S_IWOTH ? 'w' : '-',
             mode & S_IXOTH ? 'x' : '-');
    return perms;
}

char *format_time(time_t time_val, int human_readable) {
    static char buf[256];
    if (human_readable) {
        struct tm *tm = localtime(&time_val);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    } else {
        snprintf(buf, sizeof(buf), "%ld", (long)time_val);
    }
    return buf;
}

char *format_size(off_t size, int human_readable) {
    static char formatted_size[64];
    if (human_readable) {
        if (size >= 1073741824) {
            sprintf(formatted_size, "%.2f GB", size / 1073741824.0);
        } else if (size >= 1048576) {
            sprintf(formatted_size, "%.2f MB", size / 1048576.0);
        } else if (size >= 1024) {
            sprintf(formatted_size, "%.2f KB", size / 1024.0);
        } else {
            sprintf(formatted_size, "%lld bytes", (long long)size);
        }
    } else {
        sprintf(formatted_size, "%lld", (long long)size);
    }
    return formatted_size;
}

void print_inode_info_json(const char *file_path, struct stat *fileInfo, int human_readable) {
    printf("{\n");
    printf("  \"filePath\": \"%s\",\n", file_path);
    printf("  \"inode\": {\n");
    printf("    \"number\": %lu,\n", (unsigned long)fileInfo->st_ino);
    printf("    \"type\": \"%s\",\n",
           S_ISDIR(fileInfo->st_mode)    ? "directory"
           : S_ISREG(fileInfo->st_mode)  ? "regular file"
           : S_ISLNK(fileInfo->st_mode)  ? "symbolic link"
           : S_ISCHR(fileInfo->st_mode)  ? "character device"
           : S_ISBLK(fileInfo->st_mode)  ? "block device"
           : S_ISFIFO(fileInfo->st_mode) ? "FIFO"
           : S_ISSOCK(fileInfo->st_mode) ? "socket"
                                         : "unknown");
    printf("    \"permissions\": \"%s\",\n", format_permissions(fileInfo->st_mode));
    printf("    \"linkCount\": %lu,\n", (unsigned long)fileInfo->st_nlink);
    printf("    \"uid\": %u,\n", fileInfo->st_uid);
    printf("    \"gid\": %u,\n", fileInfo->st_gid);
    printf("    \"size\": \"%s\",\n", format_size(fileInfo->st_size, human_readable));
    printf("    \"accessTime\": \"%s\",\n", format_time(fileInfo->st_atime, human_readable));
    printf("    \"modificationTime\": \"%s\",\n", format_time(fileInfo->st_mtime, human_readable));
    printf("    \"statusChangeTime\": \"%s\"\n", format_time(fileInfo->st_ctime, human_readable));
    printf("  }\n");
    printf("}\n");
}

void print_inode_info_text(const char *file_path, struct stat *fileInfo, int human_readable) {
    printf("Information for %s:\n", file_path);
    printf("File Inode: %lu\n", (unsigned long)fileInfo->st_ino);
    printf("File Type: %s\n",
           S_ISDIR(fileInfo->st_mode)    ? "directory"
           : S_ISREG(fileInfo->st_mode)  ? "regular file"
           : S_ISLNK(fileInfo->st_mode)  ? "symbolic link"
           : S_ISCHR(fileInfo->st_mode)  ? "character device"
           : S_ISBLK(fileInfo->st_mode)  ? "block device"
           : S_ISFIFO(fileInfo->st_mode) ? "FIFO"
           : S_ISSOCK(fileInfo->st_mode) ? "socket"
                                         : "unknown");
    printf("Permissions: %s\n", format_permissions(fileInfo->st_mode));
    printf("Number of Hard Links: %lu\n", (unsigned long)fileInfo->st_nlink);
    printf("Owner UID: %u\n", fileInfo->st_uid);
    printf("Group GID: %u\n", fileInfo->st_gid);
    printf("File Size: %s\n", format_size(fileInfo->st_size, human_readable));
    printf("Last Access Time: %s\n", format_time(fileInfo->st_atime, human_readable));
    printf("Last Modification Time: %s\n", format_time(fileInfo->st_mtime, human_readable));
    printf("Last Status Change Time: %s\n", format_time(fileInfo->st_ctime, human_readable));
}
