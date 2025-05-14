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

// Function to add a new entity (student, faculty, or course)
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
        PRINT("Enter student name: ");
        scanf("%s", command.student.name);

        PRINT("Enter student password: ");
        scanf("%s", command.student.password);

        PRINT("Enter student ID: ");
        scanf("%s", command.student.student_id);

        int isActive;
        PRINT("Is student active? (1 for yes, 0 for no): ");
        scanf("%d", &isActive);
        command.student.isActive = isActive + ISBLOCKED;

        command.student.isEXISTS = 1;
        command.student.course_count = 0;
    }
    else if (type == FACULTY)
    {
        PRINT("Enter faculty name: ");
        scanf("%s", command.faculty.name);

        PRINT("Enter faculty password: ");
        scanf("%s", command.faculty.password);

        PRINT("Enter faculty ID: ");
        scanf("%s", command.faculty.faculty_id);

        PRINT("Enter department: ");
        scanf("%s", command.faculty.department);

        command.faculty.isEXISTS = 1;
        command.faculty.course_count = 0;
    }
    else if (type == COURSE)
    {
        PRINT("Enter course name: ");
        scanf("%s", command.course.course_name);

        PRINT("Enter course code: ");
        scanf("%s", command.course.course_code);

        PRINT("Enter student limit: ");
        scanf("%d", &command.course.student_limit);
        strcpy(command.course.faculty_id, ((struct Faculty *)user)->faculty_id);
        PRINT("Faculty ID: %s\n", ((struct Faculty *)user)->faculty_id);
    }

    int response = sendToServer(command);
    if (response == SUCCESS)
    {
        if (type != COURSE)
        {
            // if the added entity is a student or faculty
            PRINT("Added successfully.\n");
        }
        else
        {
            // if course is added successfully, add it to the faculty's course list
            strcpy(((struct Faculty *)user)->courses[((struct Faculty *)user)->course_count],
                   command.course.course_code);
            ((struct Faculty *)user)->course_count++;

            struct Command com;
            com.faculty = *((struct Faculty *)user);
            com.cmd_code = MODIFY_FACULTY;
            com.role = ADMIN;
            com.whereData = FACULTY;

            int res = sendToServer(com);
            if (res == SUCCESS)
                PRINT("Added successfully.\n");
            else if (response == NOT_FOUND)
                PRINT("Faculty not found.\n");
            else
                PRINT("Failed to add entity.\n");
        }
    }
    else if (response == STUDENT_ALREADY_ADDED)
        PRINT("Student already exists.\n");
    else if (response == FACULTY_ALREADY_ADDED)
        PRINT("Entity already exists.\n");
    else if (response == COURSE_ALREADY_ADDED)
        PRINT("Course already exists.\n");
    else
        PRINT("Failed to add entity.\n");
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

        if (entity_count == 0)
        {
            PRINT("No entities found.\n");
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

        if (type == STUDENT)
            PRINT("Student details:\n");
        else if (type == FACULTY)
            PRINT("Faculty details:\n");
        else if (type == COURSE)
            PRINT("Course details:\n");

        printf("****************************************************************************\n");
        for (int i = 0; i < entity_count; i++)
        {
            if (type == STUDENT)
            {
                struct Student *student = (struct Student *)entities_list + i;
                PRINT("Name: %s, ID: %s, Active: %d, Course Count: %d\n",
                      student->name,
                      student->student_id,
                      student->isActive - ISBLOCKED,
                      student->course_count);

                if (student->course_count > 0)
                {
                    PRINT("Courses: ");
                    for (int j = 0; j < student->course_count; j++)
                    {
                        PRINT("%s ", student->course_list[j]);
                    }
                    PRINT("\n");
                }
            }
            else if (type == FACULTY)
            {
                struct Faculty *faculty = (struct Faculty *)entities_list + i;
                PRINT("Name: %s, ID: %s, Department: %s, Course Count: %d\n",
                      faculty->name,
                      faculty->faculty_id,
                      faculty->department,
                      faculty->course_count);

                if (faculty->course_count > 0)
                {
                    PRINT("Courses: ");
                    for (int j = 0; j < faculty->course_count; j++)
                    {
                        PRINT("%s ", faculty->courses[j]);
                    }
                    PRINT("\n");
                }
            }
            else if (type == COURSE)
            {
                // this is for viewing all courses irrespective of the faculty
                struct Course *course = (struct Course *)entities_list + i;
                if (strcmp(course->faculty_id, ((struct Faculty *)user)->faculty_id))
                    continue;

                PRINT("Name: %s, Code: %s, Student Count: %d/%d, Faculty: %s\n",
                      course->course_name,
                      course->course_code,
                      course->student_count,
                      course->student_limit,
                      course->faculty_id);

                if (course->student_count > 0)
                {
                    PRINT("Students: ");
                    for (int j = 0; j < course->student_count; j++)
                    {
                        PRINT("%s ", course->studentlist[j]);
                    }
                    PRINT("\n");
                }
            }
            printf("****************************************************************************\n");
        }
        free(entities_list);
    }
    else if (response == NOT_FOUND)
        PRINT("Entity not found.\n");
    else
        PRINT("Failed to retrieve entity details.\n");
}

