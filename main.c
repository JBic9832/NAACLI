#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3.h"

// Include the platform specific header file for cwd check
#if defined (_WIN32)
#include <direct.h>
#define PATH_SIZE _MAX_PATH
#else
#include <unistd.h>
#define PATH_SIZE PATH_MAX
#endif

typedef struct {
    const char* name;
    const char* usage;
} Arg;

typedef struct {
    const char* name;
    const char* description;
    const char* usage;
    const Arg *args;
    const int num_arguments;
} CLI_Info;

// Remove this in production
void debug_arguments(int argc, char* argv[]) {
    for (int i = 0; i < argc; ++i) {
        printf("%d: %s\n", i, argv[i]);
    }
}

void print_help_screen(CLI_Info info) {
    // Name info
    printf("NAME\n");
    printf("\t%s\n\n", info.name);

    // Description
    printf("DESCRIPTION\n");
    printf("\t%s\n\n", info.description);

    // Usage
    printf("USAGE\n");
    printf("\t%s\n\n", info.usage);

    // Arguments
    printf("ARGUMENTS\n");
    printf("\t-h:\tShow help information\n\n");
    for (int i = 0; i < info.num_arguments; ++i) {
        printf("\tArg %d %s:\t%s\n\n", i, info.args[i].name, info.args[i].usage);
    }
    printf("\n");

    // Me
    printf("Author\n");
    printf("\tJoseph Bickford\n");
}

// Refactor for getopt once functionality is fleshed out
int process_args(int argc, char* argv[], char* cwd_path, CLI_Info info) {
    // The user did not supply any arguments
    if (argc <= 1) {
        printf("No arguments supplied!\n");
        // Show the user how to use the app to avoid further confusion
        print_help_screen(info);    
        return -1;
    }

    // Does the user need help??????????????????
    if (strcmp(argv[1], "-h") == 0) {
        print_help_screen(info);
        return 0;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    char buffer[PATH_SIZE];
    char* cwd_path = "";

    #if defined (_WIN32)
    if (_getcwd(buffer, sizeof(buffer)) == NULL) {
        printf("Could not get CWD!");
        return -1;
    }
    #else
    if (getcwd(buffer, sizeof(buffer)) == NULL) {
        printf("Could not get CWD!");
        return -1;
    }
    #endif

    cwd_path = buffer;

    Arg arg0 = {
        "Category",
        "Defines the category the note should be stored in. Example: Todo, Note, Random"
    };

    Arg arg1 = {
        "Content",
        "This is the content of the note. Example: \"Have claude fix all my bugs.\""
    };

    Arg args[] = {
        arg0,
        arg1
    };

    CLI_Info app_info = {
        "NAACLI",
        "NAACLI is: Notes as a Command Line Interface. It is meant to keep track of your thoughts/ideas for your projects. \n\tExample:\n\t\tnaacli todo \"Add support for GLTF.\"",
        "Use this tool inside of your project directory to add notes, todo items, etc. for your project.",
        args,
        2
    };

    return process_args(argc, argv, cwd_path, app_info);
}