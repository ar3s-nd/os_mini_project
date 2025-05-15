#include "../lib/server.h"
#include <time.h>

int add_entity(void *data, int type)
{
    pthread_mutex_t *file_mutex;
    const char *filename;
    size_t struct_size;
    char id[50];

    if (type == STUDENT)
    {
        file_mutex = &student_file_mutex;
        filename = STUDENT_FILE;
        struct_size = sizeof(struct Student);
        strcpy(id, ((struct Student *)data)->student_id);
    }
    else if (type == FACULTY)
    {
        file_mutex = &faculty_file_mutex;
        filename = FACULTY_FILE;
        struct_size = sizeof(struct Faculty);
        strcpy(id, ((struct Faculty *)data)->faculty_id);
    }
    else if (type == COURSE)
    {
        file_mutex = &course_file_mutex;
        filename = COURSE_FILE;
        struct_size = sizeof(struct Course);
        strcpy(id, ((struct Course *)data)->course_code);
    }
    else
    {
        return FAILED;
    }

    pthread_mutex_lock(file_mutex);

    int fd = open(filename, O_CREAT | O_RDWR, 0744);
    if (fd == -1)
    {
        LOGMSG("Failed to open file.");
        pthread_mutex_unlock(file_mutex);
        return FAILED;
    }

    int cnt = 0;
    read(fd, &cnt, sizeof(cnt));

    void *existing = malloc(struct_size);
    while (read(fd, existing, struct_size) > 0)
    {
        if ((type == STUDENT && strcmp(((struct Student *)existing)->student_id, id) == 0 && ((struct Student *)existing)->isEXISTS) ||
            (type == FACULTY && strcmp(((struct Faculty *)existing)->faculty_id, id) == 0 && ((struct Faculty *)existing)->isEXISTS) ||
            (type == COURSE && strcmp(((struct Course *)existing)->course_code, id) == 0 && ((struct Course *)existing)->isEXISTS))
        {
            free(existing);
            close(fd);
            pthread_mutex_unlock(file_mutex);
            return type == STUDENT ? STUDENT_ALREADY_ADDED : type == FACULTY ? FACULTY_ALREADY_ADDED
                                                         : type == COURSE    ? COURSE_ALREADY_ADDED
                                                                             : FAILED;
        }
    }
    free(existing);

    // Set isEXISTS = 1
    if (type == STUDENT)
        ((struct Student *)data)->isEXISTS = 1;
    else if (type == FACULTY)
        ((struct Faculty *)data)->isEXISTS = 1;
    else if (type == COURSE)
        ((struct Course *)data)->isEXISTS = 1;

    lseek(fd, 0, SEEK_END);
    ssize_t bytes_written = write(fd, data, struct_size);
    if (bytes_written == -1)
    {
        LOGMSG("Failed to write to file");
        close(fd);
        pthread_mutex_unlock(file_mutex);
        return FAILED;
    }

    cnt++;
    lseek(fd, 0, SEEK_SET);
    write(fd, &cnt, sizeof(cnt));

    close(fd);
    pthread_mutex_unlock(file_mutex);
    return SUCCESS;
}

void *view_entity(int *count_out, int type)
{
    const char *filename;
    pthread_mutex_t *file_mutex;
    size_t struct_size;

    if (type == STUDENT)
    {
        filename = STUDENT_FILE;
        file_mutex = &student_file_mutex;
        struct_size = sizeof(struct Student);
    }
    else if (type == FACULTY)
    {
        filename = FACULTY_FILE;
        file_mutex = &faculty_file_mutex;
        struct_size = sizeof(struct Faculty);
    }
    else if (type == COURSE)
    {
        filename = COURSE_FILE;
        file_mutex = &course_file_mutex;
        struct_size = sizeof(struct Course);
    }
    else
    {
        return NULL;
    }

    pthread_mutex_lock(file_mutex);

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        LOGMSG("Failed to open file");
        pthread_mutex_unlock(file_mutex);
        return NULL;
    }

    int total_count = 0;
    read(fd, &total_count, sizeof(total_count));
    if (total_count == 0)
    {
        close(fd);
        pthread_mutex_unlock(file_mutex);
        return NULL;
    }

    void *list = malloc(struct_size * total_count);
    void *temp = malloc(struct_size);
    int actual_count = 0;

    while (read(fd, temp, struct_size) > 0)
    {
        int exists = 0;

        if (type == STUDENT)
        {
            struct Student *stu = (struct Student *)temp;
            exists = stu->isEXISTS;
        }
        else if (type == FACULTY)
        {
            struct Faculty *fac = (struct Faculty *)temp;
            exists = fac->isEXISTS;
        }
        else if (type == COURSE)
        {
            struct Course *course = (struct Course *)temp;
            exists = course->isEXISTS;
        }

        if (exists == 1)
        {
            memcpy((char *)list + (actual_count * struct_size), temp, struct_size);
            actual_count++;
        }
    }

    *count_out = total_count;

    free(temp);
    close(fd);
    pthread_mutex_unlock(file_mutex);

    if (actual_count == 0)
    {
        free(list);
        return NULL;
    }

    return list;
}