void changeStudentActiveness(int isActive)
{
    struct Command command;
    command.role = ADMIN;
    command.whereData = STUDENT;

    PRINT("Enter student ID: ");
    scanf("%s", command.student.student_id);

    command.student.isActive = isActive;
    command.cmd_code = (isActive == ISACTIVE) ? ACTIVATE_STUDENT : BLOCK_STUDENT;

    int response = sendToServer(command);
    if (response == SUCCESS)
        (isActive == ISACTIVE) ? PRINT("Student activeness changed successfully to ACTIVE\n")
                               : PRINT("Student activeness changed successfully to BLOCKED\n");
    else
        PRINT("Failed to change student activeness.\n");
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
        PRINT("Enter student ID: ");
        scanf("%s", command.student.student_id);

        PRINT("Enter new student name: ");
        scanf("%s", command.student.name);

        PRINT("Enter new student password: ");
        scanf("%s", command.student.password);
        command.student.isEXISTS = 1;
    }
    else if (type == FACULTY)
    {
        PRINT("Enter faculty ID: ");
        scanf("%s", command.faculty.faculty_id);
        PRINT("Enter new faculty name: ");
        scanf("%s", command.faculty.name);
        PRINT("Enter new faculty password: ");
        scanf("%s", command.faculty.password);
        PRINT("Enter new department: ");
        scanf("%s", command.faculty.department);
        command.faculty.isEXISTS = 1;
    }
    else if (type == COURSE)
    {
        PRINT("Enter course code: ");
        scanf("%s", command.course.course_code);

        PRINT("Enter new course name: ");
        scanf("%s", command.course.course_name);

        PRINT("Enter new student limit: ");
        scanf("%d", &command.course.student_limit);
        command.course.isEXISTS = 1;
    }

    int response = sendToServer(command);
    if (response == SUCCESS)
    {
        PRINT("Entity modified successfully.\n");
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
        PRINT("Failed to modify entity. %d\n", response);
}

void logout()
{
    struct Command command;
    command.cmd_code = LOGOUT;
    command.role = NO_USER;
    command.whereData = NO_USER;

    int response = sendToServer(command);
    if (response == SUCCESS)
        PRINT("Logged out successfully.\n");
    else
        PRINT("Failed to log out.\n");
    close(sockfd);
    exit(SUCCESS);
}

