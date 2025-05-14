/*
    server side code
    this is to accept requests from the multiple clients using concurrent programming and socket programming and then process the requests, store data in files using locking,
    basically to implement a simple file server
*/

#include "../lib/server.h"

int sockfd;
struct sockaddr_in serv_addr;

int tryLogin(struct Command *command)
{
    char filepath[20];
    void *command_data = NULL, *data = NULL;
    int data_size = 0;
    pthread_mutex_t *mutex = NULL;

    // Set command_data and allocate memory based on role
    switch (command->role)
    {
    case ADMIN:
        strcpy(filepath, ADMIN_FILE);
        command_data = &command->admin;
        data = malloc(sizeof(command->admin));
        data_size = sizeof(command->admin);
        mutex = &admin_file_mutex;
        if (data == NULL)
        {
            return FAILED;
        }
        break;
    case STUDENT:
        strcpy(filepath, STUDENT_FILE);
        command_data = &command->student;
        data_size = sizeof(command->student);
        data = malloc(sizeof(command->student));
        mutex = &student_file_mutex;
        if (data == NULL)
        {
            return FAILED;
        }
        break;
    case FACULTY: // FACULTY
        strcpy(filepath, FACULTY_FILE);
        command_data = &command->faculty;
        data_size = sizeof(command->faculty);
        data = malloc(sizeof(command->faculty));
        mutex = &faculty_file_mutex;
        if (data == NULL)
        {
            return FAILED;
        }
        break;
    default:
        return FAILED;
    }

    pthread_mutex_lock(mutex); // Lock before accessing the file

    int fd = open(filepath, O_CREAT | O_RDWR, 0744);
    if (fd == -1)
    {
        perror(filepath);
        pthread_mutex_unlock(mutex);
        return FAILED;
    }

    if (command->role == STUDENT || command->role == FACULTY)
    {
        int cnt = 0;
        read(fd, &cnt, sizeof(cnt));
        if (cnt == 0)
        {
            close(fd);
            pthread_mutex_unlock(mutex);
            return NOT_FOUND;
        }
    }

    // Process login request here
    while (read(fd, data, data_size) > 0)
    {
        if (command->role == ADMIN)
        {
            struct Admin *admin_data = (struct Admin *)data;
            struct Admin *admin_command_data = (struct Admin *)command_data;
            if (!strcmp(admin_data->name, admin_command_data->name))
            {
                if (!strcmp(admin_data->password, admin_command_data->password))
                {
                    // Match found
                    strcpy(command->admin.name, admin_data->name);
                    strcpy(command->admin.password, admin_data->password);
                    close(fd);
                    free(data);
                    pthread_mutex_unlock(mutex); // Unlock after processing
                    return SUCCESS;
                }
                pthread_mutex_unlock(mutex); // Unlock after processing
                return INVALID_CREDENTIALS;
            }
        }
        else if (command->role == STUDENT)
        {
            struct Student *student_data = (struct Student *)data;
            struct Student *student_command_data = (struct Student *)command_data;
            LOGMSG("%s %s\n", student_data->student_id, student_command_data->student_id);
            if (!strcmp(student_data->student_id, student_command_data->student_id))
            {
                if (!strcmp(student_data->password, student_command_data->password))
                {
                    // Match found
                    strcpy(command->student.name, student_data->name);
                    strcpy(command->student.password, student_data->password);
                    strcpy(command->student.student_id, student_data->student_id);
                    command->student.isActive = student_data->isActive;
                    command->student.isEXISTS = student_data->isEXISTS;
                    command->student.course_count = student_data->course_count;
                    for (int i = 0; i < student_data->course_count; i++)
                    {
                        strcpy(command->student.course_list[i], student_data->course_list[i]);
                    }
                    close(fd);
                    free(data);
                    pthread_mutex_unlock(mutex); // Unlock after processing
                    return SUCCESS;
                }
                pthread_mutex_unlock(mutex); // Unlock after processing
                return INVALID_CREDENTIALS;
            }
        }
        else if (command->role == FACULTY)
        {
            struct Faculty *faculty_data = (struct Faculty *)data;
            struct Faculty *faculty_command_data = (struct Faculty *)command_data;
            if (!strcmp(faculty_data->faculty_id, faculty_command_data->faculty_id))
            {
                if (!strcmp(faculty_data->password, faculty_command_data->password))
                {
                    // Match found
                    strcpy(command->faculty.name, faculty_data->name);
                    strcpy(command->faculty.password, faculty_data->password);
                    strcpy(command->faculty.faculty_id, faculty_data->faculty_id);
                    strcpy(command->faculty.department, faculty_data->department);
                    command->faculty.course_count = faculty_data->course_count;
                    command->faculty.isEXISTS = faculty_data->isEXISTS;
                    for (int i = 0; i < faculty_data->course_count; i++)
                    {
                        strcpy(command->faculty.courses[i], faculty_data->courses[i]);
                    }
                    close(fd);
                    free(data);
                    pthread_mutex_unlock(mutex); // Unlock after processing
                    return SUCCESS;
                }
                pthread_mutex_unlock(mutex); // Unlock after processing
                return INVALID_CREDENTIALS;
            }
        }
    }

    // If no match found, add the new admin
    if (command->role == ADMIN)
    {
        lseek(fd, 0, SEEK_END);
        write(fd, command_data, sizeof(command->admin));
        close(fd);
        free(data);
        if (pthread_mutex_unlock(mutex) != 0)
        {
            perror("Failed to unlock mutex");
            return FAILED;
        }
        return SUCCESS;
    }

    pthread_mutex_unlock(mutex);
    free(data);
    close(fd);
    return NOT_FOUND;
}

