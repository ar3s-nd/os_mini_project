/*
    server side code
    this is to accept requests from the multiple clients using concurrent programming and socket programming and then process the requests, store data in files using locking,
    basically to implement a simple file server
*/

#include "data.h"

int sockfd;
struct sockaddr_in serv_addr;

// Function to process login
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
    case STUDENT: // STUDENT
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

    // Process login request here
    while (read(fd, data, data_size) > 0)
    {
        if (command->role == ADMIN)
        {
            struct Admin *admin_data = (struct Admin *)data;
            struct Admin *admin_command_data = (struct Admin *)command_data;
            printf("Admin data: %s\n", admin_data->name);
            printf("Command data: %s\n", admin_command_data->name);
            if (!strcmp(admin_data->name, admin_command_data->name))
            {
                if (!strcmp(admin_data->password, admin_command_data->password))
                {
                    // Match found
                    printf("Admin login successful: %s\n", admin_data->name);
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
            if (student_data->student_id == student_command_data->student_id)
            {
                if (!strcmp(student_data->password, student_command_data->password))
                {
                    // Match found
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
            if (faculty_data->faculty_id == faculty_command_data->faculty_id)
            {
                if (!strcmp(faculty_data->password, faculty_command_data->password))
                {
                    // Match found
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
        printf("New admin added: %s\n", command->admin.name);
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
        printf("Received command: %d\n", command.cmd_code);
        // Process the command based on the role

        if (role == NO_USER)
        {
            if (command.cmd_code == LOGIN)
            {
                int status = tryLogin(&command);
                if (status == SUCCESS)
                {
                    role = command.role;
                }
                write(sockfd, &status, sizeof(status));
            }
            else
            {
                int status = NOT_LOGGED_IN;
                write(sockfd, &status, sizeof(NOT_LOGGED_IN));
            }
        }
        else
        {
            printf("User role: %d\n", role);
            if (command.cmd_code == LOGOUT)
            {
                if (role == NO_USER)
                {
                    int status = NOT_LOGGED_IN;
                    write(sockfd, &status, sizeof(NOT_LOGGED_IN));
                }
                role = NO_USER;
                printf("User logged out\n");
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
                else if (command.cmd_code == ADD_COURSE)
                {
                    int status = add_entity(&command.course, COURSE);
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
    return NULL;
}

void handleCtrlC(int sig)
{
    printf("\nExiting...\n");
    close(sockfd);
    exit(0);
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
    printf("Admin file created successfully\n");
    close(fd);

    fd = open(STUDENT_FILE, O_CREAT | O_RDWR | O_TRUNC, 0744);
    if (fd == -1)
    {
        perror("Failed to create student file");
        pthread_mutex_unlock(&student_file_mutex);
        exit(EXIT_FAILURE);
    }
    write(fd, &cnt, sizeof(cnt));
    printf("Student file created successfully\n");
    close(fd);

    fd = open(FACULTY_FILE, O_CREAT | O_RDWR | O_TRUNC, 0744);
    if (fd == -1)
    {
        perror("Failed to create faculty file");
        pthread_mutex_unlock(&faculty_file_mutex);
        exit(EXIT_FAILURE);
    }
    write(fd, &cnt, sizeof(cnt));
    printf("Faculty file created successfully\n");
    close(fd);

    fd = open(COURSE_FILE, O_CREAT | O_RDWR | O_TRUNC, 0744);
    if (fd == -1)
    {
        perror("Failed to create course file");
        pthread_mutex_unlock(&course_file_mutex);
        exit(EXIT_FAILURE);
    }
    write(fd, &cnt, sizeof(cnt));
    printf("Course file created successfully\n");
    close(fd);

    printf("Files initialized successfully\n");
}

int main()
{
    signal(SIGINT, handleCtrlC);
    printf("Server started\n");

    printf("Initialise files? (y/n): ");
    char choice;
    scanf(" %c", &choice);
    if (choice == 'y' || choice == 'Y')
    {
        printf("Initializing files...\n");
        initialiseFiles();
    }
    else
    {
        printf("Skipping file initialization...\n");
    }

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
        // Detach the thread to allow it to clean up after itself
        pthread_detach(tid);
    }
}