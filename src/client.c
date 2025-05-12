#include "../lib/client.h"

/*
    this is the client side code which says different things based on roles of the user and uses socket programming to communicate with the server
*/

struct sockaddr_in serv_addr;
int sockfd;

void *user;

int sendToServer(struct Command command)
{
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
        strcpy(command.course.faculty_id, ((struct Faculty *)user)->faculty_id);
        printf("Faculty ID: %s\n", ((struct Faculty *)user)->faculty_id);
    }

    int response = sendToServer(command);
    if (response == SUCCESS)
    {
        printf("Entity added successfully.\n");
        if (type == COURSE)
        {
            strcpy(((struct Faculty *)user)->courses[((struct Faculty *)user)->course_count], command.course.course_code);
            ((struct Faculty *)user)->course_count++;
            struct Command com;
            com.faculty = *((struct Faculty *)user);
            com.cmd_code = MODIFY_FACULTY;
            com.role = ADMIN;
            com.whereData = FACULTY;
            int res = sendToServer(com);
            if (res == SUCCESS)
                printf("Faculty modified successfully. %d\n", com.faculty.course_count);
            else
                printf("Failed to modify faculty.\n");
        }
    }
    else if (response == STUDENT_ALREADY_ADDED)
        printf("Student already exists.\n");
    else if (response == FACULTY_ALREADY_ADDED)
        printf("Entity already exists.\n");
    else if (response == COURSE_ALREADY_ADDED)
        printf("Course already exists.\n");
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
        int struct_size = 0;
        if (type == STUDENT)
        {
            entities_list = malloc(sizeof(struct Student) * entity_count);
            struct_size = sizeof(struct Student);
        }
        else if (type == FACULTY)
        {
            entities_list = malloc(sizeof(struct Faculty) * entity_count);
            struct_size = sizeof(struct Faculty);
        }
        else if (type == COURSE)
        {
            entities_list = malloc(sizeof(struct Course) * entity_count);
            struct_size = sizeof(struct Course);
        }

        if (recv(sockfd, entities_list, struct_size * entity_count, 0) < 0)
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
                if (student->course_count > 0)
                    printf("\n");
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
                if (faculty->course_count > 0)
                    printf("\n");
            }
            else if (type == COURSE)
            {
                struct Course *course = (struct Course *)entities_list + i;
                if (strcmp(course->faculty_id, ((struct Faculty *)user)->faculty_id))
                    continue;

                printf("Name: %s, Code: %s, Student Count: %d/%d, Faculty: %s\n", course->course_name, course->course_code, course->student_count, course->student_limit, course->faculty_id);
                if (course->student_count > 0)
                    printf("Students: ");
                for (int j = 0; j < course->student_count; j++)
                {
                    printf("%s ", course->studentlist[j]);
                }
                if (course->student_count > 0)
                    printf("\n");
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
        (isActive == ISACTIVE) ? printf("Student activeness changed successfully to ACTIVE\n") : printf("Student activeness changed successfully to BLOCKED\n");
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
        command.student.isEXISTS = 1;
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
    }
    else if (type == COURSE)
    {
        printf("Enter course code: ");
        scanf("%s", command.course.course_code);

        printf("Enter new course name: ");
        scanf("%s", command.course.course_name);

        printf("Enter new student limit: ");
        scanf("%d", &command.course.student_limit);
        command.course.isEXISTS = 1;
    }

    int response = sendToServer(command);
    if (response == SUCCESS)
    {
        printf("Entity modified successfully.\n");
        if (type == STUDENT)
        {
            strcpy(((struct Student *)user)->name, command.student.name);
            strcpy(((struct Student *)user)->password, command.student.password);
        }
        else if (type == FACULTY)
        {
            strcpy(((struct Faculty *)user)->name, command.faculty.name);
            strcpy(((struct Faculty *)user)->password, command.faculty.password);
            strcpy(((struct Faculty *)user)->department, command.faculty.department);
        }
    }
    else
        printf("Failed to modify entity. %d\n", response);
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
    printf("\n----------------------------------------------\n\n");

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