void showAdminCommands()
{
    PRINT("Admin Commands:\n");
    PRINT("1. Add Student\n");
    PRINT("2. View Student Details\n");
    PRINT("3. Add Faculty\n");
    PRINT("4. View Faculty Details\n");
    PRINT("5. Activate Student\n");
    PRINT("6. Block Student\n");
    PRINT("7. Modify Student Details\n");
    PRINT("8. Modify Faculty Details\n");
    PRINT("9. Logout and Exit\n");
    PRINT("Enter choice: ");
    int choice;
    scanf("%d", &choice);
    PRINT("\n----------------------------------------------\n\n");

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
        PRINT("Invalid choice. Please try again.\n");
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

        if (entity_count == 0)
        {
            PRINT("No entities found.\n");
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

        PRINT("Course details:\n");
        printf("****************************************************************************\n");
        for (int i = 0; i < entity_count; i++)
        {
            if (type == FACULTY)
            {
                struct Course *course = (struct Course *)entities_list + i;
                PRINT("Name: %s, Code: %s, Student Count: %d/%d, Faculty: %s\n",
                      course->course_name,
                      course->course_code,
                      course->student_count,
                      course->student_limit,
                      course->faculty_id);

                if (course->student_count > 0)
                {
                    PRINT("Students: ");
                    for (int j = 0; j < course->student_count; j++)
                    {
                        PRINT("%s ", course->studentlist[j]);
                    }
                    PRINT("\n");
                }
            }

            else if (type == STUDENT)
            {
                struct Course *course = (struct Course *)entities_list + i;
                for (int j = 0; j < course->student_count; j++)
                {
                    if (strcmp(course->studentlist[j], ((struct Student *)user)->student_id))
                        continue;
                }

                PRINT("Name: %s, Code: %s, Faculty: %s\n",
                      course->course_name,
                      course->course_code,
                      course->faculty_id);
            }
            printf("****************************************************************************\n");
        }
        free(entities_list);
    }
    else if (response == NOT_FOUND)
        PRINT("No courses found.\n");
    else
        PRINT("Failed to retrieve entity details.\n");
}

void removeCourse()
{
    struct Command command;
    command.cmd_code = REMOVE_COURSE;
    command.role = FACULTY;
    command.whereData = COURSE;

    PRINT("Enter course code: ");
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
        PRINT("Course removed successfully.\n");
        // copy the contents into user as well
        for (int i = 0; i < command.faculty.course_count; i++)
        {
            strcpy(((struct Faculty *)user)->courses[i], command.faculty.courses[i]);
        }
        ((struct Faculty *)user)->course_count--;
    }
    else if (response == INVALID_OPERATION)
        PRINT("All students must drop the course before it can be removed.\n");
    else if (response == NOT_FOUND)
    {
        PRINT("Course not found.\n");
    }
    else
        PRINT("Failed to remove course.\n");
}

void changePassword(int role, int type)
{
    struct Command command;
    command.cmd_code = CHANGE_PASSWORD;
    command.role = role;
    command.whereData = type;

    char pwd[50];
    PRINT("Enter new password: ");
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
        PRINT("Password changed successfully.\n");
        if (type == STUDENT)
            strcpy(((struct Student *)user)->password, pwd);
        else if (type == FACULTY)
            strcpy(((struct Faculty *)user)->password, pwd);
    }
    else
        PRINT("Failed to change password.\n");
}

void showFacultyCommands()
{
    PRINT("Faculty Commands:\n");
    PRINT("1. Add Course\n");
    PRINT("2. Remove Course\n");
    PRINT("3. View Course\n");
    PRINT("4. Modify Course\n");
    PRINT("5. Change Password\n");
    PRINT("6. Logout\n");
    PRINT("Enter choice: ");

    int choice;
    scanf("%d", &choice);
    PRINT("\n----------------------------------------------\n\n");
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
        PRINT("Invalid choice. Please try again.\n");
        break;
    }
}

