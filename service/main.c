#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
/*#include <linux/limits.h>*/
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PROGNAME "adDOCtive"

#define PATH_MAX 4096

// declarations
ssize_t writeall(int fd, const unsigned char *buf, size_t len);
int read_int();
void handle_client();
void handle_pre_login();
void handle_post_login();
void setup_user_dir();
void do_exit();
void do_login();
void do_register();
void do_list_users();
void do_logout();
void do_show_user_info();
void do_change_pw();
void do_list_templates();
void do_show_template();
void do_create_template();
void do_instantiate_template();
void disable_buffering();
void drop_privs();
int main(int argc, char **argv);

struct Command {
    void (*func)();
    const char *name;
};

// global vars
const char *welcome =
    "\n"
    ".----------------------------------------------.\n"
    "|                                              |\n"
    "|            WELCOME TO adDOCtive              |\n"
    "|                                              |\n"
    "|  Providing the world with useless document   |\n"
    "|    management services since April 2015!     |\n"
    "|                                              |\n"
    "'----------------------------------------------'\n"
    "\n";
char *sys_dir, *sys_username;
char username[512];
char password[512];

struct Command post_login_commands[] = {
    { &do_logout, "Logout" },
    { &do_show_user_info, "Show my user information" },
    { &do_list_templates, "List my document templates" },
    { &do_show_template, "Show a specific document template" },
    { &do_create_template, "Create a new document template" },
    { &do_instantiate_template, "Create documents from a template" },
};
struct Command pre_login_commands[] = {
    { &do_exit, "Exit" },
    { &do_register, "Register" },
    { &do_login, "Login" },
    { &do_list_users, "Show users" },
};

// implementations
ssize_t writeall(int fd, const unsigned char *buf, size_t len) {
    const unsigned char *p = buf;
    while (p != buf + len) {
        ssize_t res = write(fd, p, buf + len - p);
        if (res < 0)
            return res;
        p += res;
    }
    return len;
}

int read_int() {
    int x;
    int res = scanf("%d", &x);
    if (res == EOF)
        exit(0);
    for (;;) {
        int c = getchar();
        if (c == EOF)
            exit(0);
        if (c == '\n')
            break;
    }
    if (res == 0)
        return -1;
    return x;
}

void build_user_path(char *buf) {
    strcat(buf, sys_dir);
    strcat(buf, "/users/");
    // BUG: stack-based overrun, probably hard to exploit because no null-bytes
    strcat(buf, username);
}

bool login_valid(const char *user, const char *pw) {
    char pw_file[PATH_MAX] = {0};
    build_user_path(pw_file);
    strcat(pw_file, "/password");
    int fd = open(pw_file, O_RDONLY);
    if (fd < 0) {
        // no such user
        close(fd);
        return 0;
    }
    // BUG: timing side channel
    for(int i = 0;; ++i) {
        char c;
        if (!read(fd, &c, 1)) {
            close(fd);
            return 1;
        }
        if (c != pw[i]) {
            close(fd);
            return 0;
        }
    }
    close(fd);
    return 1;
}

void do_exit() {
    printf("Thank you for choosing " PROGNAME " and see you again soon!\n");
    exit(0);
}

void do_login() {
    printf("Username: ");
    if (!gets(username))
        exit(0);
    printf("Password: ");
    if (!gets(password))
        exit(0);
    if (!login_valid(username, password)) {
        printf("Login failed! Bye.\n");
        return;
    }
    // BUG: format string
    printf("Hi "); printf(username); printf(", how are you today?\n");
    handle_post_login();
}

void setup_user_dir(const char *dir) {
    char path[PATH_MAX] = {0};
    strcat(path, dir);
    // BUG: stack-based overrun
    strcat(path, "/templates");
    mkdir(path, 0755);
}

void do_register() {
    printf("Please provide a username and password for your new account\n");
    printf("Username: ");
    // BUG: BSS buffer overrun
    if (!gets(username))
        exit(0);
    printf("Password: ");
    if (!gets(password))
        exit(0);
    char path[PATH_MAX] = {0};
    build_user_path(path);
    if (!mkdir(path, 0755)) {
        setup_user_dir(path);
        strcat(path, "/password");
        int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);
        writeall(fd, (unsigned char*)password, strlen(password));
        close(fd);
        printf("Registered successfully, you can now log in as ");
        // BUG: format string
        printf(username);
        printf("\n");
    } else {
        printf("This user already exists!\n");
    }
}