void viewCourses(int type)
{
    struct Command command;
    command.cmd_code = VIEW_COURSE;
    command.role = type;
    command.whereData = COURSE;
    command.faculty = *((struct Faculty *)user);
    command.student = *((struct Student *)user);

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
        int struct_size = 0;

        entities_list = malloc(sizeof(struct Course) * entity_count);
        struct_size = sizeof(struct Course);

        if (recv(sockfd, entities_list, struct_size * entity_count, 0) < 0)
        {
            perror("Receive failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printf("Received entity details:\n");
        for (int i = 0; i < entity_count; i++)
        {
            if (type == FACULTY)
            {
                struct Course *course = (struct Course *)entities_list + i;
                printf("Name: %s, Code: %s, Student Count: %d/%d, Faculty: %s\n", course->course_name, course->course_code, course->student_count, course->student_limit, course->faculty_id);
                if (course->student_count > 0)
                    printf("Students: ");
                for (int j = 0; j < course->student_count; j++)
                {
                    printf("%s ", course->studentlist[j]);
                }
                if (course->student_count > 0)
                    printf("\n");
            }

            else if (type == STUDENT)
            {
                struct Course *course = (struct Course *)entities_list + i;
                for (int j = 0; j < course->student_count; j++)
                {
                    if (strcmp(course->studentlist[j], ((struct Student *)user)->student_id))
                        continue;
                }
                printf("Name: %s, Code: %s, Faculty: %s\n", course->course_name, course->course_code, course->faculty_id);
            }
        }
        free(entities_list);
    }
    else if (response == NOT_FOUND)
        printf("No courses found.\n");
    else
        printf("Failed to retrieve entity details.\n");
}

void removeCourse()
{
    struct Command command;
    command.cmd_code = REMOVE_COURSE;
    command.role = FACULTY;
    command.whereData = COURSE;

    printf("Enter course code: ");
    scanf("%s", command.course.course_code);
    command.course.isEXISTS = 0;

    command.faculty = *((struct Faculty *)user);

    for (int i = 0; i < command.faculty.course_count; i++)
    {
        if (strcmp(command.faculty.courses[i], command.course.course_code) == 0)
        {
            while (i < command.faculty.course_count - 1)
            {
                strcpy(command.faculty.courses[i], command.faculty.courses[i + 1]);
                i++;
            }
            command.faculty.course_count--;
            break;
        }
    }

    int response = sendToServer(command);
    if (response == SUCCESS)
    {
        printf("Course removed successfully.\n");
        // copy the contents into user as well
        for (int i = 0; i < command.faculty.course_count; i++)
        {
            strcpy(((struct Faculty *)user)->courses[i], command.faculty.courses[i]);
        }
        ((struct Faculty *)user)->course_count--;
    }
    else if (response == INVALID_COMMAND)
        printf("All students must drop the course before it can be removed.\n");
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
        command.student = *((struct Student *)user);
        strcpy(command.student.password, pwd);
    }
    else if (type == FACULTY)
    {
        command.faculty = *((struct Faculty *)user);
        strcpy(command.faculty.password, pwd);
    }

    int response = sendToServer(command);
    if (response == SUCCESS)
    {
        printf("Password changed successfully.\n");
        if (type == STUDENT)
            strcpy(((struct Student *)user)->password, pwd);
        else if (type == FACULTY)
            strcpy(((struct Faculty *)user)->password, pwd);
    }
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
    printf("\n----------------------------------------------\n\n");
    switch (choice)
    {
    case 1:
        addEntity(FACULTY, COURSE);
        break;
    case 2:
        removeCourse();
        break;
    case 3:
        viewCourses(FACULTY);
        break;
    case 4:
        modifyEntity(FACULTY, COURSE);
        break;
    case 5:
        changePassword(FACULTY, FACULTY);
        break;
    case 6:
        logout();
        break;

    default:
        printf("Invalid choice. Please try again.\n\n");
        break;
    }
}

