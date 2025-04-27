#include "../lib/server.h"

pthread_mutex_t admin_file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t student_file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t course_file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t faculty_file_mutex = PTHREAD_MUTEX_INITIALIZER;