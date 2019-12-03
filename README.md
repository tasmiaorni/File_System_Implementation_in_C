# File_System_Implementation_in_C

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