void enrollCourse()
{
    struct Command command;
    command.cmd_code = ENROLL_COURSE;
    command.role = STUDENT;
    command.whereData = STUDENT;

    PRINT("Enter course code: ");
    scanf("%s", command.course.course_code);
    command.student = *((struct Student *)user);
    for (int i = 0; i < command.student.course_count; i++)
    {
        if (strcmp(command.course.course_code, command.student.course_list[i]) == 0)
        {
            PRINT("Student already enrolled.\n");
            return;
        }
    }
    strcpy(command.student.course_list[command.student.course_count],
           command.course.course_code);
    command.student.course_count++;

    int response = sendToServer(command);
    if (response == SUCCESS)
    {
        PRINT("Student enrolled successfully.\n");
        strcpy(((struct Student *)user)->course_list[((struct Student *)user)->course_count],
               command.course.course_code);
        ((struct Student *)user)->course_count++;
    }
    else if (response == NOT_FOUND)
        PRINT("Course not found.\n");
    else if (response == LIMIT_EXCEEDED)
        PRINT("Student limit reached. Cannot enroll for this course.");
    else
        PRINT("Error occurred.\n");
}

void dropCourse()
{
    struct Command command;
    command.cmd_code = DROP_COURSE;
    command.role = STUDENT;
    command.whereData = STUDENT;

    PRINT("Enter course code: ");
    scanf("%s", command.course.course_code);
    command.student = *((struct Student *)user);
    int isstudentenrolled = 0;
    for (int i = 0; i < command.student.course_count; i++)
    {
        if (strcmp(command.course.course_code, command.student.course_list[i]) == 0)
        {
            for (; i < command.student.course_count - 1; i++)
            {
                strcpy(command.student.course_list[i],
                       command.student.course_list[i + 1]);
            }
            command.student.course_count--;
            isstudentenrolled = 1;
            break;
        }
    }

    if (!isstudentenrolled)
    {
        PRINT("Student is not enrolled in this course.\n");
        return;
    }

    int response = sendToServer(command);
    if (response == SUCCESS)
    {
        PRINT("Student dropped out successfully.\n");

        for (int i = 0; i < command.student.course_count; i++)
        {
            strcpy(((struct Student *)user)->course_list[i],
                   command.student.course_list[i]);
        }
        ((struct Student *)user)->course_count--;
    }
    else if (response == NOT_FOUND)
        PRINT("Course not found.\n");
    else
        PRINT("Error occurred.\n");
}

void showStudentCommands()
{
    if (((struct Student *)user)->isActive == ISBLOCKED)
    {
        PRINT("You are blocked. Please contact admin.\n");
        logout();
        return;
    }

    PRINT("Student Commands:\n");
    PRINT("1. View Course\n");
    PRINT("2. Enroll in Course\n");
    PRINT("3. Drop Course\n");
    PRINT("4. Change Password\n");
    PRINT("5. Logout\n");
    PRINT("Enter choice: ");
    int choice;
    scanf("%d", &choice);
    PRINT("\n----------------------------------------------\n\n");

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
        PRINT("Invalid choice. Please try again.\n");
        break;
    }
}

void handleSignals(int sig)
{
    PRINT("\nExiting...\n");
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
        PRINT("Enter your role (\n\t0 for Admin, \n\t1 for Faculty, \n\t2 for Student\n): ");
        int role;
        scanf("%d", &role);
        role += ADMIN;
        if (role < ADMIN || role > STUDENT)
        {
            PRINT("Invalid role. Please try again.\n");
            continue;
        }
        PRINT("Enter your username: ");
        char username[50];
        scanf("%s", username);
        PRINT("Enter your password: ");
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
            strcpy(command.faculty.faculty_id, username);
            strcpy(command.faculty.password, password);
        }
        else if (role == STUDENT)
        {
            strcpy(command.student.student_id, username);
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
            PRINT("Login failed. Please try again.\n");
            break;
        case NOT_LOGGED_IN:
            PRINT("You are not logged in.\n");
            break;
        case INVALID_CREDENTIALS:
            PRINT("Invalid credentials. Please try again.\n");
            break;
        case NOT_FOUND:
            PRINT("User not found.\n");
            break;
        default:
            PRINT("Unknown error occurred: %d\n", response);
            break;
        }
    }

    while (1)
    {
        PRINT("\n----------------------------------------------\n\n");
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
    PRINT("\n----------------------------------------------\n\n");
    free(user);
    close(sockfd);
    return 0;
}