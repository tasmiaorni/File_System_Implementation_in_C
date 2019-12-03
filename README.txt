This directory contains a simple demonstration of file system management for project 2.
In this project we built a user-level library, LibFS , that implements the file system.
Directory Contents ========================================================================== 
This is the directory structure and package description.
group3_project2
   |- report.pdf      
   |- File_Manager_in_C
   |    |- LibDisk.c : The source file for the disk abstraction
   |    |- LibDisk.h : The header file for disk abstraction
   |    |- LibFS.c : The LibFS source file
   |    |- LibFS.h : The LibFS header
   |    |- main.c: An example
   |    |- Makefile : An example MakeFile
   |	|- Makefile.LibDisk : An example MakeFile for LibDisk
   |    |- Makefile.LibFS   : An example MakeFile for LibFS
   |    |- simple-test.c : A simple test application to implement file system commands like file create,file unlink, directory create, direcrtory unlink, file open, file write
   |    |- slow-cat.c    : A simple test application to implement file system command file read
   |    |- slow-export.c : A simple test application to implement copying a unix file out from our simple file system
   |    |- slow-import.c : A simple test application to implement copying a unix file into from our simple file system
   |    |- slow-ls.c     : A simple test application to implement file system command ls
   |    |- slow-mkdir.c  : A simple test application to implement file system command file read
   |    |- slow-touch.c  : create an empty file
   |    |- slow-rm.c     : A simple test application to implement file system command rm
   |    |- slow-rmdir.c  : A simple test application to implement file system command file rmdir   
   |- README.txt


Compilation ========================================================================== 
Step 1.
Before testing shared library, one needs to set the environment by using below command:

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. 

Step 2. 

make clean
make

Step 3. 
1)
./simple-test.exe <disk_image_file>

e.g. ./simple-test.exe test 

This will check some basic implementations like file create,file unlink, directory create, direcrtory unlink, file open, file write.

2)
 ./slow-ls.exe <disk_image_file> <path>
e.g. ./slow-ls.exe test /

This will show the listings of the given path.

3) 
./slow-cat.exe <disk_image_file> <path>

e.g. ./slow-cat.exe test /second-file

This will show the contents of the given file.

4)
./slow-mkdir.exe <disk_image_file> <path>

e.g. ./slow-mkdir.exe test /first-dir
 
This will create a directory on the given path.

4)
./slow-rmdir.exe <disk_image_file> <path>

e.g. ./slow-rmdir.exe test /first-dir
 
This will unlink a directory on the given path.

5)
./slow-touch.exe <disk_image_file> <path>

e.g. ./slow-touch.exe test /first-file
 
This will create an empty file.

6)
./slow-rm.exe <disk_image_file> <path>

e.g. ./slow-rm.exe test /first-file
 
This will remove a file.

7)
./slow-export.exe <disk_image_file> <path> <filename>

e.g. ./slow-export.exe test /second-file  abc.txt
 
A simple test application to implement copying a unix file out from our simple file system
8)
./slow-import.exe <disk_image_file> <path> <filename>

e.g. ./slow-import.exe test /fourth-file  abc.txt
 
A simple test application to implement copying a unix file into  our simple file system

 

