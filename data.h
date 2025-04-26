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

// Error Codes
#define SUCCESS 0
#define FAILED 1
#define NOT_LOGGED_IN 2
#define ALREADY_LOGGED_IN 3
#define USER_NOT_FOUND 4
#define USER_ALREADY_EXISTS 5
#define INVALID_CREDENTIALS 6
#define STUDENT_ALREADY_ADDED 7
#define OVERFLOW 8

// Status Codes
#define ISACTIVE 0
#define ISBLOCKED 1

// User Types
#define NO_USER -1
#define ADMIN 0
#define FACULTY 1
#define STUDENT 2
#define COURSE 3

// File Names
#define STUDENT_FILE "student.txt"
#define FACULTY_FILE "faculty.txt"
#define COURSE_FILE "course.txt"
#define ADMIN_FILE "admin.txt"

// IP and Port
#define IP "127.0.0.0"
#define PORT 8080

// Command Codes
#define LOGIN 1
#define LOGOUT 2
#define ADD_FACULTY 3
#define ADD_STUDENT 4
#define ADD_COURSE 5
#define VIEW_ADMIN 6
#define VIEW_FACULTY 7
#define VIEW_STUDENT 8
#define VIEW_COURSE 9
#define REMOVE_ADMIN 10
#define REMOVE_FACULTY 11
#define REMOVE_STUDENT 12
#define REMOVE_COURSE 13
#define ENROLL_COURSE 14
#define DROP_COURSE 15
#define VIEW_ENROLLED_COURSES 16
#define VIEW_ENROLLED_STUDENTS 17

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
    int isEXISTS;
};

struct Student
{
    char name[50];
    char password[50];
    char student_id[50];
    char course[50];
    int isEXISTS, isActive;
    int course_count;
    int course_list[MAX_SIZE]; // Changed to pointer for dynamic allocation
};

struct Course
{
    char course_name[50];
    char course_code[50];
    char faculty_id[50];
    int student_limit, student_count;
    int isEXISTS;
    int studentlist[MAX_SIZE]; // Changed to pointer for dynamic allocation
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

// ADMIN operations
int add_entity(void *entity, int type);
void *view_entity(int *count_out, int type);

#endif // DATA_H
