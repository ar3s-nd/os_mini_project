#ifndef SERVER_H

#include "client.h"

#define LOG(format, ...) printf(format, ##__VA_ARGS__)
#define LOGMSG(format, ...)                                             \
    do                                                                  \
    {                                                                   \
        time_t now = time(NULL);                                        \
        struct tm *t = localtime(&now);                                 \
        char timestamp[20];                                             \
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t); \
        LOG("\n-------------------------------------------\n");         \
        LOG("%s\n", timestamp);                                         \
        LOG(format, ##__VA_ARGS__);                                     \
        LOG("\n-------------------------------------------\n");         \
    } while (0)

// ADMIN operations
int add_entity(void *entity, int type);
void *view_entity(int *count_out, int type);
int modify_entity(void *data, int type);
int change_student_activeness(struct Student *student, int isActive);
void *view_courses(char *faculty_id, int *count_out, int type);
int remove_course(struct Command command);
int enroll_course(struct Command command);
int drop_course(struct Command command);

#endif