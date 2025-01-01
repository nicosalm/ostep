/*
 * process exercises: demonstrations of fork, pipe, and i/o redirection
 * questions from ostep chapter 5
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

/*
 * q1: fork basics
 * demonstrates how child and parent processes get their own copy of variables
 */
void q1() {
    int x = 100;
    printf("before fork: %d\n", x);
    int rc = fork();

    if (rc < 0) {         // fork failed
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) { // child process
        x = 25;
        printf("inside child: %d\n", x);
    } else {              // parent process (rc > 0)
        x = 50;
        printf("inside parent: %d\n", x);
    }

    printf("end: %d\n", x);
}

/*
 * q2: file descriptors and concurrent writing
 * demonstrates how child and parent can share a file descriptor
 * shows concurrent writing to the same file
 */
void q2() {
    // open file BEFORE fork so both processes share same file descriptor
    int fd = open("./q2.output", O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);

    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        printf("Child has fd: %d\n", fd);
        for (int i = 0; i < 1000; i++) {
            char buf[100];
            sprintf(buf, "child %d\n", i);
            write(fd, buf, strlen(buf));
            // do some work to consume CPU time
            for(int j = 0; j < 1000000; j++) { }
        }
    } else {
        printf("Parent has fd: %d\n", fd);
        for (int i = 0; i < 1000; i++) {
            char buf[100];
            sprintf(buf, "parent %d\n", i);
            write(fd, buf, strlen(buf));
            // Do some work to consume CPU time
            for(int j = 0; j < 1000000; j++) { }
        }
    }

    close(fd);  //both processes close the same fd
}

/*
 * q3: process synchronization without wait
 * uses a pipe to ensure child prints "hello" before parent prints "goodbye"
 */
void q3() {
    int pipefd[2];
    pipe(pipefd);  // create a pipe before forking

    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        printf("hello\n");
        // signal to parent we're done
        write(pipefd[1], "done", 4);
        close(pipefd[1]);
    } else  {
        char buf[10];
        // wait for child's signal
        read(pipefd[0], buf, 4);
        close(pipefd[0]);
        printf("goodbye\n");
    }
}

/*
 * q4: exec variants
 * demonstrates different ways to execute a new program using exec family
 */
void q4() {
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        // show multiple exec variants
        printf("child: about to exec ls\n");

        // could cycle through these variants:
        execl("/bin/ls", "ls", NULL);
        // execle("/bin/ls", "ls", NULL, (char *[]){NULL});
        // execlp("ls", "ls", NULL);
        // execv("/bin/ls", (char *[]){"ls", NULL});
        // execvp("ls", (char *[]){"ls", NULL});

        printf("this shouldn't print\n");
    } else {
        wait(NULL);
    }
}

/*
 * q5: understanding wait
 * demonstrates how wait() works and what it returns
 */
void q5() {
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        printf("child: pid %d\n", (int)getpid());
        // try wait in child
        int wc = wait(NULL);
        printf("child wait returned: %d\n", wc);
    } else {
        int wc = wait(NULL);
        printf("parent wait returned: %d\n", wc);
    }
}

/*
 * q6: waitpid versus wait
 * shows how to wait for specific child process
 */
void q6() {
    int rc1 = fork();
    if (rc1 < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc1 == 0) {
        printf("child 1: pid %d\n", (int)getpid());
        sleep(1);
    } else {
        int rc2 = fork();
        if (rc2 < 0) {
            fprintf(stderr, "fork failed\n");
            exit(1);
        } else if (rc2 == 0) {
            printf("child 2: pid %d\n", (int)getpid());
            sleep(2);
        } else {
            waitpid(rc2, NULL, 0);  // wait specifically for second child
            printf("parent: child 2 finished\n");
        }
    }
}

/*
 * q7: closing standard output
 * demonstrates behavior of printf and write after closing stdout
 */
void q7() {
    int rc = fork();

    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        printf("child before closing stdout: this should print\n");
        close(STDOUT_FILENO);  // close stdout

        // try to print after closing stdout
        printf("child after closing stdout: this shouldn't show\n");

        // let's also try write() to see the difference
        write(STDOUT_FILENO, "try direct write\n", 16);

        // can still print to stderr
        fprintf(stderr, "child stderr: this should print\n");
    } else {
        wait(NULL);
        printf("parent: still can print\n");
    }
}

/*
 * q8: pipe between two children
 * implements "ls | wc -l" using fork, pipe, and exec
 * connects stdout of first child to stdin of second child
 */
void q8() {
    int pipefd[2];
    pipe(pipefd);  // create pipe before forking

    int rc1 = fork();  // first fork
    if (rc1 < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc1 == 0) {  // first child (writer)
        close(pipefd[0]);   // close unused read end

        // redirect stdout to pipe write end
        close(STDOUT_FILENO);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        // execute 'ls' ... its output will go to the pipe
        char *args[] = {"ls", NULL};
        execvp(args[0], args);

        fprintf(stderr, "exec failed\n");
        exit(1);
    }

    int rc2 = fork();  // second fork
    if (rc2 < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc2 == 0) {  // second child (reader)
        close(pipefd[1]);   // close unused write end

        // redirect stdin to pipe read end
        close(STDIN_FILENO);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        // execute 'wc' ... it will read from the pipe
        char *args[] = {"wc", "-l", NULL};
        execvp(args[0], args);

        fprintf(stderr, "exec failed\n");
        exit(1);
    }

    // parent closes both ends of pipe
    close(pipefd[0]);
    close(pipefd[1]);

    // wait for both children
    waitpid(rc1, NULL, 0);
    waitpid(rc2, NULL, 0);
}

int main() {
    // q1();
    // q2();
    // q3();
    // q4();
    // q5();
    // q6();
    // q7();
    q8();

    return 0;
}
