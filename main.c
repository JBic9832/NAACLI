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
#include <limits.h>
#define PATH_SIZE PATH_MAX
#endif

#define AC_RED     "\x1B[31m"
#define AC_GREEN   "\x1B[32m"
#define AC_YELLOW  "\x1B[33m"
#define AC_BLUE    "\x1B[34m"
#define AC_MAGENTA "\x1B[35m"
#define AC_CYAN    "\x1B[36m"
#define AC_NORMAL  "\x1B[0m" // Code to reset color

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

void open_db(sqlite3** db, const char* cwd) {
	char buffer[PATH_SIZE];
	sprintf(buffer, "%s/notes.db", cwd);
	int res = sqlite3_open(buffer, db);
	if(res) {
		fprintf(stderr, "ERROR: Failed to open db connection!");
	}
}

int create_entry(sqlite3** db, const char* category, const char* content) {
	int res;

	char create_table_buffer[512];
	snprintf(create_table_buffer, sizeof(create_table_buffer), "CREATE TABLE IF NOT EXISTS %s (ID INTEGER PRIMARY KEY, CONTENT TEXT NOT NULL)", category);

	const char* sql = create_table_buffer;

	char* error_message = 0;
	res = sqlite3_exec(*db, sql, NULL, NULL, &error_message);
	if (res != SQLITE_OK) {
		fprintf(stderr, "ERROR: Failed to create table %s: %s - %s\n", category, error_message, sqlite3_errstr(res));
		sqlite3_free(error_message);
	}

	sqlite3_stmt* create_stmt;
	char query_buffer[512];
	snprintf(query_buffer, sizeof(query_buffer), "INSERT INTO %s (CONTENT) VALUES (\"%s\")", category, content);
	res = sqlite3_prepare_v2(*db, query_buffer, -1, &create_stmt, 0);
	if (res != SQLITE_OK) {
		fprintf(stderr, "ERROR: Could not create entry in %s. (%s), (%s)\n", category, sqlite3_errmsg(*db), sqlite3_errstr(res));
		return -1;
	}

	if (sqlite3_step(create_stmt) != SQLITE_DONE) {
		fprintf(stderr, "ERROR: Failed to execute INSERT statement. (%s), (%s)\n", sqlite3_errmsg(*db), sqlite3_errstr(res));
		return -1;
	}

	sqlite3_reset(create_stmt);
	sqlite3_finalize(create_stmt);

	printf("Created entry in %s: %s\n", category, content);
	return 0;
}

void handle_db_create_entry(const char* category, const char* content, const char* cwd) {
	sqlite3* db;
	open_db(&db, cwd);

	create_entry(&db, category, content);

	sqlite3_close(db);
}

void list_all_in_category(sqlite3** db, const char* category) {
	char query_buffer[512];
	snprintf(query_buffer, sizeof(query_buffer), "SELECT * FROM %s", category);

	sqlite3_stmt* stmt;
	int res = sqlite3_prepare_v2(*db, query_buffer, -1, &stmt, 0);
	if (res != SQLITE_OK) {
		fprintf(stderr, "ERROR: Failed to prepare statement - %s. (%s), (%s)\n", query_buffer, sqlite3_errmsg(*db), sqlite3_errstr(res));
		return;
	}

	printf("%s:\n", category);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		const char* content = (const char*)sqlite3_column_text(stmt, 1);
		printf("\t%d: %s\n", id, content);
	}

	sqlite3_finalize(stmt);
}

void handle_list_all_in_category(const char* category, const char* cwd) {
	sqlite3* db;
	open_db(&db, cwd);

	list_all_in_category(&db, category);

	sqlite3_close(db);
}

void list_all(sqlite3 **db) {
	char* table_query = "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';";

	sqlite3_stmt* table_stmt;
	int res = sqlite3_prepare_v2(*db, table_query, -1, &table_stmt, 0);
	if (res != SQLITE_OK) {
		fprintf(stderr, "ERROR: Failed to get tables. (%s), (%s)\n", sqlite3_errmsg(*db), sqlite3_errstr(res));
		return;
	}

	char* table_names[512];

	int i = 0;
	while (sqlite3_step(table_stmt) == SQLITE_ROW) {
		char* res = (char*)sqlite3_column_text(table_stmt, 0);
		// sqlite3_column_text returns temporary memory which is
		// cleared after sqlite3_step meaning the strings are invalid
		// so we must duplicate them
		char* name = strdup((const char*)res);
		table_names[i] = name;
		++i;
	}

	sqlite3_stmt* stmt;
	for (int j = 0; j < i; ++j) {
		char* name = strdup((const char*)table_names[j]);
		char query[512];
		snprintf(query, sizeof(query), "SELECT * FROM %s", name);
		res = sqlite3_prepare_v2(*db, query, -1, &stmt, 0);
		if (res != SQLITE_OK) {
			fprintf(stderr, "ERROR: Failed to select items from table %s. (%s), (%s)\n", name, sqlite3_errmsg(*db), sqlite3_errstr(res));
			return;
		}

		printf("%s%s\n%s", AC_GREEN, name, AC_NORMAL);
		printf("%s\tID   CONTENT\n%s", AC_BLUE, AC_NORMAL);
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			int id = sqlite3_column_int(stmt, 0);
			char* content = sqlite3_column_text(stmt, 1);
			printf("\t%d: %s\n", id, content);
		}

		printf("\n");
	}
}

