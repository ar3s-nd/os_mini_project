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

#define SUCCESS 0
#define FAILED 1
#define NOT_LOGGED_IN 2
#define ALREADY_LOGGED_IN 3
#define USER_NOT_FOUND 4
#define USER_ALREADY_EXISTS 5
#define INVALID_CREDENTIALS 6

#define NO_USER -1
#define ADMIN 0
#define FACULTY 1
#define STUDENT 2
#define COURSE 3

#define STUDENT_FILE "student.txt"
#define FACULTY_FILE "faculty.txt"
#define COURSE_FILE "course.txt"
#define ADMIN_FILE "admin.txt"

#define PORT 8080

// command codes
#define LOGIN 1

struct Command
{
    int cmd_code;
    int type; // to identify where the data is stored
    int role; // 0 for admin, 1 for faculty, 2 for student. to know the type of user
    struct Admin admin;
    struct Faculty faculty;
    struct Student student;
    struct Course course;
};

struct Admin // type 0
{
    char name[50];
    char password[50];
    int offset;
};

struct Faculty // type 1
{
    char name[50];
    char password[50];
    char faculty_id[50];
    char department[50];
    int offset;
};

struct Student // type 2
{
    char name[50];
    char password[50];
    char student_id[50];
    char course[50];
    struct Course course_list[10];
    int offset;
};

struct Course // type 3
{
    char course_name[50];
    char course_code[50];
    char faculty_id[50];
    int student_limit, student_count;
    struct Student student_list[100];
    int offset;
};
