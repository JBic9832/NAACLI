#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "sqlite3.h"

// Include the platform specific header file for cwd check
#if defined (_WIN32)
#include <direct.h>
#else
#include <unistd.h>
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

int open_db(sqlite3* db, const char* cwd) {
	char buffer[PATH_MAX];
	sprintf(buffer, "%s/notes.db", cwd);
	int res = sqlite3_open(buffer, &db);
	if(res) {
		fprintf(stderr, "ERROR: Failed to open db connection!");
		return res;
	}

	return 0;
}

int create_entry(sqlite3* db, const char* category, const char* content) {
	int res;

	char create_table_buffer[512];
	snprintf(create_table_buffer, sizeof(create_table_buffer), "CREATE TABLE IF NOT EXISTS %s (id INTEGER PRIMARY KEY, content TEXT)", category);

	const char* sql = create_table_buffer;

	char* error_message = 0;
	res = sqlite3_exec(db, sql, 0, 0, &error_message);
	if (res != SQLITE_OK) {
		fprintf(stderr, "ERROR: Failed to create table %s: %s - %s\n", category, error_message, sqlite3_errstr(res));
		sqlite3_free(error_message);
		return -1;
	}

	sqlite3_stmt* create_stmt;
	char create_entry_buffer[1024];
	sprintf(create_entry_buffer, "INSERT INTO %s (content) VALUES (%s)", category, content);
	res = sqlite3_prepare_v2(db, create_entry_buffer, -1, &create_stmt, 0);
	if (res != SQLITE_OK) {
		fprintf(stderr, "ERROR: Could not create entry in %s\n", category);
		return -1;
	}

	printf("Created entry in %s: %s\n", category, content);
	return 0;
}

int handle_db_create_entry(const char* category, const char* content, const char* cwd) {
	sqlite3* db;
	open_db(db, cwd);

	create_entry(db, category, content);

	sqlite3_close(db);
	return 0;
}

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
int process_input(int argc, char* argv[], const char* cwd_path, CLI_Info info) {
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

	// User wants to list notes
	if (strcmp(argv[1], "list") == 0) {
		// User did not specify a category
		if (argc == 2) {
			// Select all categories and list all notes
			return 0;
		}

		// Select rows from category and display them
		return 0;
	}

	if (strcmp(argv[1], "add") == 0) {
		// check if enough args exist
		if (argc == 4) {
			printf("Adding to docs: %s, %s\n", argv[2], argv[3]);
			// create entry
			return handle_db_create_entry(argv[2], argv[3], cwd_path);
		} else {
			fprintf(stderr, "ERROR: Invalid usage!");
			return -1;
		}
	}

    return 0;
}

int main(int argc, char *argv[]) {
    char buffer[PATH_MAX];
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

	printf("CURRENT WORKING DIR: %s\n", cwd_path);

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

    return process_input(argc, argv, cwd_path, app_info);
}