void handle_list_all(const char* cwd) {
	sqlite3* db;
	open_db(&db, cwd);

	list_all(&db);

	sqlite3_close(db);
}

bool is_table_empty(sqlite3** db, const char* table) {
	sqlite3_stmt* stmt;
	char query[512];
	snprintf(query, sizeof(query), "SELECT EXISTS (SELECT 1 FROM %s);", table);

	int res = sqlite3_prepare_v2(*db, query,-1, &stmt, 0);
	if (res != SQLITE_OK) {
		fprintf(stderr, "ERROR: Failed to compile command for table empty. (%s), (%s)", sqlite3_errmsg(*db), sqlite3_errstr(res));
		return false;
	}

	int step_result = sqlite3_step(stmt);

	if (step_result == SQLITE_ROW) {
		int exists = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);
		return exists == 0;
	} else if (step_result == SQLITE_DONE) {
		//This should not happen
		sqlite3_finalize(stmt);
		return true;
	} else {
		fprintf(stderr, "ERROR: Failed to check if table empty. (%s), (%s)", sqlite3_errmsg(*db), sqlite3_errstr(step_result));
		return false;
	}
}

void delete_entry(sqlite3** db, const char* table, const char* id) {
	char query_buffer[512];
	snprintf(query_buffer, sizeof(query_buffer), "DELETE FROM %s WHERE ID = %s", table, id);

	sqlite3_stmt* stmt;

	char* err = 0;
	int res = sqlite3_exec(*db, query_buffer, NULL, NULL, &err);
	if (res != SQLITE_OK) {
		fprintf(stderr, "%sFailed to execute deletion. (%s), (%s)%s\n", AC_RED, sqlite3_errmsg(*db), err, AC_NORMAL);
		return;
	}

	printf("%sDeleted #%s from %s%s\n", AC_GREEN, id, table, AC_NORMAL);

	if (is_table_empty(db, table)) {
		char buffer[256];
		snprintf(buffer, sizeof(buffer), "DROP TABLE %s;", table);

		int res = sqlite3_exec(*db, buffer, NULL, NULL, &err);
		if (res != SQLITE_OK) {
			fprintf(stderr, "%sFailed to execute deletion. (%s), (%s), (%s)%s\n", AC_RED, sqlite3_errmsg(*db), err, sqlite3_errstr(res), AC_NORMAL);
			return;
		}

		printf("%sDeleted table due to being empty: \"%s\"%s\n", AC_GREEN, table, AC_NORMAL);
	}
}

void handle_delete(const char* table, const char* id, const char* cwd) {
	sqlite3* db;
	open_db(&db, cwd);

	delete_entry(&db, table, id);

	sqlite3_close(db);
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
			handle_list_all(cwd_path);
			return 0;
		}
		const char* category = argv[2];
		handle_list_all_in_category(category, cwd_path);

		// Select rows from category and display them
		return 0;
	}

	if (strcmp(argv[1], "add") == 0) {
		// check if enough args exist
		if (argc == 4) {
			printf("Adding to docs: %s, %s\n", argv[2], argv[3]);
			// create entry
			handle_db_create_entry(argv[2], argv[3], cwd_path);
		} else {
			fprintf(stderr, "ERROR: Invalid usage!\n");
			print_help_screen(info);
			return -1;
		}

		return 0;
	}

	if (strcmp(argv[1], "delete") == 0 || strcmp(argv[1], "check") == 0) {
		if (argc != 4) {
			fprintf(stderr, "ERROR: Invalid usage!\n");
			print_help_screen(info);
			return -1;
		}

		handle_delete(argv[2], argv[3], cwd_path);

		return 0;
	}

	print_help_screen(info);

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

    return process_input(argc, argv, cwd_path, app_info);
}