void do_list_users() {
    printf("Available users:\n");
    char path[PATH_MAX] = {0};
    // BUG: stack-based overrun
    strcat(path, sys_dir);
    strcat(path, "/users");
    DIR *dir = opendir(path);
    if (!dir) {
        printf("Something went terribly wrong :(\n");
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(dir))) {
        if (ent->d_name[0] != '.') {
            // BUG: format string
            printf(ent->d_name);
            puts("");
        }
    }
}

void do_logout() {
    printf("Bye bye ");
    printf(username);
    printf("\n");
}

void do_show_user_info() {
    char pw_file[PATH_MAX] = {0};
    build_user_path(pw_file);
    strcat(pw_file, "/password");
    int fd = open(pw_file, O_RDONLY);
    printf("User information:\n");
    printf("Username: %s\n", username);
    char buf[50];
    int i = 0;
    // BUG: stack-based overrun
    while (read(fd, buf+(i++), 1))
        ;
    buf[i-1] = 0;
    printf("Password: %s\n", buf);
}

void do_list_templates() {
    printf("Available templates:\n");
    char path[PATH_MAX] = {0};
    build_user_path(path);
    strcat(path, "/templates");
    DIR *dir = opendir(path);
    if (!dir) {
        printf("Something went terribly wrong :(\n");
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(dir))) {
        if (ent->d_name[0] != '.') {
            // BUG: format string
            printf(ent->d_name);
            puts("");
        }
    }
}

void do_show_template() {
    printf("What's the name of the template you want to show? ");
    char buf[1024];
    // BUG: immediate stack-based overrun, very critical
    if (!gets(buf))
        exit(0);
    char path[PATH_MAX] = {0};
    build_user_path(path);
    strcat(path, "/templates/");
    // BUG: stack-based overrun (hard to exploit, no null-bytes)
    // BUG: local file inclusion
    strcat(path, buf);
    printf("Template created at ");
    char cmd[PATH_MAX + 200] = {0};
    // BUG: stack-based overrun (hard to exploit, no null-bytes)
    strcat(cmd, "ls -l '");
    strcat(cmd, path);
    strcat(cmd, "' | awk '{print $6, $7 \", \" $8}'");
    // BUG: shell injection (critical)
    system(cmd);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("Template not found!\n");
        return;
    }
    size_t len;
    while ((len = read(fd, buf, 1024)) > 0)
        fwrite(buf, len, 1, stdout);
}

void do_create_template() {
    printf("What's the name of the template you want to create? ");
    char buf[1024];
    // BUG: immediate stack-based overrun, very critical
    if (!gets(buf))
        exit(0);
    char path[PATH_MAX] = {0};
    build_user_path(path);
    strcat(path, "/templates/");
    // BUG: stack-based overrun (hard to exploit, no null-bytes)
    // BUG: local file inclusion
    strcat(path, buf);
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0400);
    if (fd < 0) {
        printf("Could not open template file for writing, maybe it already exists?\n");
        return;
    }
    // TODO better text
    printf(
        "\n"
        "INSTRUCTIONS FOR CREATING DOCUMENT TEMPLATES\n"
        "\n"
        "You can use up to 3 variables in your input via the syntax [variable number!]\n"
        "Example:\n"
        "\n"
        "  Hello Mr. [1!],\n"
        "\n"
        "  I regret to inform you that your software [2!] sucks.\n"
        "\n"
        "  Sincerely yours,\n"
        "  [3!]\n"
        "\n"
        "The variable values can be initialized later when instantiating the template.\n"
        "Type your template below. End it with a single line containing the string EOF:\n"
    );
    for (;;) {
        // BUG: immediate stack-based overrun, very critical
        if (!gets(buf))
            exit(0);
        if (!strcmp(buf, "EOF"))
            break;
        writeall(fd, (unsigned char*)buf, strlen(buf));
        writeall(fd, (unsigned char*)"\n", 1);
    }
}

