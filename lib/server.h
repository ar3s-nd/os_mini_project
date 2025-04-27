#ifndef SERVER_H

#include "client.h"

// ADMIN operations
int add_entity(void *entity, int type);
void *view_entity(int *count_out, int type);
int modify_entity(void *data, int type);
int change_student_activeness(struct Student *student, int isActive);
int change_password(void *data, int type);
int remove_course(struct Course *course);

#endif