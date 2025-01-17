use nix::sys::wait::{wait, waitpid};
use nix::unistd;
use nix::unistd::{fork, pipe, ForkResult};
use std::fs::File;
use std::io::{Read, Write};
use std::os::unix::io::{AsRawFd, FromRawFd};
use std::process::{exit, Command};

#[allow(dead_code)]
mod exercises {
    use super::*;
    // q1: fork basics
    pub fn q1() {
        let x = 100;
        println!("before fork: {}", x);

        match unsafe { fork() } {
            Ok(ForkResult::Parent { child: _ }) => {
                let x = 50;
                println!("inside parent: {}", x);
            }
            Ok(ForkResult::Child) => {
                let x = 25;
                println!("inside child: {}", x);
            }
            Err(_) => {
                eprintln!("fork failed");
                exit(1);
            }
        }
        println!("end: {}", x);
    }

    // q2: file descriptors and concurrent writing
    pub fn q2() {
        let file = File::create("./q2.output").expect("failed to create file");
        let fd = file.as_raw_fd();

        match unsafe { fork() } {
            Ok(ForkResult::Child) => {
                println!("Child has fd: {}", fd);
                for i in 0..1000 {
                    let buf = format!("child {}\n", i);
                    unsafe {
                        let mut file = File::from_raw_fd(fd);
                        file.write_all(buf.as_bytes()).expect("write failed");
                        // don't close file ... avoids closing fd
                        std::mem::forget(file);
                    }
                    // busy work
                    for _ in 0..1000000 {}
                }
            }
            Ok(ForkResult::Parent { child: _ }) => {
                println!("Parent has fd: {}", fd);
                for i in 0..1000 {
                    let buf = format!("parent {}\n", i);
                    unsafe {
                        let mut file = File::from_raw_fd(fd);
                        file.write_all(buf.as_bytes()).expect("write failed");
                        std::mem::forget(file);
                    }
                    // busy work
                    for _ in 0..1000000 {}
                }
            }
            Err(_) => {
                eprintln!("fork failed");
                exit(1);
            }
        }
    }

    // q3: process synchronization without wait
    pub fn q3() {
        let (read_fd, write_fd) = pipe().expect("pipe failed");

        match unsafe { fork() } {
            Ok(ForkResult::Child) => {
                println!("hello");
                // signal to parent we're done
                unsafe {
                    let mut pipe = File::from_raw_fd(write_fd);
                    pipe.write_all(b"done").expect("write failed");
                }
                // read_fd automatically closed
            }
            Ok(ForkResult::Parent { child: _ }) => {
                // Wait for child's signal
                let mut buf = [0u8; 4];
                unsafe {
                    let mut pipe = File::from_raw_fd(read_fd);
                    pipe.read_exact(&mut buf).expect("read failed");
                }
                println!("goodbye");
                // write_fd automatically closed
            }
            Err(_) => {
                eprintln!("fork failed");
                exit(1);
            }
        }
    }

    // q4: exec variants
    pub fn q4() {
        match unsafe { fork() } {
            Ok(ForkResult::Child) => {
                println!("child: about to exec ls");

                // rust equivalent of different exec variants

                // execv style (using Command)
                Command::new("/bin/ls")
                    .spawn()
                    .expect("exec failed")
                    .wait()
                    .expect("wait failed");

                // could also use nix::unistd::execvp directly (more C-like)
                // let args = ["ls".to_string()];
                // nix::unistd::execvp(
                //     &std::ffi::CString::new("ls").unwrap(),
                //     &[&std::ffi::CString::new("ls").unwrap()]
                // ).expect("exec failed");

                println!("this shouldn't print");
                exit(0);
            }
            Ok(ForkResult::Parent { child }) => {
                waitpid(Some(child), None).expect("wait failed");
            }
            Err(_) => {
                eprintln!("fork failed");
                exit(1);
            }
        }
    }

