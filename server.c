/*
    server side code
    this is to accept requests from the multiple clients using concurrent programming and socket programming and then process the requests, store data in files using locking,
    basically to implement a simple file server
*/

#include "data.h";

// access codes:
// -1 - not logged in
// 0 - admin
// 1 - faculty
// 2 - student

// int tryLogin(struct Command *command)
// {
//     char filepath[20];
//     void *command_data = NULL, *data = NULL;

//     int data_size = 0;
//     switch (command->role)
//     {
//     case ADMIN:
//         strcpy(filepath, ADMIN_FILE);
//         command_data =
//             command_data = &command->admin;
//         data = malloc(sizeof(command->admin));
//         data_size = sizeof(command->admin);
//         if (data == NULL)
//         {
//             perror("malloc");
//             return FAILED;
//         }
//         break;
//     case STUDENT:
//         strcpy(filepath, STUDENT_FILE);
//         command_data = &command->student;
//         data_size = sizeof(command->student);
//         data = malloc(sizeof(command->student));
//         if (data == NULL)
//         {
//             perror("malloc");
//             return FAILED;
//         }
//         break;
//     case FACULTY:
//         strcpy(filepath, FACULTY_FILE);
//         command_data = &command->faculty;
//         data_size = sizeof(command->faculty);
//         data = malloc(sizeof(command->faculty));
//         if (data == NULL)
//         {
//             perror("malloc");
//             return FAILED;
//         }
//         break;
//     default:
//         return FAILED;
//     }

//     int fd = open(filepath, O_RDWR);

//     if (fd == -1)
//     {
//         perror("open");
//         return -1;
//     }
//     struct flock lock;
//     lock.l_type = F_RDLCK;                // set the lock type to write
//     lock.l_whence = SEEK_SET;             // set the starting point for the lock
//     lock.l_start = 0;                     // set the starting point for the lock
//     lock.l_len = 0;                       // set the length of the lock to 0 (lock the whole file)
//     if (fcntl(fd, F_SETLKW, &lock) == -1) // set the lock
//     {
//         perror("fcntl");
//         close(fd);
//         return FAILED;
//     }

//     // process login request here
//     // read the file and check if the user exists
//     while (read(fd, data, data_size) > 0)
//     {
//         if (command->role == ADMIN)
//         {
//             struct Admin *admin_data = (struct Admin *)data;
//             struct Admin *admin_command_data = (struct Admin *)command_data;
//             if (!strcmp(admin_data->name, admin_command_data->name) && !strcmp(admin_data->password, admin_command_data->password))
//             {
//                 // match found
//                 lock.l_type = F_UNLCK;     // unlock the file
//                 fcntl(fd, F_SETLK, &lock); // set the lock
//                 close(fd);
//                 free(data);
//                 return SUCCESS;
//             }
//         }
//         else if (command->role == STUDENT)
//         {
//             struct Student *student_data = (struct Student *)data;
//             struct Student *student_command_data = (struct Student *)command_data;
//             if (!strcmp(student_data->name, student_command_data->name) && !strcmp(student_data->password, student_command_data->password))
//             {
//                 // match found
//                 lock.l_type = F_UNLCK;     // unlock the file
//                 fcntl(fd, F_SETLK, &lock); // set the lock
//                 close(fd);
//                 free(data);
//                 return SUCCESS;
//             }
//         }
//         else if (command->role == FACULTY)
//         {
//             struct Faculty *faculty_data = (struct Faculty *)data;
//             struct Faculty *faculty_command_data = (struct Faculty *)command_data;
//             if (!strcmp(faculty_data->name, faculty_command_data->name) && !strcmp(faculty_data->password, faculty_command_data->password))
//             {
//                 // match found
//                 lock.l_type = F_UNLCK;     // unlock the file
//                 fcntl(fd, F_SETLK, &lock); // set the lock
//                 close(fd);
//                 free(data);
//                 return SUCCESS;
//             }
//         }
//     }

//     free(data);
//     lock.l_type = F_UNLCK;     // unlock the file
//     fcntl(fd, F_SETLK, &lock); // set the lock
//     close(fd);
//     return FAILED;
// }

static pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

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

    pthread_mutex_lock(&file_mutex); // Lock before accessing the file

    int fd = open(filepath, O_RDWR);
    if (fd == -1)
    {
        pthread_mutex_unlock(&file_mutex); // Unlock if open fails
        return -1;
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
                    close(fd);
                    free(data);
                    pthread_mutex_unlock(&file_mutex); // Unlock after processing
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
                    close(fd);
                    free(data);
                    pthread_mutex_unlock(&file_mutex); // Unlock after processing
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
                    close(fd);
                    free(data);
                    pthread_mutex_unlock(&file_mutex); // Unlock after processing
                    return SUCCESS;
                }
                return INVALID_CREDENTIALS;
            }
        }
    }

    free(data);
    close(fd);
    pthread_mutex_unlock(&file_mutex); // Unlock after finishing the file access

    return USER_NOT_FOUND;
}

void handle_client(void *arg)
{
    int role = NO_USER;
    int sockfd = *(int *)arg;
    free(arg);

    struct Command command;
    int bytes_read = read(sockfd, &command, sizeof(command));
    if (bytes_read <= 0)
    {
        close(sockfd);
        return;
    }

    while (1)
    {
        if (role == NO_USER)
        {
            if (command.cmd_code == LOGIN)
            {
                // process login request
                int status = tryLogin(&command);
                if (status == SUCCESS)
                {
                    role = command.type;
                }
                write(sockfd, status, sizeof(status));
            }
            else
            {
                write(sockfd, NOT_LOGGED_IN, sizeof(NOT_LOGGED_IN));
            }
        }

        if (command.cmd_code == LOGIN)
        {
            return ALREADY_LOGGED_IN;
        }

        struct Command command;
        int bytes_read = read(sockfd, &command, sizeof(command));
        if (bytes_read <= 0)
        {
            close(sockfd);
            return;
        }
    }

    close(sockfd);
}

int main()
{
    struct sockaddr_in serv_addr;
    int addrlen = sizeof(serv_addr);
    int sockfd, *new_sockfd;
    void *data = NULL;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
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