void do_instantiate_template() {
    printf("What's the name of the template you want to use? ");
    char buf[1024];
    // BUG: immediate stack-based overrun, very critical
    if (!gets(buf))
        exit(0);
    char path[PATH_MAX] = {0};
    build_user_path(path);
    strcat(path, "/templates/");
    // BUG: stack-based overrun (hard to exploit, no null-bytes)
    // BUG: local file inclusion
    strcat(path, buf);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("Template not found!\n");
        return;
    }
    char *tmpl = malloc(0);
    size_t tmpl_size = 0;
    ssize_t len;
    while ((len = read(fd, buf, 1024)) > 0) {
        tmpl = realloc(tmpl, tmpl_size + len + 1);
        memcpy(tmpl + tmpl_size, buf, len);
        tmpl_size += len;
    }
    tmpl[tmpl_size] = 0;
    printf("Template loaded. Please provide the values for the up to 3 variables:\n");
    char values[3][1024];
    for (int i = 1; i <= 3; ++i) {
        printf("Variable %d: ", i);
        // BUG: immediate stack-based overrun, very critical
        if (!gets(values[i-1]))
            exit(0);
    }
    for (int i = 0; i < tmpl_size; ++i) {
        if (tmpl[i] == '[') tmpl[i] = '%';
        if (tmpl[i] == '!') tmpl[i] = '$';
        if (tmpl[i] == ']') tmpl[i] = 's';
    }
    printf("Here's your final document, ready for copy & paste:\n");
    // BUG: format string
    printf(tmpl, values[0], values[1], values[2]);
}

void handle_pre_login() {
    for(;;) {
        printf("------------------------------\n");
        printf("How can we help you today?\n");
        int num_commands = sizeof pre_login_commands / sizeof *pre_login_commands;
        for (int i = 0; i < num_commands; ++i) {
            printf("%d) %s\n", i, pre_login_commands[i].name);
        }
        int action;
        for(;;) {
            printf("What do you want to do? Type the command number: ");
            action = read_int();
            if (action != -1 && action < num_commands)
                break;
            else
                printf("No such command, try again!\n");
        }
        printf("------------------------------\n");
        // BUG: out-of-bounds read+jmp. can be used for auth bypass by jumping into
        // post_login_commands after setting the username buffer via failed
        // login and then by typing in a negative number (not -1 though, it's
        // represents invalid input)
        pre_login_commands[action].func();
    }
}

void handle_post_login() {
    for(;;) {
        printf("------------------------------\n");
        printf("How can we help you today?\n");
        int num_commands = sizeof post_login_commands / sizeof *post_login_commands;
        for (int i = 0; i < num_commands; ++i) {
            printf("%d) %s\n", i, post_login_commands[i].name);
        }
        int action;
        for(;;) {
            printf("What do you want to do? Type the command number: ");
            action = read_int();
            if (action != -1 && action < num_commands)
                break;
            else
                printf("No such command, try again!\n");
        }
        printf("------------------------------\n");
        // BUG: out-of-bounds read+jmp
        post_login_commands[action].func();
        // special case: logout
        if (action == 0)
            return;
    }
}

void handle_client() {
    printf(welcome);
    handle_pre_login();
}

void disable_buffering() {
    setvbuf(stdin, 0, _IONBF, 0);
    setvbuf(stdout, 0, _IONBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);
}

void drop_privs() {
    chdir(sys_dir);
    struct passwd *pw = getpwnam(sys_username);
    if (!pw) {
        fprintf(stderr, "ERROR User %s does not exist\n", sys_username);
        exit(EXIT_FAILURE);
    }
    if (setgid(pw->pw_gid) ||
            setegid(pw->pw_gid) ||
	    setuid(pw->pw_uid) ||
            seteuid(pw->pw_uid)) {
        perror("ERROR setting user and group ID");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s homedir username\n", argv[0]);
        return EXIT_FAILURE;
    }
    sys_dir = argv[1];
    sys_username = argv[2];
    /*fprintf(stderr, "Directory: %s\nUsername: %s\n", sys_dir, sys_username);*/
    drop_privs();
    disable_buffering();
    handle_client();
}