void *view_courses(char *entity_id, int *count_out, int type)
{
    const char *filename;
    pthread_mutex_t *file_mutex;
    size_t struct_size;

    filename = COURSE_FILE;
    file_mutex = &course_file_mutex;
    struct_size = sizeof(struct Course);

    pthread_mutex_lock(file_mutex);

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        LOGMSG("Failed to open file");
        pthread_mutex_unlock(file_mutex);
        return NULL;
    }

    int total_count = 0;
    read(fd, &total_count, sizeof(total_count));
    if (total_count == 0)
    {
        close(fd);
        pthread_mutex_unlock(file_mutex);
        return NULL;
    }

    void *list = malloc(struct_size * total_count);
    void *temp = malloc(struct_size);
    int actual_count = 0;

    while (read(fd, temp, struct_size) > 0)
    {
        int exists = 0;
        struct Course *course = (struct Course *)temp;
        if (type == FACULTY && !strcmp(course->faculty_id, entity_id))
        {
            exists = course->isEXISTS;
        }
        else if (type == STUDENT)
        {
            for (int i = 0; i < course->student_count; i++)
            {
                if (!strcmp(course->studentlist[i], entity_id))
                {
                    exists = course->isEXISTS;
                    break;
                }
            }
        }

        if (exists == 1)
        {
            memcpy((char *)list + (actual_count * struct_size), temp, struct_size);
            actual_count++;
        }
    }

    *count_out = actual_count;

    free(temp);
    close(fd);
    pthread_mutex_unlock(file_mutex);

    if (actual_count == 0)
    {
        free(list);
        return NULL;
    }

    return list;
}

int change_student_activeness(struct Student *student, int isActive)
{
    pthread_mutex_lock(&student_file_mutex);

    int fd = open(STUDENT_FILE, O_RDWR);
    if (fd == -1)
    {
        LOGMSG("Failed to open file");
        pthread_mutex_unlock(&student_file_mutex);
        return FAILED;
    }

    int student_count = 0;
    read(fd, &student_count, sizeof(student_count));

    struct Student temp;
    int found = 0;

    while (read(fd, &temp, sizeof(struct Student)) > 0)
    {
        if (strcmp(temp.student_id, student->student_id) == 0)
        {
            lseek(fd, -sizeof(struct Student), SEEK_CUR); // Move back to the position of the found student
            temp.isActive = isActive;
            write(fd, &temp, sizeof(struct Student));
            found = 1;
            break;
        }
    }

    close(fd);
    pthread_mutex_unlock(&student_file_mutex);

    return found ? SUCCESS : NOT_FOUND;
}

int modify_entity(void *data, int type)
{
    pthread_mutex_t *file_mutex;
    const char *filename;
    size_t struct_size;
    char id[50];

    if (type == STUDENT)
    {
        file_mutex = &student_file_mutex;
        filename = STUDENT_FILE;
        struct_size = sizeof(struct Student);
        strcpy(id, ((struct Student *)data)->student_id);
    }
    else if (type == FACULTY)
    {
        file_mutex = &faculty_file_mutex;
        filename = FACULTY_FILE;
        struct_size = sizeof(struct Faculty);
        strcpy(id, ((struct Faculty *)data)->faculty_id);
    }
    else if (type == COURSE)
    {
        file_mutex = &course_file_mutex;
        filename = COURSE_FILE;
        struct_size = sizeof(struct Course);
        strcpy(id, ((struct Course *)data)->course_code);
    }
    else
    {
        return FAILED;
    }

    pthread_mutex_lock(file_mutex);
    int fd = open(filename, O_RDWR);
    if (fd == -1)
    {
        LOGMSG("Failed to open file");
        pthread_mutex_unlock(file_mutex);
        return FAILED;
    }

    int cnt = 0;
    read(fd, &cnt, sizeof(cnt));

    void *existing = malloc(struct_size);
    while (read(fd, existing, struct_size) > 0)
    {
        if ((type == STUDENT && strcmp(((struct Student *)existing)->student_id, id) == 0) ||
            (type == FACULTY && strcmp(((struct Faculty *)existing)->faculty_id, id) == 0) ||
            (type == COURSE && strcmp(((struct Course *)existing)->course_code, id) == 0))
        {
            lseek(fd, -struct_size, SEEK_CUR); // Move back to the position of the found entity
            if (type == COURSE)
            {
                struct Course old = *((struct Course *)existing);
                struct Course new = *((struct Course *)data);
                strcpy(old.course_code, new.course_code);
                strcpy(old.course_name, new.course_name);
                old.student_limit = new.student_limit;
                old.student_count = new.student_count;
                old.isEXISTS = new.isEXISTS;
                write(fd, &old, struct_size); // Write the modified data
            }
            else
                write(fd, data, struct_size); // Write the modified data

            free(existing);
            close(fd);
            pthread_mutex_unlock(file_mutex);
            return SUCCESS;
        }
    }

    free(existing);
    close(fd);
    pthread_mutex_unlock(file_mutex);
    return NOT_FOUND;
}

