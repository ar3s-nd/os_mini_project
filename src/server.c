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

    // Set command_data and allocate memory based on role
    switch (command->role)
    {
    case ADMIN:
        strcpy(filepath, ADMIN_FILE);
        command_data = &command->admin;
        data = malloc(sizeof(command->admin));
        data_size = sizeof(command->admin);
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
        if (data == NULL)
        {
            return FAILED;
        }
        break;
    default:
        return FAILED;
    }

    // print admin data

    pthread_mutex_lock(&admin_file_mutex); // Lock before accessing the file

    int fd = open(filepath, O_CREAT | O_RDWR, 0744);
    if (fd == -1)
    {
        perror(filepath);
        pthread_mutex_unlock(&admin_file_mutex);
        return -1;
    }

    if (command->role == STUDENT || command->role == FACULTY)
    {
        int cnt = 0;
        read(fd, &cnt, sizeof(cnt));
        if (cnt == 0)
        {
            close(fd);
            pthread_mutex_unlock(&admin_file_mutex);
            return USER_NOT_FOUND;
        }
    }

    // Process login request here
    while (read(fd, data, data_size) > 0)
    {
        if (command->role == ADMIN)
        {
            struct Admin *admin_data = (struct Admin *)data;
            struct Admin *admin_command_data = (struct Admin *)command_data;
            LOG("Admin data: %s\n", admin_data->name);
            LOG("Command data: %s\n", admin_command_data->name);
            if (!strcmp(admin_data->name, admin_command_data->name))
            {
                if (!strcmp(admin_data->password, admin_command_data->password))
                {
                    // Match found
                    LOG("Admin login successful: %s\n", admin_data->name);
                    strcpy(command->admin.name, admin_data->name);
                    strcpy(command->admin.password, admin_data->password);
                    close(fd);
                    free(data);
                    pthread_mutex_unlock(&admin_file_mutex); // Unlock after processing
                    return SUCCESS;
                }
                return INVALID_CREDENTIALS;
            }
        }
        else if (command->role == STUDENT)
        {
            struct Student *student_data = (struct Student *)data;
            struct Student *student_command_data = (struct Student *)command_data;
            if (!strcmp(student_data->name, student_command_data->name))
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
                    pthread_mutex_unlock(&admin_file_mutex); // Unlock after processing
                    return SUCCESS;
                }
                return INVALID_CREDENTIALS;
            }
        }
        else if (command->role == FACULTY)
        {
            struct Faculty *faculty_data = (struct Faculty *)data;
            struct Faculty *faculty_command_data = (struct Faculty *)command_data;
            if (!strcmp(faculty_data->name, faculty_command_data->name))
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
                    pthread_mutex_unlock(&admin_file_mutex); // Unlock after processing
                    return SUCCESS;
                }
                return INVALID_CREDENTIALS;
            }
        }
    }

    // If no match found, add the new admin
    if (command->role == ADMIN)
    {
        close(fd);
        fd = open(filepath, O_RDWR | O_APPEND);
        lseek(fd, 0, SEEK_END);
        write(fd, command_data, sizeof(command->admin));
        close(fd);
        free(data);
        LOG("New admin added: %s\n", command->admin.name);
        pthread_mutex_unlock(&admin_file_mutex);
        return SUCCESS;
    }

    free(data);
    close(fd);
    // Unlock after finishing the file access
    return USER_NOT_FOUND;
}

