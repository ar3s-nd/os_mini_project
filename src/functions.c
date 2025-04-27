#include "../lib/server.h"

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
        perror("Failed to open file");
        pthread_mutex_unlock(file_mutex);
        return FAILED;
    }

    int cnt = 0;
    read(fd, &cnt, sizeof(cnt));

    void *existing = malloc(struct_size);
    while (read(fd, existing, struct_size) > 0)
    {
        if ((type == STUDENT && strcmp(((struct Student *)existing)->student_id, id) == 0 && ((struct Student *)data)->isEXISTS) ||
            (type == FACULTY && strcmp(((struct Faculty *)existing)->faculty_id, id) == 0 && ((struct Faculty *)data)->isEXISTS) ||
            (type == COURSE && strcmp(((struct Course *)existing)->course_code, id) == 0 && ((struct Course *)data)->isEXISTS))
        {
            free(existing);
            close(fd);
            pthread_mutex_unlock(file_mutex);
            return STUDENT_ALREADY_ADDED;
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
        perror("Failed to write to file");
        close(fd);
        pthread_mutex_unlock(file_mutex);
        return FAILED;
    }

    cnt++;
    lseek(fd, 0, SEEK_SET);
    write(fd, &cnt, sizeof(cnt));

    LOG("Entity added successfully.\n");

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

    printf("Entity List:\n");
    pthread_mutex_lock(file_mutex);

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("Failed to open file");
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
            printf("Name: %s, ID: %s, Exists: %d\n", stu->name, stu->student_id, stu->isEXISTS);
            exists = stu->isEXISTS;
        }
        else if (type == FACULTY)
        {
            struct Faculty *fac = (struct Faculty *)temp;
            printf("Name: %s, ID: %s, Exists: %d\n", fac->name, fac->faculty_id, fac->isEXISTS);
            exists = fac->isEXISTS;
        }
        else if (type == COURSE)
        {
            struct Course *course = (struct Course *)temp;
            printf("Name: %s, Code: %s, Exists: %d\n", course->course_name, course->course_code, course->isEXISTS);
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

int change_student_activeness(struct Student *student, int isActive)
{
    pthread_mutex_lock(&student_file_mutex);

    int fd = open(STUDENT_FILE, O_RDWR);
    if (fd == -1)
    {
        perror("Failed to open file");
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

    return found ? SUCCESS : USER_NOT_FOUND;
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
        perror("Failed to open file");
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
            LOG("Total count: %d\n", cnt);
            lseek(fd, -struct_size, SEEK_CUR); // Move back to the position of the found entity
            write(fd, data, struct_size);      // Write the modified data
            free(existing);
            close(fd);
            pthread_mutex_unlock(file_mutex);
            return SUCCESS;
        }
    }

    free(existing);
    close(fd);
    pthread_mutex_unlock(file_mutex);
    return USER_NOT_FOUND;
}

int remove_course(struct Course *course)
{
    pthread_mutex_lock(&course_file_mutex);

    int fd = open(COURSE_FILE, O_RDWR);
    if (fd == -1)
    {
        perror("Failed to open file");
        pthread_mutex_unlock(&course_file_mutex);
        return FAILED;
    }

    int cnt = 0;
    read(fd, &cnt, sizeof(cnt));

    struct Course temp;
    int found = 0;

    while (read(fd, &temp, sizeof(struct Course)) > 0)
    {
        if (strcmp(temp.course_code, course->course_code) == 0)
        {
            lseek(fd, -sizeof(struct Course), SEEK_CUR); // Move back to the position of the found course
            temp.isEXISTS = 0;
            write(fd, &temp, sizeof(struct Course));
            found = 1;
            break;
        }
    }

    close(fd);
    pthread_mutex_unlock(&course_file_mutex);

    return found ? SUCCESS : USER_NOT_FOUND;
}

int change_password(void *data, int type)
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
    else
    {
        return FAILED;
    }

    pthread_mutex_lock(file_mutex);

    int fd = open(filename, O_RDWR);
    if (fd == -1)
    {
        perror("Failed to open file");
        pthread_mutex_unlock(file_mutex);
        return FAILED;
    }

    int cnt = 0;
    read(fd, &cnt, sizeof(cnt));

    void *existing = malloc(struct_size);
    while (read(fd, existing, struct_size) > 0)
    {
        if ((type == STUDENT && strcmp(((struct Student *)existing)->student_id, id) == 0) ||
            (type == FACULTY && strcmp(((struct Faculty *)existing)->faculty_id, id) == 0))
        {
            lseek(fd, -struct_size, SEEK_CUR); // Move back to the position of the found entity
            write(fd, data, struct_size);      // Write the modified data
            free(existing);
            close(fd);
            pthread_mutex_unlock(file_mutex);
            return SUCCESS;
        }
    }

    free(existing);
    close(fd);
    pthread_mutex_unlock(file_mutex);
    return USER_NOT_FOUND;
}