int remove_course(struct Command command)
{
    pthread_mutex_lock(&course_file_mutex);
    int fd = open(COURSE_FILE, O_RDWR);
    if (fd == -1)
    {
        pthread_mutex_unlock(&course_file_mutex);
        return FAILED;
    }

    int flag = 0;
    read(fd, &flag, sizeof(int));
    flag = 0;
    struct Course course;
    while (read(fd, &course, sizeof(struct Course)))
    {
        if (strcmp(course.course_code, command.course.course_code) == 0)
        {
            if (course.student_count != 0)
            {
                close(fd);
                pthread_mutex_unlock(&course_file_mutex);
                return INVALID_OPERATION;
            }

            if (strcmp(course.faculty_id, command.faculty.faculty_id) == 0)
            {
                flag = 1;
            }
        }
    }
    pthread_mutex_unlock(&course_file_mutex);
    close(fd);

    if (!flag)
        return NOT_FOUND;

    int status = modify_entity(&command.course, COURSE);
    if (status != SUCCESS)
    {
        command.course.isEXISTS = 1;
        while (modify_entity(&command.course, COURSE) != SUCCESS)
            ;
        return status;
    }

    return modify_entity(&command.faculty, FACULTY);
}

int enroll_course(struct Command command)
{
    pthread_mutex_lock(&course_file_mutex);

    int fd = open(COURSE_FILE, O_RDWR);
    if (fd == -1)
    {
        LOGMSG("Failed to open file");
        pthread_mutex_unlock(&course_file_mutex);
        return FAILED;
    }

    int cnt = 0;
    read(fd, &cnt, sizeof(cnt));

    struct Course temp;
    int found = 0;

    while (read(fd, &temp, sizeof(struct Course)) > 0)
    {
        if (strcmp(temp.course_code, command.course.course_code) == 0 && temp.isEXISTS)
        {
            if (temp.student_count >= temp.student_limit)
            {
                pthread_mutex_unlock(&course_file_mutex);
                return LIMIT_EXCEEDED;
            }
            lseek(fd, -sizeof(struct Course), SEEK_CUR); // Move back to the position of the found course
            temp.student_count++;
            strcpy(temp.studentlist[temp.student_count - 1], command.student.student_id);
            write(fd, &temp, sizeof(struct Course));
            found = 1;
            break;
        }
    }

    close(fd);
    pthread_mutex_unlock(&course_file_mutex);

    if (!found)
        return NOT_FOUND;

    return modify_entity(&command.student, STUDENT);
}

int drop_course(struct Command command)
{
    pthread_mutex_lock(&course_file_mutex);

    int fd = open(COURSE_FILE, O_RDWR);
    if (fd == -1)
    {
        LOGMSG("Failed to open file");
        pthread_mutex_unlock(&course_file_mutex);
        return FAILED;
    }

    int cnt = 0;
    read(fd, &cnt, sizeof(cnt));

    struct Course temp;
    int found = 0;

    while (read(fd, &temp, sizeof(struct Course)) > 0)
    {
        if (strcmp(temp.course_code, command.course.course_code) == 0 && temp.isEXISTS)
        {
            lseek(fd, -sizeof(struct Course), SEEK_CUR); // Move back to the position of the found course

            for (int i = 0; i < temp.student_count; i++)
            {
                if (strcmp(command.student.student_id, temp.studentlist[i]) == 0)
                {
                    for (; i < temp.student_count - 1; i++)
                    {
                        strcpy(temp.studentlist[i], temp.studentlist[i + 1]);
                    }
                    break;
                }
            }
            temp.student_count--;

            write(fd, &temp, sizeof(struct Course));
            found = 1;
            break;
        }
    }

    close(fd);
    pthread_mutex_unlock(&course_file_mutex);

    if (!found)
        return NOT_FOUND;

    return modify_entity(&command.student, STUDENT);
}
