#include "../lib/client.h"

/*
    this is the client side code which says different things based on roles of the user and uses socket programming to communicate with the server
*/

struct sockaddr_in serv_addr;
int sockfd;

void *user;

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
    printf("Received response: %d\n", response);
    return response;
}

void addEntity(int role, int type)
{
    struct Command command;
    command.cmd_code = (type == STUDENT) ? ADD_STUDENT : (type == FACULTY) ? ADD_FACULTY
                                                     : (type == COURSE)    ? ADD_COURSE
                                                                           : 0;
    command.role = role;
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
        command.student.isActive = isActive + ISBLOCKED;

        command.student.isEXISTS = 1;
        command.student.course_count = 0;
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

        command.faculty.isEXISTS = 1;
        command.faculty.course_count = 0;
    }
    else if (type == COURSE)
    {
        printf("Enter course name: ");
        scanf("%s", command.course.course_name);

        printf("Enter course code: ");
        scanf("%s", command.course.course_code);

        printf("Enter student limit: ");
        scanf("%d", &command.course.student_limit);

        strcpy(((struct Faculty *)user)->courses[((struct Faculty *)user)->course_count], command.course.course_code);
        ((struct Faculty *)user)->course_count++;

        command.faculty = *((struct Faculty *)user);
    }

    int response = sendToServer(command);
    if (response == SUCCESS)
        printf("Entity added successfully.\n");
    else if (response == STUDENT_ALREADY_ADDED)
        printf("Entity already exists. You should activate it.\n");
    else
        printf("Failed to add entity.\n");
}

void viewEntityDetails(int role, int type)
{
    struct Command command;
    command.cmd_code = (type == STUDENT) ? VIEW_STUDENT : (type == FACULTY) ? VIEW_FACULTY
                                                      : (type == COURSE)    ? VIEW_COURSE
                                                                            : 0;
    command.role = role;
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

        void *entities_list;
        if (type == STUDENT)
            entities_list = malloc(sizeof(struct Student) * entity_count);
        else if (type == FACULTY)
            entities_list = malloc(sizeof(struct Faculty) * entity_count);
        else if (type == COURSE)
            entities_list = malloc(sizeof(struct Course) * entity_count);

        if (recv(sockfd, entities_list, sizeof(struct Student) * entity_count, 0) < 0)
        {
            perror("Receive failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printf("Received entity details:\n");
        for (int i = 0; i < entity_count; i++)
        {
            if (type == STUDENT)
            {
                struct Student *student = (struct Student *)entities_list + i;
                printf("Name: %s, ID: %s, Active: %d, Course Count: %d\n", student->name, student->student_id, student->isActive - ISBLOCKED, student->course_count);
                if (student->course_count > 0)
                    printf("Courses: ");
                for (int j = 0; j < student->course_count; j++)
                {
                    printf("%s ", student->course_list[j]);
                }
            }
            else if (type == FACULTY)
            {
                struct Faculty *faculty = (struct Faculty *)entities_list + i;
                printf("Name: %s, ID: %s, Department: %s, Course Count: %d\n", faculty->name, faculty->faculty_id, faculty->department, faculty->course_count);
                if (faculty->course_count > 0)
                    printf("Courses: ");
                for (int j = 0; j < faculty->course_count; j++)
                {
                    printf("%s ", faculty->courses[j]);
                }
            }
            else if (type == COURSE)
            {
                struct Course *course = (struct Course *)entities_list + i;
                printf("Name: %s, Code: %s\n", course->course_name, course->course_code);
            }
        }
        free(entities_list);
    }
    else
        printf("Failed to retrieve entity details.\n");
}

void changeStudentActiveness(int isActive)
{
    struct Command command;
    command.role = ADMIN;
    command.whereData = STUDENT;

    printf("Enter student ID: ");
    scanf("%s", command.student.student_id);

    command.student.isActive = isActive;
    command.cmd_code = (isActive == ISACTIVE) ? ACTIVATE_STUDENT : BLOCK_STUDENT;

    int response = sendToServer(command);
    if (response == SUCCESS)
        isActive ? printf("Student activeness changed successfully to ACTIVE\n") : printf("Student activeness changed successfully to BLOCKED\n");
    else
        printf("Failed to change student activeness.\n");
}

void modifyEntity(int role, int type)
{
    struct Command command;
    command.cmd_code = (type == STUDENT) ? MODIFY_STUDENT : (type == FACULTY) ? MODIFY_FACULTY
                                                        : (type == COURSE)    ? MODIFY_COURSE
                                                                              : 0;
    command.role = role;
    command.whereData = type;
    ;

    if (type == STUDENT)
    {
        printf("Enter student ID: ");
        scanf("%s", command.student.student_id);

        printf("Enter new student name: ");
        scanf("%s", command.student.name);

        printf("Enter new student password: ");
        scanf("%s", command.student.password);

        strcpy(((struct Student *)user)->name, command.student.name);
        strcpy(((struct Student *)user)->password, command.student.password);

        command.student = *((struct Student *)user);
    }
    else if (type == FACULTY)
    {
        printf("Enter faculty ID: ");
        scanf("%s", command.faculty.faculty_id);
        printf("Enter new faculty name: ");
        scanf("%s", command.faculty.name);
        printf("Enter new faculty password: ");
        scanf("%s", command.faculty.password);
        printf("Enter new department: ");
        scanf("%s", command.faculty.department);
        command.faculty.isEXISTS = 1;

        strcpy(((struct Faculty *)user)->name, command.faculty.name);
        strcpy(((struct Faculty *)user)->password, command.faculty.password);
        strcpy(((struct Faculty *)user)->department, command.faculty.department);
        command.faculty = *((struct Faculty *)user);
    }
    else if (type == COURSE)
    {
        printf("Enter course code: ");
        scanf("%s", command.course.course_code);

        printf("Enter new course name: ");
        scanf("%s", command.course.course_name);

        printf("Enter new student limit: ");
        scanf("%d", &command.course.student_limit);
    }

    int response = sendToServer(command);
    if (response == SUCCESS)
        printf("Entity modified successfully.\n");
    else
        printf("Failed to modify entity.\n");
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
    close(sockfd);
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
        addEntity(ADMIN, STUDENT);
        break;
    case 2:
        viewEntityDetails(ADMIN, STUDENT);
        break;
    case 3:
        addEntity(ADMIN, FACULTY);
        break;
    case 4:
        viewEntityDetails(ADMIN, FACULTY);
        break;
    case 5:
        changeStudentActiveness(ISACTIVE);
        break;
    case 6:
        changeStudentActiveness(ISBLOCKED);
        break;
    case 7:
        modifyEntity(ADMIN, STUDENT);
        break;
    case 8:
        modifyEntity(ADMIN, FACULTY);
        break;
    case 9:
        logout();
        break;
    default:
        printf("Invalid choice. Please try again.\n\n");
        break;
    }
}

