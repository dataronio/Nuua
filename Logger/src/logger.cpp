/**
 * |-------------------|
 * | Nuua Error Logger |
 * |-------------------|
 *
 * Copyright 2019 Erik Campobadal <soc@erik.cat>
 * https://nuua.io
 */
#include "../include/logger.hpp"
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <cstdlib>

Logger *logger = new Logger;

static int red_printf(const char *format, ...)
{
    va_list arg;
    int result;
    va_start(arg, format);
    #if defined(_WIN32) || defined(_WIN64)
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        WORD saved_attributes;
        GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
        saved_attributes = consoleInfo.wAttributes;
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
        result = vfprintf(stderr, format, arg);
        SetConsoleTextAttribute(hConsole, saved_attributes);
    #else
        char *fmt = (char *) malloc(sizeof(char) * (strlen(format) + 10)); // \x1b[31m\x1b[0m = (9 + '\0')
        strcpy(fmt, "\x1b[31m");
        strcat(fmt, format);
        strcat(fmt, "\x1b[0m");
        result = vfprintf(stderr, fmt, arg);
        free(fmt);
    #endif
    va_end(arg);
    return result;
}

static void print_file_line(const char *file, const line_t line, const column_t column)
{
    FILE *source_file = fopen(file, "r");
    if (source_file == NULL) {
        printf("%*c\n%*c<unknown>\n", 3, ' ', 3, ' ');
        exit(EXIT_FAILURE);
    }
    char buffer[MAX_LINE_LENGTH];
    for (line_t current_line = 1; current_line <= line; current_line++) {
        error_read_again:
        char *ok = fgets(buffer, sizeof(buffer), source_file);
        if (ok == NULL || buffer == NULL || *buffer == EOF) {
            printf("\n%*c<unknown>\n", 3, ' ');
            exit(EXIT_FAILURE);
        }
        if (strlen(buffer) == MAX_LINE_LENGTH && buffer[strlen(buffer) - 1] != '\n') {
            goto error_read_again;
        }
    }
    fclose(source_file);
    // Trim the initial spaces / tabs.
    uint16_t offset = 0;
    while (buffer[offset] == '\t' || buffer[offset] == ' ') offset++;
    printf("\n%*c%s%*c", 3, ' ', buffer + offset, 3, ' ');
    for (column_t i = 1; i < column - offset; i++) printf(" ");
    fflush(stdout);
    printf("^\n");
}

static void print_msg(const std::string &msg, bool red)
{
    printf("%*c", 3, ' ');
    size_t current_char = 0, current_col = 0;
    while (current_char < msg.length()) {
        if (current_col > MAX_LINE_CHARS && msg[current_char] == ' ') {
            printf("\n%*c", 3, ' ');
            ++current_char;
            current_col = 0;
        }
        fflush(stdout);
        if (red) red_printf("%c", msg[current_char++]);
        else printf("%c", msg[current_char++]);
        ++current_col;
    }
    printf("\n");
}

void Logger::add_entity(const std::shared_ptr<const std::string> &file, const line_t line, const column_t column, const std::string &msg)
{
    this->entities.push_back({ file, line, column, msg });
}

void Logger::pop_entity()
{
    this->entities.pop_back();
}

int Logger::crash() const
{
    if (this->entities.size() == 0) {
        // Show a generic error since no entities exists.
        red_printf("There was an error\n");
        return EXIT_FAILURE;
    }
    // Display the error stack using the entities.
    // printf(" ----------------------\n");
    // red_printf(" > THERE WAS AN ERROR <\n");
    // printf(" ----------------------\n");
    for (size_t i = 0; i < this->entities.size() - 1; i++) {
        display_log(i, false);
    }
    display_log(this->entities.size() - 1, true);

    return EXIT_FAILURE;
}

void Logger::display_log(const uint16_t index, const bool red) const
{
    if (this->entities[index].file) {
        printf(
            "\n > %s, line: %u, column: %u\n\n",
            this->entities[index].file->c_str(),
            this->entities[index].line,
            this->entities[index].column
        );
    } else printf("\n");
    print_msg(this->entities[index].msg, red);
    if (this->entities[index].line != 0 && this->entities[index].column != 0) {
        print_file_line(
            this->entities[index].file->c_str(),
            this->entities[index].line,
            this->entities[index].column
        );
        printf("\n");
    }
}