    // q5: understanding wait
    pub fn q5() {
        match unsafe { fork() } {
            Ok(ForkResult::Child) => {
                println!("child: pid {}", unistd::getpid());
                // try wait in child
                match wait() {
                    Ok(status) => println!("child wait returned: {:?}", status),
                    Err(e) => println!("child wait returned error: {}", e),
                }
            }
            Ok(ForkResult::Parent { child }) => match waitpid(Some(child), None) {
                Ok(status) => println!("parent wait returned: {:?}", status),
                Err(e) => println!("parent wait returned error: {}", e),
            },
            Err(_) => {
                eprintln!("fork failed");
                exit(1);
            }
        }
    }

    // q6: waitpid versus wait
    pub fn q6() {
        match unsafe { fork() } {
            Ok(ForkResult::Child) => {
                println!("child 1: pid {}", unistd::getpid());
                std::thread::sleep(std::time::Duration::from_secs(1));
                exit(0);
            }
            Ok(ForkResult::Parent { child: _child1 }) => {
                match unsafe { fork() } {
                    Ok(ForkResult::Child) => {
                        println!("child 2: pid {}", unistd::getpid());
                        std::thread::sleep(std::time::Duration::from_secs(2));
                        exit(0);
                    }
                    Ok(ForkResult::Parent { child: child2 }) => {
                        // wait specifically for second child
                        waitpid(Some(child2), None).expect("waitpid failed");
                        println!("parent: child 2 finished");
                        // could wait for child1 here if needed
                    }
                    Err(_) => {
                        eprintln!("fork failed");
                        exit(1);
                    }
                }
            }
            Err(_) => {
                eprintln!("fork failed");
                exit(1);
            }
        }
    }

    // q7: closing standard output
    pub fn q7() {
        match unsafe { fork() } {
            Ok(ForkResult::Child) => {
                println!("child before closing stdout: this should print");

                // close stdout
                unistd::close(1).expect("close failed");

                // try to print after closing stdout
                println!("child after closing stdout: this shouldn't show");

                // try direct write to stdout
                let message = b"try direct write\n";
                unsafe {
                    let mut stdout = File::from_raw_fd(1);
                    let _ = stdout.write_all(message); // this will fail
                    std::mem::forget(stdout); // don't close fd 1 again
                }

                // can still print to stderr
                eprintln!("child stderr: this should print");
            }
            Ok(ForkResult::Parent { child }) => {
                waitpid(Some(child), None).expect("wait failed");
                println!("parent: still can print");
            }
            Err(_) => {
                eprintln!("fork failed");
                exit(1);
            }
        }
    }

    // q8: pipe between two children
    pub fn q8() {
        let (read_fd, write_fd) = pipe().expect("pipe failed");

        match unsafe { fork() } {
            Ok(ForkResult::Child) => {
                // first child (writer)
                nix::unistd::close(read_fd).expect("close failed");
                nix::unistd::dup2(write_fd, 1).expect("dup2 failed");
                nix::unistd::close(write_fd).expect("close failed");

                Command::new("ls")
                    .spawn()
                    .expect("ls failed")
                    .wait()
                    .expect("wait failed");
                exit(0);
            }
            Ok(ForkResult::Parent { child: child1 }) => {
                match unsafe { fork() } {
                    Ok(ForkResult::Child) => {
                        // second child (reader)
                        nix::unistd::close(write_fd).expect("close failed");
                        nix::unistd::dup2(read_fd, 0).expect("dup2 failed");
                        nix::unistd::close(read_fd).expect("close failed");

                        Command::new("wc")
                            .arg("-l")
                            .spawn()
                            .expect("wc failed")
                            .wait()
                            .expect("wait failed");
                        exit(0);
                    }
                    Ok(ForkResult::Parent { child: child2 }) => {
                        // parent
                        nix::unistd::close(read_fd).expect("close failed");
                        nix::unistd::close(write_fd).expect("close failed");

                        waitpid(Some(child1), None).expect("waitpid failed");
                        waitpid(Some(child2), None).expect("waitpid failed");
                    }
                    Err(_) => {
                        eprintln!("fork failed");
                        exit(1);
                    }
                }
            }
            Err(_) => {
                eprintln!("fork failed");
                exit(1);
            }
        }
    }
}

fn main() {
    // exercises::q1();
    // exercises::q2();
    // exercises::q3();
    // exercises::q4();
    // exercises::q5();
    // exercises::q6();
    // exercises::q7();
    exercises::q8();
}