void removeCourse()
{
    struct Command command;
    command.cmd_code = REMOVE_COURSE;
    command.role = FACULTY;
    command.whereData = COURSE;

    printf("Enter course code: ");
    scanf("%s", command.course.course_code);

    for (int i = 0; i < ((struct Faculty *)user)->course_count; i++)
    {
        if (strcmp(((struct Faculty *)user)->courses[i], command.course.course_code) == 0)
        {
            ((struct Faculty *)user)->course_count--;
            break;
        }
    }

    command.faculty = *((struct Faculty *)user);

    int response = sendToServer(command);
    if (response == SUCCESS)
        printf("Course removed successfully.\n");
    else
        printf("Failed to remove course.\n");
}

void changePassword(int role, int type)
{
    struct Command command;
    command.cmd_code = CHANGE_PASSWORD;
    command.role = role;
    command.whereData = type;

    char pwd[50];
    printf("Enter new password: ");
    scanf("%s", pwd);

    if (type == STUDENT)
    {
        strcpy(command.student.password, pwd);
        command.student = *((struct Student *)user);
    }
    else if (type == FACULTY)
    {
        strcpy(command.faculty.password, pwd);
        command.faculty = *((struct Faculty *)user);
    }

    int response = sendToServer(command);
    if (response == SUCCESS)
        printf("Password changed successfully.\n");
    else
        printf("Failed to change password.\n");
}

void showFacultyCommands()
{
    printf("Faculty Commands:\n");
    printf("1. Add Course\n");
    printf("2. Remove Course\n");
    printf("3. View Course\n");
    printf("4. Modify Course\n");
    printf("5. Change Password\n");
    printf("6. Logout\n");
    printf("Enter choice: ");

    int choice;
    scanf("%d", &choice);
    switch (choice)
    {
    case 1:
        addEntity(FACULTY, COURSE);
        break;
    case 2:
        // Remove course logic
        break;
    case 3:
        viewEntityDetails(FACULTY, COURSE);
        break;
    case 4:
        modifyEntity(FACULTY, COURSE);
        break;
    case 5:
        // Change password logic
        break;

    default:
        break;
    }
}

void showStudentCommands()
{
    printf("Student Commands:\n");
    printf("1. View Course\n");
    printf("2. Enroll in Course\n");
    printf("3. Drop Course\n");
    printf("4. Change Password\n");
    printf("5. Logout\n");
    printf("Enter choice: ");
    int choice;
    scanf("%d", &choice);
    printf("%d", choice);
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
        role += ADMIN;
        if (role < ADMIN || role > STUDENT)
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
        if (role == ADMIN)
        {
            strcpy(command.admin.name, username);
            strcpy(command.admin.password, password);
        }
        else if (role == FACULTY)
        {
            strcpy(command.faculty.name, username);
            strcpy(command.faculty.password, password);
        }
        else if (role == STUDENT)
        {
            strcpy(command.student.name, username);
            strcpy(command.student.password, password);
        }

        int response = sendToServer(command);
        switch (response)
        {
        case SUCCESS:
            isLoggedInAs = role;
            int strcut_size = 0;
            if (role == ADMIN)
            {
                user = malloc(sizeof(struct Admin));
                strcut_size = sizeof(struct Admin);
            }
            else if (role == FACULTY)
            {
                user = malloc(sizeof(struct Faculty));
                strcut_size = sizeof(struct Faculty);
            }
            else
            {
                user = malloc(sizeof(struct Student));
                strcut_size = sizeof(struct Student);
            }

            if (recv(sockfd, user, strcut_size, 0) < 0)
            {
                perror("Receive failed");
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            printf("Login successful as %s %s\n", ((struct Student *)user)->name, ((struct Admin *)user)->password);
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
            printf("User not found.\n");
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
        case ADMIN:
            showAdminCommands();
            break;
        case FACULTY:
            showFacultyCommands();
            break;
        case STUDENT:
            showStudentCommands();
            break;
        }
    }
    // Close socket
    close(sockfd);
    return 0;
}