void enrollCourse()
{
    struct Command command;
    command.cmd_code = ENROLL_COURSE;
    command.role = STUDENT;
    command.whereData = STUDENT;

    printf("Enter course code: ");
    scanf("%s", command.course.course_code);
    command.student = *((struct Student *)user);
    for (int i = 0; i < command.student.course_count; i++)
    {
        if (strcmp(command.course.course_code, command.student.course_list[i]) == 0)
        {
            printf("Student already enrolled.\n");
            return;
        }
    }
    strcpy(command.student.course_list[command.student.course_count], command.course.course_code);
    command.student.course_count++;

    int response = sendToServer(command);
    if (response == SUCCESS)
    {
        printf("Student enrolled successfully.\n");
        strcpy(((struct Student *)user)->course_list[((struct Student *)user)->course_count], command.course.course_code);
        ((struct Student *)user)->course_count++;
    }
    else if (response == NOT_FOUND)
        printf("Course not found.\n");
    else if (response == LIMIT_EXCEEDED)
        printf("Student limit reached. Cannot enroll for this course.");
    else
        printf("Error occurred.\n");
}

void dropCourse()
{
    struct Command command;
    command.cmd_code = DROP_COURSE;
    command.role = STUDENT;
    command.whereData = STUDENT;

    printf("Enter course code: ");
    scanf("%s", command.course.course_code);
    command.student = *((struct Student *)user);
    int isstudentenrolled = 0;
    for (int i = 0; i < command.student.course_count; i++)
    {
        if (strcmp(command.course.course_code, command.student.course_list[i]) == 0)
        {
            printf("TO be DROPPED: %s", command.student.course_list[i]);
            for (; i < command.student.course_count - 1; i++)
            {
                strcpy(command.student.course_list[i], command.student.course_list[i + 1]);
            }
            command.student.course_count--;
            isstudentenrolled = 1;
            break;
        }
    }

    if (!isstudentenrolled)
    {
        printf("Student not enrolled.\n");
        return;
    }

    int response = sendToServer(command);
    if (response == SUCCESS)
    {
        printf("Student dropped out successfully.\n");

        for (int i = 0; i < command.student.course_count; i++)
        {
            printf("%s %d ", ((struct Student *)user)->course_list[i], strcmp(command.course.course_code, ((struct Student *)user)->course_list[i]) == 0);
            strcpy(((struct Student *)user)->course_list[i], command.student.course_list[i]);
        }
        ((struct Student *)user)->course_count--;
        for (int i = 0; i < ((struct Student *)user)->course_count; i++)
        {
            printf("%s %d ", ((struct Student *)user)->course_list[i], strcmp(command.course.course_code, ((struct Student *)user)->course_list[i]) == 0);
        }
    }
    else if (response == NOT_FOUND)
        printf("Course not found.\n");
    else
        printf("Error occurred.\n");
}

void showStudentCommands()
{
    if (((struct Student *)user)->isActive == ISBLOCKED)
    {
        printf("You are blocked. Please contact admin.\n");
        logout();
        return;
    }

    printf("Student Commands:\n");
    printf("1. View Course\n");
    printf("2. Enroll in Course\n");
    printf("3. Drop Course\n");
    printf("4. Change Password\n");
    printf("5. Logout\n");
    printf("Enter choice: ");
    int choice;
    scanf("%d", &choice);
    printf("\n----------------------------------------------\n\n");

    switch (choice)
    {
    case 1:
        viewCourses(STUDENT);
        break;
    case 2:
        enrollCourse();
        break;
    case 3:
        dropCourse();
        break;
    case 4:
        changePassword(STUDENT, STUDENT);
        break;
    case 5:
        logout();
        break;
    default:
        printf("Invalid choice. Please try again.\n\n");
        break;
    }
}

void handleSignals(int sig)
{
    printf("\nExiting...\n");
    free(user);
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
            int struct_size = 0;
            if (role == ADMIN)
            {
                user = malloc(sizeof(struct Admin));
                struct_size = sizeof(struct Admin);
            }
            else if (role == FACULTY)
            {
                user = malloc(sizeof(struct Faculty));
                struct_size = sizeof(struct Faculty);
            }
            else
            {
                user = malloc(sizeof(struct Student));
                struct_size = sizeof(struct Student);
            }

            if (recv(sockfd, user, struct_size, 0) < 0)
            {
                perror("Receive failed");
                close(sockfd);
                exit(EXIT_FAILURE);
            }

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
        case NOT_FOUND:
            printf("User not found.\n");
            break;
        default:
            printf("Unknown error occurred: %d\n", response);
            break;
        }
    }

    while (1)
    {
        printf("\n----------------------------------------------\n\n");
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
    printf("\n----------------------------------------------\n\n");
    free(user);
    // Close socket
    close(sockfd);
    return 0;
}