struct thread_args
{
    int sockfd, clientid;
};

void *handle_client(void *arg)
{
    int role = NO_USER;
    int sockfd = ((struct thread_args *)arg)->sockfd;
    int clientid = ((struct thread_args *)arg)->clientid;
    free(arg);

    struct Command command;
    int bytes_read = read(sockfd, &command, sizeof(command));
    if (bytes_read <= 0)
    {
        close(sockfd);
        return NULL;
    }

    while (1)
    {
        if (role == NO_USER)
        {
            if (command.cmd_code == LOGIN)
            {
                int status = tryLogin(&command);
                send(sockfd, &status, sizeof(status), 0);
                if (status == SUCCESS)
                {
                    role = command.role;
                    if (role == ADMIN)
                    {
                        send(sockfd, &command.admin, sizeof(command.admin), 0);
                    }
                    else if (role == STUDENT)
                    {
                        send(sockfd, &command.student, sizeof(command.student), 0);
                    }
                    else if (role == FACULTY)
                    {
                        send(sockfd, &command.faculty, sizeof(command.faculty), 0);
                    }
                }
            }
            else
            {
                int status = NOT_LOGGED_IN;
                send(sockfd, &status, sizeof(NOT_LOGGED_IN), 0);
            }
        }
        else
        {
            if (command.cmd_code == LOGOUT)
            {
                if (role == NO_USER)
                {
                    int status = NOT_LOGGED_IN;
                    send(sockfd, &status, sizeof(NOT_LOGGED_IN), 0);
                }
                role = NO_USER;
                int status = SUCCESS;
                send(sockfd, &status, sizeof(status), 0);
                break;
            }
            if (command.role == ADMIN)
            {
                if (command.cmd_code == ADD_FACULTY)
                {
                    int status = add_entity(&command.faculty, FACULTY);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else if (command.cmd_code == ADD_STUDENT)
                {
                    int status = add_entity(&command.student, STUDENT);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else if (command.cmd_code == VIEW_STUDENT)
                {
                    int student_count = 0;
                    struct Student *students = view_entity(&student_count, STUDENT);

                    if (student_count == 0)
                    {
                        int status = NOT_FOUND;
                        send(sockfd, &status, sizeof(status), 0);
                    }
                    else if (students == NULL)
                    {
                        int status = FAILED;
                        send(sockfd, &status, sizeof(status), 0);
                    }
                    else
                    {
                        int status = SUCCESS;
                        send(sockfd, &status, sizeof(status), 0);
                        send(sockfd, &student_count, sizeof(student_count), 0);
                        send(sockfd, students, sizeof(struct Student) * student_count, 0);
                        free(students);
                    }
                }
                else if (command.cmd_code == VIEW_FACULTY)
                {
                    int faculty_count = 0;
                    struct Faculty *faculties = view_entity(&faculty_count, FACULTY);

                    if (faculty_count == 0)
                    {
                        int status = NOT_FOUND;
                        send(sockfd, &status, sizeof(status), 0);
                    }
                    else if (faculties == NULL)
                    {
                        int status = FAILED;
                        send(sockfd, &status, sizeof(status), 0);
                    }
                    else
                    {
                        int status = SUCCESS;
                        send(sockfd, &status, sizeof(status), 0);
                        send(sockfd, &faculty_count, sizeof(faculty_count), 0);
                        send(sockfd, faculties, sizeof(struct Faculty) * faculty_count, 0);
                        free(faculties);
                    }
                }
                else if (command.cmd_code == ACTIVATE_STUDENT)
                {
                    int status = change_student_activeness(&command.student, ISACTIVE);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else if (command.cmd_code == BLOCK_STUDENT)
                {
                    int status = change_student_activeness(&command.student, ISBLOCKED);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else if (command.cmd_code == MODIFY_STUDENT)
                {
                    int status = modify_entity(&command.student, STUDENT);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else if (command.cmd_code == MODIFY_FACULTY)
                {
                    int status = modify_entity(&command.faculty, FACULTY);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else
                {
                    int status = INVALID_COMMAND;
                    send(sockfd, &status, sizeof(status), 0);
                }
            }
            else if (command.role == FACULTY)
            {
                if (command.cmd_code == ADD_COURSE)
                {
                    int status = add_entity(&command.course, COURSE);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else if (command.cmd_code == VIEW_COURSE)
                {
                    int course_count = 0;
                    struct Course *courses = view_courses(command.faculty.faculty_id, &course_count, FACULTY);
                    if (course_count == 0)
                    {
                        int status = NOT_FOUND;
                        send(sockfd, &status, sizeof(status), 0);
                    }
                    else if (courses == NULL)
                    {
                        int status = FAILED;
                        send(sockfd, &status, sizeof(int), 0);
                    }
                    else
                    {
                        int status = SUCCESS;
                        send(sockfd, &status, sizeof(status), 0);
                        send(sockfd, &course_count, sizeof(course_count), 0);
                        send(sockfd, courses, sizeof(struct Course) * course_count, 0);

                        for (int i = 0; i < course_count; i++)
                        {
                        }

                        free(courses);
                    }
                }
                else if (command.cmd_code == MODIFY_COURSE)
                {
                    int status = modify_entity(&command.course, COURSE);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else if (command.cmd_code == REMOVE_COURSE)
                {
                    int status = remove_course(command);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else if (command.cmd_code == CHANGE_PASSWORD)
                {
                    int status = modify_entity(&command.faculty, FACULTY);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else
                {
                    int status = INVALID_COMMAND;
                    send(sockfd, &status, sizeof(status), 0);
                }
            }
            else if (command.role == STUDENT)
            {
                if (command.cmd_code == VIEW_COURSE)
                {
                    int course_count = 0;
                    struct Course *courses = view_courses(command.student.student_id, &course_count, STUDENT);
                    if (course_count == 0)
                    {
                        int status = NOT_FOUND;
                        send(sockfd, &status, sizeof(status), 0);
                    }
                    else if (courses == NULL)
                    {
                        int status = FAILED;
                        send(sockfd, &status, sizeof(status), 0);
                    }
                    else
                    {
                        int status = SUCCESS;
                        send(sockfd, &status, sizeof(status), 0);
                        send(sockfd, &course_count, sizeof(course_count), 0);
                        send(sockfd, courses, sizeof(struct Course) * course_count, 0);

                        for (int i = 0; i < course_count; i++)
                        {
                        }

                        free(courses);
                    }
                }
                else if (command.cmd_code == ENROLL_COURSE)
                {
                    int status = enroll_course(command);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else if (command.cmd_code == DROP_COURSE)
                {
                    int status = drop_course(command);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else if (command.cmd_code = CHANGE_PASSWORD)
                {
                    int status = modify_entity(&command.student, STUDENT);
                    send(sockfd, &status, sizeof(status), 0);
                }
                else
                {
                    int status = INVALID_COMMAND;
                    send(sockfd, &status, sizeof(status), 0);
                }
            }
        }

        // Wait for the next command from the client
        bytes_read = read(sockfd, &command, sizeof(command));
        if (bytes_read <= 0)
        {
            // If no data is received or client disconnects, close the socket
            break;
        }
    }

    close(sockfd);
    LOGMSG("Client disconnected\nID: %d\nSocket: %d", clientid, sockfd);
    return NULL;
}

void initialiseFiles()
{
    LOG("Clearing all existing data...\n");
    // Create files if they don't exist
    int cnt = 0;
    int fd = open(ADMIN_FILE, O_CREAT | O_RDWR | O_TRUNC, 0744);
    if (fd == -1)
    {
        perror("Failed to create admin file");
        exit(EXIT_FAILURE);
    }
    close(fd);

    fd = open(STUDENT_FILE, O_CREAT | O_RDWR | O_TRUNC, 0744);
    if (fd == -1)
    {
        perror("Failed to create student file");
        exit(EXIT_FAILURE);
    }
    write(fd, &cnt, sizeof(cnt));
    close(fd);

    fd = open(FACULTY_FILE, O_CREAT | O_RDWR | O_TRUNC, 0744);
    if (fd == -1)
    {
        perror("Failed to create faculty file");
        exit(EXIT_FAILURE);
    }
    write(fd, &cnt, sizeof(cnt));
    close(fd);

    fd = open(COURSE_FILE, O_CREAT | O_RDWR | O_TRUNC, 0744);
    if (fd == -1)
    {
        perror("Failed to create course file");
        exit(EXIT_FAILURE);
    }
    write(fd, &cnt, sizeof(cnt));
    close(fd);

    fd = open(LOG_FILE, O_CREAT | O_RDWR | O_TRUNC, 0744);
    close(fd);
}

void handleSignals(int sig)
{
    close(sockfd);
    LOGMSG("Server is terminating...");
    exit(0);
}

int main()
{
    signal(SIGINT, handleSignals);  // Interrupt from keyboard
    signal(SIGTERM, handleSignals); // Termination signal
    signal(SIGQUIT, handleSignals); // Quit signal
    signal(SIGSEGV, handleSignals); // Segmentation fault
    signal(SIGABRT, handleSignals); // Abnormal termination
    signal(SIGILL, handleSignals);  // Illegal instruction
    signal(SIGFPE, handleSignals);  // Floating-point exception
    signal(SIGBUS, handleSignals);  // Bus error
    signal(SIGPIPE, SIG_IGN);       // Ignore broken pipe

    char choice;
    LOG("Do you want to clear all data? \n'y' for YES, 'n' for NO: ");
    scanf(" %c", &choice);
    if (choice == 'y' || choice == 'Y')
    {
        initialiseFiles();
    }

    int logfd = open(LOG_FILE, O_CREAT | O_RDWR | O_APPEND, 0744);
    if (logfd == -1)
    {
        perror("Failed to create log file");
        exit(EXIT_FAILURE);
    }
    dup2(logfd, STDOUT_FILENO);

    LOGMSG("Server is starting...\n");

    int addrlen = sizeof(serv_addr);
    int *new_sockfd;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP);
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&student_file_mutex, NULL);
    pthread_mutex_init(&admin_file_mutex, NULL);
    pthread_mutex_init(&course_file_mutex, NULL);
    pthread_mutex_init(&faculty_file_mutex, NULL);

    int id = 1;
    while (1)
    {
        struct thread_args *args = malloc(sizeof(struct thread_args));
        args->sockfd = accept(sockfd, (struct sockaddr *)&serv_addr, &addrlen); // accept a new connection from a new client
        if (args->sockfd < 0)
        {
            free(args);
            continue;
        }

        // create a new thread to handle that client
        args->clientid = id++;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, args);

        LOGMSG("New client connected.\nID: %d\nSocket: %d", args->clientid, args->sockfd);
        if (tid == 0)
        {
            perror("Failed to create thread");
            close(args->sockfd);
            free(args);
            continue;
        }

        // Detach the thread to allow it to clean up after itself
        pthread_detach(tid);
    }
}