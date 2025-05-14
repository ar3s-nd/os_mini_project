# This shell script is used to initialize the project with dummy data.
# Data created by this script are:
# - 1 admin (admin1010)
# - 3 faculty (f1, f2, f3) 
# - Student s1 in courses: c1 offered by f1 and c4 offered by f2
# - Student s2 in courses: c2 offered by f1 and c5 offered by f3
# - Student s3 in courses: c3 offered by f2 and c6 offered by f3

gcc src/client.c -o bin/client
./bin/client < test/admin
./bin/client < test/faculty1
./bin/client < test/faculty2
./bin/client < test/faculty3
./bin/client < test/student1
./bin/client < test/student2
./bin/client < test/student3