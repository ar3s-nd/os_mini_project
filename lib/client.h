#ifndef DATA_H
#define DATA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <netinet/in.h>

#define MAX_SIZE 100
#define PRINT(format, ...) printf(format, ##__VA_ARGS__)

// Error Codes
#define SUCCESS 0
#define FAILED 1
#define NOT_LOGGED_IN 2
#define ALREADY_LOGGED_IN 3
#define NOT_FOUND 4
#define USER_ALREADY_EXISTS 5
#define INVALID_CREDENTIALS 6
#define STUDENT_ALREADY_ADDED 7
#define FACULTY_ALREADY_ADDED 8
#define COURSE_ALREADY_ADDED 9
#define INVALID_COMMAND 10
#define LIMIT_EXCEEDED 11
#define INVALID_OPERATION 12

// Status Codes
#define ISBLOCKED 20
#define ISACTIVE 21

// User Types
#define NO_USER 29
#define ADMIN 30
#define FACULTY 31
#define STUDENT 32
#define COURSE 33

// File Names
#define STUDENT_FILE "db/student.txt"
#define FACULTY_FILE "db/faculty.txt"
#define COURSE_FILE "db/course.txt"
#define ADMIN_FILE "db/admin.txt"
#define LOG_FILE "db/log.txt"

// IP and Port
#define IP "127.0.0.0"
#define PORT 8080

// Command Codes
#define LOGIN 101
#define LOGOUT 102
#define ADD_FACULTY 103
#define ADD_STUDENT 104
#define ADD_COURSE 105
#define VIEW_ADMIN 106
#define VIEW_FACULTY 107
#define VIEW_STUDENT 108
#define VIEW_COURSE 109
#define ACTIVATE_STUDENT 110
#define BLOCK_STUDENT 111
#define MODIFY_STUDENT 112
#define MODIFY_FACULTY 113
#define MODIFY_COURSE 114
#define REMOVE_COURSE 115
#define CHANGE_PASSWORD 116
#define ENROLL_COURSE 117
#define DROP_COURSE 118

extern pthread_mutex_t admin_file_mutex;
extern pthread_mutex_t student_file_mutex;
extern pthread_mutex_t course_file_mutex;
extern pthread_mutex_t faculty_file_mutex;

// Forward declaration of structs for Course and Student
struct Student;
struct Course;

// Struct Definitions
struct Admin
{
    char name[50];
    char password[50];
};

struct Faculty
{
    char name[50];
    char password[50];
    char faculty_id[50];
    char department[50];
    char courses[MAX_SIZE][50];
    int course_count;
    int isEXISTS;
};

struct Student
{
    char name[50];
    char password[50];
    char student_id[50];
    int isEXISTS, isActive;
    int course_count;
    char course_list[MAX_SIZE][50]; // Changed to pointer for dynamic allocation
};

struct Course
{
    char course_name[50];
    char course_code[50];
    char faculty_id[50];
    int student_limit, student_count;
    int isEXISTS;
    char studentlist[MAX_SIZE][50]; // Array of MAX_SIZE where each element is a string of size 50
};

// Command Structure
struct Command
{
    int cmd_code;
    int whereData; // to identify where the data is stored
    int role;      // 0 for admin, 1 for faculty, 2 for student
    struct Admin admin;
    struct Faculty faculty;
    struct Student student;
    struct Course course;
};

#endif // DATA_H