void *handle_client(void *arg)
{
    int role = NO_USER;
    int sockfd = *(int *)arg;
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
        LOG("Received command: %d\n", command.cmd_code);
        // Process the command based on the role

        if (role == NO_USER)
        {
            if (command.cmd_code == LOGIN)
            {
                int status = tryLogin(&command);
                write(sockfd, &status, sizeof(status));
                if (status == SUCCESS)
                {
                    role = command.role;
                    if (role == ADMIN)
                    {
                        LOG("Admin logged in: %s\n", command.admin.name);
                        write(sockfd, &command.admin, sizeof(command.admin));
                    }
                    else if (role == STUDENT)
                    {
                        LOG("Student logged in: %s\n", command.student.name);
                        write(sockfd, &command.student, sizeof(command.student));
                    }
                    else if (role == FACULTY)
                    {
                        LOG("Faculty logged in: %s\n", command.faculty.name);
                        write(sockfd, &command.faculty, sizeof(command.faculty));
                    }
                }
            }
            else
            {
                int status = NOT_LOGGED_IN;
                write(sockfd, &status, sizeof(NOT_LOGGED_IN));
            }
        }
        else
        {
            LOG("User role: %d\n", role);
            if (command.cmd_code == LOGOUT)
            {
                if (role == NO_USER)
                {
                    int status = NOT_LOGGED_IN;
                    write(sockfd, &status, sizeof(NOT_LOGGED_IN));
                }
                role = NO_USER;
                LOG("User logged out\n");
                int status = SUCCESS;
                write(sockfd, &status, sizeof(status));
                break;
            }
            if (role == ADMIN)
            {
                if (command.cmd_code == ADD_FACULTY)
                {
                    int status = add_entity(&command.faculty, FACULTY);
                    write(sockfd, &status, sizeof(status));
                }
                else if (command.cmd_code == ADD_STUDENT)
                {
                    // int status = add_student(&command.student);
                    int status = add_entity(&command.student, STUDENT);
                    write(sockfd, &status, sizeof(status));
                }
                else if (command.cmd_code == VIEW_STUDENT)
                {
                    printf("View student command received\n");

                    int student_count;
                    struct Student *students = view_entity(&student_count, STUDENT);

                    if (students == NULL || student_count == 0)
                    {
                        printf("No students found %d\n", student_count);
                        int status = FAILED;
                        write(sockfd, &status, sizeof(status));
                    }
                    else
                    {
                        printf("Total active students: %d\n", student_count);
                        int status = SUCCESS;
                        write(sockfd, &status, sizeof(status));
                        write(sockfd, &student_count, sizeof(student_count));
                        write(sockfd, students, sizeof(struct Student) * student_count);

                        for (int i = 0; i < student_count; i++)
                        {
                            printf("Student: %s\n", students[i].name);
                        }

                        free(students);
                    }
                }
                else if (command.cmd_code == VIEW_FACULTY)
                {
                    printf("View faculty command received\n");

                    int faculty_count;
                    struct Student *faculties = view_entity(&faculty_count, FACULTY);

                    if (faculties == NULL || faculty_count == 0)
                    {
                        printf("No students found %d\n", faculty_count);
                        int status = FAILED;
                        write(sockfd, &status, sizeof(status));
                    }
                    else
                    {
                        printf("Total active students: %d\n", faculty_count);
                        int status = SUCCESS;
                        write(sockfd, &status, sizeof(status));
                        write(sockfd, &faculty_count, sizeof(faculty_count));
                        write(sockfd, faculties, sizeof(struct Student) * faculty_count);

                        for (int i = 0; i < faculty_count; i++)
                        {
                            printf("Student: %s\n", faculties[i].name);
                        }

                        free(faculties);
                    }
                }
                else if (command.cmd_code == ACTIVATE_STUDENT)
                {
                    LOG("Change student activeness command received\n");
                    int status = change_student_activeness(&command.student, ISACTIVE);
                    write(sockfd, &status, sizeof(status));
                }
                else if (command.cmd_code == BLOCK_STUDENT)
                {
                    LOG("Change student activeness command received\n");
                    int status = change_student_activeness(&command.student, ISBLOCKED);
                    write(sockfd, &status, sizeof(status));
                }
                else if (command.cmd_code == MODIFY_STUDENT)
                {
                    LOG("Modify student command received\n");
                    int status = modify_entity(&command.student, STUDENT);
                    write(sockfd, &status, sizeof(status));
                }
                else if (command.cmd_code == MODIFY_FACULTY)
                {
                    LOG("Modify faculty command received\n");
                    int status = modify_entity(&command.faculty, FACULTY);
                    write(sockfd, &status, sizeof(status));
                }
                else
                {
                    LOG("Invalid command for admin\n");
                    int status = INVALID_COMMAND;
                    write(sockfd, &status, sizeof(status));
                }
            }
            else if (role == FACULTY)
            {
                if (command.cmd_code == ADD_COURSE)
                {
                    int status = add_entity(&command.course, COURSE);
                    write(sockfd, &status, sizeof(status));
                }
                else if (command.cmd_code == VIEW_COURSE)
                {
                    LOG("View course command received\n");

                    int course_count;
                    struct Course *courses = view_entity(&course_count, COURSE);

                    if (courses == NULL || course_count == 0)
                    {
                        LOG("No courses found %d\n", course_count);
                        int status = FAILED;
                        write(sockfd, &status, sizeof(status));
                    }
                    else
                    {
                        LOG("Total active courses: %d\n", course_count);
                        int status = SUCCESS;
                        write(sockfd, &status, sizeof(status));
                        write(sockfd, &course_count, sizeof(course_count));
                        write(sockfd, courses, sizeof(struct Course) * course_count);

                        for (int i = 0; i < course_count; i++)
                        {
                            LOG("Course: %s\n", courses[i].course_name);
                        }

                        free(courses);
                    }
                }
                else if (command.cmd_code == MODIFY_COURSE)
                {
                    LOG("Modify course command received\n");
                    int status = modify_entity(&command.course, COURSE);
                    write(sockfd, &status, sizeof(status));
                }
                else if (command.cmd_code == REMOVE_COURSE)
                {
                    LOG("Remove course command received\n");
                    int status = remove_course(&command.course);
                    status &= modify_entity(&command.faculty, FACULTY);
                    write(sockfd, &status, sizeof(status));
                }
                else if (command.cmd_code == CHANGE_PASSWORD)
                {
                    LOG("Change password command received\n");
                    int status = change_password(&command.faculty, FACULTY);
                    write(sockfd, &status, sizeof(status));
                }
                else
                {
                    LOG("Invalid command for faculty\n");
                    int status = INVALID_COMMAND;
                    write(sockfd, &status, sizeof(status));
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

    LOG("Client disconnected\n");
    close(sockfd);
    return NULL;
}

void initialiseFiles()
{
    system("rm -rf admin.txt student.txt faculty.txt course.txt");

    // Create files if they don't exist
    int cnt = 0;
    int fd = open(ADMIN_FILE, O_CREAT | O_RDWR | O_TRUNC, 0744);
    if (fd == -1)
    {
        perror("Failed to create admin file");
        pthread_mutex_unlock(&admin_file_mutex);
        exit(EXIT_FAILURE);
    }
    LOG("Admin file created successfully\n");
    close(fd);

    fd = open(STUDENT_FILE, O_CREAT | O_RDWR | O_TRUNC, 0744);
    if (fd == -1)
    {
        perror("Failed to create student file");
        pthread_mutex_unlock(&student_file_mutex);
        exit(EXIT_FAILURE);
    }
    write(fd, &cnt, sizeof(cnt));
    LOG("Student file created successfully\n");
    close(fd);

    fd = open(FACULTY_FILE, O_CREAT | O_RDWR | O_TRUNC, 0744);
    if (fd == -1)
    {
        perror("Failed to create faculty file");
        pthread_mutex_unlock(&faculty_file_mutex);
        exit(EXIT_FAILURE);
    }
    write(fd, &cnt, sizeof(cnt));
    LOG("Faculty file created successfully\n");
    close(fd);

    fd = open(COURSE_FILE, O_CREAT | O_RDWR | O_TRUNC, 0744);
    if (fd == -1)
    {
        perror("Failed to create course file");
        pthread_mutex_unlock(&course_file_mutex);
        exit(EXIT_FAILURE);
    }
    write(fd, &cnt, sizeof(cnt));
    LOG("Course file created successfully\n");
    close(fd);

    LOG("Files initialized successfully\n");
}

void handleSignals(int sig)
{
    LOG("Exiting...\n");
    // system(">log.txt");
    close(sockfd);
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

    LOG("Server started\n");
    LOG("Initialise files? (y/n): ");
    char choice;
    scanf(" %c", &choice);
    if (choice == 'y' || choice == 'Y')
    {
        LOG("Initializing files...\n");
        initialiseFiles();
    }
    else
    {
        LOG("Skipping file initialization...\n");
    }

    int logfd = open(LOG_FILE, O_CREAT | O_RDWR | O_APPEND, 0744);
    if (logfd == -1)
    {
        perror("Failed to create log file");
        exit(EXIT_FAILURE);
    }
    dup2(logfd, STDOUT_FILENO);

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

    if (listen(sockfd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        new_sockfd = malloc(sizeof(int));
        *new_sockfd = accept(sockfd, (struct sockaddr *)&serv_addr, &addrlen); // accept a new connection from a new client
        if (*new_sockfd < 0)
        {
            free(new_sockfd);
            continue;
        }

        pthread_t tid;
        // create a new thread to handle that client
        pthread_create(&tid, NULL, handle_client, new_sockfd);

        if (tid == 0)
        {
            perror("Failed to create thread");
            close(*new_sockfd);
            free(new_sockfd);
            continue;
        }
        LOG("Thread created successfully: %ld\n", tid);
        // Detach the thread to allow it to clean up after itself
        pthread_detach(tid);
    }
}