#include "lib/client.h"

/*
    this is the client side code which says different things based on roles of the user and uses socket programming to communicate with the server
*/

struct sockaddr_in serv_addr;
int sockfd;

int sendToServer(struct Command command)
{
    // Send command to server
    printf("Sending command to server...\n");
    if (send(sockfd, &command, sizeof(command), 0) < 0)
    {
        perror("Send failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Receive response from server
    int response;
    if (recv(sockfd, &response, sizeof(response), 0) < 0)
    {
        perror("Receive failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return response;
}

void addEntity(int type)
{
    struct Command command;
    command.cmd_code = (type == STUDENT) ? ADD_STUDENT : (type == FACULTY) ? ADD_FACULTY
                                                     : (type == COURSE)    ? ADD_COURSE
                                                                           : 0;
    command.role = ADMIN;
    command.whereData = type;

    if (type == STUDENT)
    {
        printf("Enter student name: ");
        scanf("%s", command.student.name);

        printf("Enter student password: ");
        scanf("%s", command.student.password);

        printf("Enter student ID: ");
        scanf("%s", command.student.student_id);

        int isActive;
        printf("Is student active? (1 for yes, 0 for no): ");
        scanf("%d", &isActive);
        command.student.isActive = isActive;
    }
    else if (type == FACULTY)
    {
        printf("Enter faculty name: ");
        scanf("%s", command.faculty.name);

        printf("Enter faculty password: ");
        scanf("%s", command.faculty.password);

        printf("Enter faculty ID: ");
        scanf("%s", command.faculty.faculty_id);

        printf("Enter department: ");
        scanf("%s", command.faculty.department);
    }
    else if (type == COURSE)
    {
        printf("Enter course name: ");
        scanf("%s", command.course.course_name);

        printf("Enter course code: ");
        scanf("%s", command.course.course_code);

        printf("Enter faculty ID: ");
        scanf("%s", command.course.faculty_id);

        printf("Enter student limit: ");
        scanf("%d", &command.course.student_limit);
    }

    int response = sendToServer(command);
    if (response == SUCCESS)
        printf("Entity added successfully.\n");
    else if (response == STUDENT_ALREADY_ADDED)
        printf("Entity already exists. You should activate it.\n");
    else if (response == OVERFLOW)
        printf("Entity limit exceeded.\n");
    else
        printf("Failed to add entity.\n");
}

void viewEntityDetails(int type)
{
    struct Command command;
    command.cmd_code = (type == STUDENT) ? VIEW_STUDENT : (type == FACULTY) ? VIEW_FACULTY
                                                      : (type == COURSE)    ? VIEW_COURSE
                                                                            : 0;
    command.role = ADMIN;
    command.whereData = type;

    int response = sendToServer(command);
    if (response == SUCCESS)
    {
        int entity_count = 0;
        if (recv(sockfd, &entity_count, sizeof(entity_count), 0) < 0)
        {
            perror("Receive failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        printf("Number of entities: %d\n", entity_count);
        if (entity_count == 0)
        {
            printf("No entities found.\n");
            return;
        }

        void *entities_list = malloc(sizeof(struct Student) * entity_count);
        if (type == FACULTY)
            entities_list = malloc(sizeof(struct Faculty) * entity_count);
        else if (type == COURSE)
            entities_list = malloc(sizeof(struct Course) * entity_count);

        if (recv(sockfd, entities_list, sizeof(struct Student) * entity_count, 0) < 0)
        {
            perror("Receive failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printf("Entity List:\n");
        for (int i = 0; i < entity_count; i++)
        {
            if (type == STUDENT)
            {
                struct Student *student = (struct Student *)entities_list + i;
                if (student->isEXISTS == 1)
                    printf("Name: %s, ID: %s\n", student->name, student->student_id);
            }
            else if (type == FACULTY)
            {
                struct Faculty *faculty = (struct Faculty *)entities_list + i;
                if (faculty->isEXISTS == 1)
                    printf("Name: %s, ID: %s\n", faculty->name, faculty->faculty_id);
            }
            else if (type == COURSE)
            {
                struct Course *course = (struct Course *)entities_list + i;
                if (course->isEXISTS == 1)
                    printf("Name: %s, Code: %s\n", course->course_name, course->course_code);
            }
        }
        free(entities_list);
    }
    else
        printf("Failed to retrieve entity details.\n");
}

void logout()
{
    struct Command command;
    command.cmd_code = LOGOUT;
    command.role = NO_USER;
    command.whereData = NO_USER;

    int response = sendToServer(command);
    if (response == SUCCESS)
        printf("Logged out successfully.\n");
    else
        printf("Failed to log out.\n");
    exit(SUCCESS);
}

void showAdminCommands()
{
    printf("Admin Commands:\n");
    printf("1. Add Student\n");
    printf("2. View Student Details\n");
    printf("3. Add Faculty\n");
    printf("4. View Faculty Details\n");
    printf("5. Activate Student\n");
    printf("6. Block Student\n");
    printf("7. Modify Student Details\n");
    printf("8. Modify Faculty Details\n");
    printf("9. Logout and Exit\n");
    printf("Enter choice: ");
    int choice;
    scanf("%d", &choice);
    switch (choice)
    {
    case 1:
        addEntity(STUDENT);
        break;
    case 2:
        viewEntityDetails(STUDENT);
        break;
    case 3:
        addEntity(FACULTY);
        break;
    case 4:
        viewEntityDetails(FACULTY);
        break;
    default:
        // printf("Invalid choice. Please try again.\n\n");
        logout();
        break;
    }
}

void showFacultyCommands()
{
    printf("Faculty Commands:\n");
    printf("1. Add Course\n");
    printf("2. Remove Course\n");
    printf("3. View Course\n");
}

void showStudentCommands()
{
    printf("Student Commands:\n");
    printf("1. View Course\n");
    printf("2. Enroll in Course\n");
    printf("3. Drop Course\n");
}

void handleSignals(int sig)
{
    printf("\nExiting...\n");
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
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP);
    serv_addr.sin_port = htons(PORT);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int isLoggedInAs = NO_USER;
    while (isLoggedInAs == NO_USER)
    {
        printf("Enter your role (0 for Admin, 1 for Faculty, 2 for Student): ");
        int role;
        scanf("%d", &role);

        if (role < 0 || role > 2)
        {
            printf("Invalid role. Please try again.\n");
            continue;
        }
        printf("Enter your username: ");
        char username[50];
        scanf("%s", username);
        printf("Enter your password: ");
        char password[50];
        scanf("%s", password);

        struct Command command;
        command.cmd_code = LOGIN;
        command.role = role;
        command.whereData = role;
        strcpy(command.admin.password, password);
        strcpy(command.admin.name, username);

        int response = sendToServer(command);
        switch (response)
        {
        case SUCCESS:
            isLoggedInAs = role;
            printf("Login successful as %s\n", username);
            break;
        case FAILED:
            printf("Login failed. Please try again.\n");
            break;
        case NOT_LOGGED_IN:
            printf("You are not logged in.\n");
            break;
        case INVALID_CREDENTIALS:
            printf("Invalid credentials. Please try again.\n");
            break;
        case USER_NOT_FOUND:
            printf("User not found. Please check your username and password.\n");
            break;
        default:
            printf("Unknown error occurred: %d\n", response);
            break;
        }
    }

    while (1)
    {
        switch (isLoggedInAs)
        {
        case 0:
            showAdminCommands();
            break;
        case 1:
            showFacultyCommands();
            break;
        case 2:
            showStudentCommands();
            break;
        }
    }
    // Close socket
    close(sockfd);
    return 0;
}