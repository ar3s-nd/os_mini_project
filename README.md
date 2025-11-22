# Project Description
This project is a multi-client course registration system built entirely in C, designed using several core Operating Systems concepts such as process management, file handling, socket programming, multithreading, and synchronization.

The system supports role-based access for Admin, Faculty, and Students, enabling secure login and controlled operations.
A file-based storage system maintains all user and course data, while a server–client architecture handles multiple users concurrently through threads (pthreads).
To ensure data consistency during course enrolment and modification, mutex locks protect critical sections and prevent race conditions.

Overall, the project demonstrates how OS principles can be combined to build a robust, concurrent, and secure academic management portal.
This project was done as part of a course EGC - 301 Operating Systems in IIITB. 

# Directory Structure
```
├── bin/                             # Compiled executables for client.c and server.c
├── db/                              # Project report, diagram PDFs, slides
|   ├── admin.txt                       # stores data about the admin users
|   ├── course.txt                      # stores data about the courses
|   ├── faculty.txt                     # stores data about the faculty users
|   ├── log.txt                         # stores the server logs   
│   └── student.txt                     # stores data about the student users   
├── lib/                             # Header files for shared modules
|   ├── client.h                        # library module for the client
│   └── server.h                        # library module for the server   
├── src/                             # Source code files for each module
│   ├── client.c                        # client-side code
│   ├── functions.c                     # contains the library functions
│   ├── mutex.c                         # code for initialising the mutexes
│   └──  server.c                       # server-side code
├── test/                            # contains dummy test input data for admin, student and faculty
│   ├── admin                           # dummy test data for admins
│   ├── faculty1                        # dummy test data for faculty number 1
│   ├── faculty2                        # dummy test data for faculty number 2
│   ├── faculty3                        # dummy test data for faculty number 3
│   ├── student1                        # dummy test data for faculty number 1
│   ├── student2                        # dummy test data for student number 2
│   └── student3                        # dummy test data for student number 3
├── client.sh                        # shell script for running client.c
├── server.sh                        # shell script for running server.c
├── init.sh                          # shell script for intialising the database with dummy values from test/
├── IMT2023095_project_report.pdf    # report of the project
└── README.md                        # readme file
```
# Project Setup Instructions
This project includes the following scripts to help you get started:

1. **`server.sh`**  
   Use this script to start the server. Ensure the server is running before interacting with the client. Logs will be made in db/logs.txt.

   ```bash
   bash server.sh
   ```

2. **`init.sh`**  
   Run this script to initialize the database files with dummy values. It sets up the necessary data for the project to function correctly.

   ```bash
   bash init.sh
   ```

3. **`client.sh`**  
   Run this script to execute the client-side code and interact with the server.
   ```bash
   bash client.sh
   ```

---

> ⚠️ **Note:**
> Follow the above steps **in order** to ensure proper functionality of the project.

---


**Submitted by:**  
   Navaneeth D  
   IMT2023095

