#include "fifo.hpp"
#include "disk.hpp"
#include "output_io.hpp"
#include "output_queue.hpp"
#include "process.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <thread>
static void check(bool ok) { if (!ok) throw std::runtime_error("output regression"); }
namespace {
int pendingCloseError=0, failedCloseFd=-1, reusedCloseFd=-1;
unsigned failedCloseCalls=0;
}
extern "C" int __real_close(int fd);
extern "C" int __wrap_close(int fd) {
    if (pendingCloseError) {
        const int error=pendingCloseError;
        pendingCloseError=0;
        failedCloseFd=fd;
        ++failedCloseCalls;
        const int result=__real_close(fd);
        if (result<0) return result;
        // Reuse the released descriptor immediately, as another thread could.
        // Retrying close or retaining fd_ would now close this unrelated file.
        reusedCloseFd=open("/dev/null",O_RDONLY|O_CLOEXEC);
        errno=error;
        return -1;
    }
    if (fd==failedCloseFd) ++failedCloseCalls;
    return __real_close(fd);
}
static void delayedCloseError(const std::string &path, int error, bool explicitFinalize) {
    failedCloseCalls=0;
    {
        Disk disk;
        check(disk.enable(path)==0);
        std::vector<unsigned char> bytes{1,2,3};
        disk.output(&bytes);
        pendingCloseError=error;
        if (explicitFinalize) {
            bool failed=false;
            try { disk.finalize(); }
            catch (const std::runtime_error &exception) {
                failed=std::string(exception.what())==std::string("Output close failed: ")+std::strerror(error);
            }
            check(failed);
            disk.finalize(); // A failed close is still final on Linux.
            disk.disable();
        }
    } // A destructor-only close failure must not throw or retry either.
    check(pendingCloseError==0 && failedCloseCalls==1);
    check(reusedCloseFd==failedCloseFd && fcntl(reusedCloseFd,F_GETFD)>=0);
    failedCloseFd=-1;
    close(reusedCloseFd);
    reusedCloseFd=-1;
}
int main() {
    Process process; Process::setActive(true);
    char directory[]="/tmp/gchd-output-XXXXXX"; check(mkdtemp(directory)!=nullptr);
    const std::string existing=std::string(directory)+"/existing.ts";
    { std::ofstream file(existing); file << "preserve me"; }
    { Fifo fifo; check(fifo.enable(existing)!=0); }
    std::ifstream file(existing); std::string content; std::getline(file,content); check(content=="preserve me");
    const std::string alias=std::string(directory)+"/symlink";
    check(symlink(existing.c_str(),alias.c_str())==0);
    { Fifo fifo; check(fifo.enable(alias)!=0); }
    struct stat st{}; check(lstat(alias.c_str(),&st)==0 && S_ISLNK(st.st_mode));
    const std::string fifoPath=std::string(directory)+"/stream";
    std::thread reader([&] {
        for (;;) {
            int fd=open(fifoPath.c_str(),O_RDONLY|O_NONBLOCK);
            if (fd>=0) {
                unsigned char byte=0;
                for (;;) { auto n=read(fd,&byte,1); if(n==1) break; std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
                check(byte==42); close(fd); break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    { Fifo fifo; check(fifo.enable(fifoPath)==0); check(lstat(fifoPath.c_str(),&st)==0 && (st.st_mode&0777)==0600);
      std::vector<unsigned char> b{42}; fifo.output(&b); reader.join(); }
    check(lstat(fifoPath.c_str(),&st)<0);
    int stdoutPipe[2]; check(pipe(stdoutPipe)==0); const int savedStdout=dup(STDOUT_FILENO);
    check(dup2(stdoutPipe[1],STDOUT_FILENO)>=0); close(stdoutPipe[1]);
    { Disk stdoutOutput; check(stdoutOutput.enableStdout()==0); std::vector<unsigned char> b{17,42}; stdoutOutput.output(&b); }
    check(dup2(savedStdout,STDOUT_FILENO)>=0); close(savedStdout);
    unsigned char actual[2]{}; check(read(stdoutPipe[0],actual,2)==2 && actual[0]==17 && actual[1]==42); close(stdoutPipe[0]);
    int pipes[2]; check(pipe2(pipes,O_NONBLOCK|O_CLOEXEC)==0);
    std::vector<unsigned char> data(1024*1024,42); size_t received=0;
    std::thread drain([&] {
        unsigned char bytes[2048];
        while(received<data.size()) {
            auto count=read(pipes[0],bytes,sizeof bytes);
            if(count>0) { for(ssize_t i=0;i<count;++i) check(bytes[i]==42); received+=static_cast<size_t>(count); }
            else std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    OutputIO::writeAll(pipes[1],data.data(),data.size()); drain.join(); check(received==data.size());
    close(pipes[0]); bool broken=false;
    try { OutputIO::writeAll(pipes[1],data.data(),1); } catch (const std::runtime_error &) { broken=true; }
    check(broken); close(pipes[1]);
    { Disk disk; check(disk.enable("/dev/full")==0); bool full=false;
      try { disk.output(&data); } catch(const std::runtime_error &) { full=true; } check(full); }
    std::atomic<bool> entered{false}, release{false};
    OutputQueue queue([&](OutputQueue::Buffer &,const std::atomic<bool> *) {
        entered=true; while(!release) std::this_thread::yield();
    },2);
    queue.push({1}); while(!entered) std::this_thread::yield();
    check(queue.push({2})); check(queue.push({3})); check(!queue.push({4}));
    release=true; queue.finish(); check(queue.written()==3 && queue.dropped()==1 && queue.droppedBytes()==1);
    OutputQueue failed([](OutputQueue::Buffer &,const std::atomic<bool> *) { throw std::runtime_error("sink failed"); });
    failed.push({1}); bool failure=false; try { failed.finish(); } catch(const std::runtime_error &) { failure=true; } check(failure);
    int blocked[2]; check(pipe2(blocked,O_NONBLOCK|O_CLOEXEC)==0);
    unsigned char fill[4096]{}; while(write(blocked[1],fill,sizeof fill)>0) {}
    std::atomic<bool> waiting{false};
    OutputQueue stalled([&](OutputQueue::Buffer &b,const std::atomic<bool> *cancel) {
        waiting=true; OutputIO::writeAll(blocked[1],b.data(),b.size(),cancel);
    });
    stalled.push({42}); while(!waiting) std::this_thread::yield();
    const auto shutdownStart=std::chrono::steady_clock::now(); stalled.finish();
    check(std::chrono::steady_clock::now()-shutdownStart<std::chrono::seconds(4));
    check(stalled.dropped()==1); close(blocked[0]); close(blocked[1]);
    const std::string closePath=std::string(directory)+"/close.ts";
    for (int error: {ENOSPC,EIO,EINTR}) delayedCloseError(closePath,error,true);
    delayedCloseError(closePath,EIO,false);
    check(unlink(closePath.c_str())==0);
    pid_t child=fork(); check(child>=0);
    if (child==0) { Process p; Process::setActive(true); raise(SIGHUP); _exit(Process::isActive() ? 1 : 0); }
    int status=0; check(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
    child=fork(); check(child>=0);
    if (child==0) { Process p; raise(SIGTERM); raise(SIGTERM); _exit(1); }
    check(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==128+SIGTERM);
    child=fork(); check(child>=0);
    if (child==0) {
        Process p; Process::setActive(true);
        std::atomic<bool> sendSignal{false};
        std::thread first([&] {
            while (!sendSignal.load()) std::this_thread::yield();
            if (raise(SIGTERM)!=0) _exit(2);
        });
        sendSignal=true;
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        while (Process::isActive() && std::chrono::steady_clock::now()<deadline) std::this_thread::yield();
        first.join();
        if (Process::isActive()) _exit(3);
        // A different thread delivers the second signal; it must force exit
        // using that signal's status after the first requested a clean stop.
        std::thread second([] { raise(SIGHUP); });
        second.join();
        _exit(4);
    }
    check(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==128+SIGHUP);
    for (const auto &p: {existing,existing+".lock",alias,alias+".lock",fifoPath+".lock"}) unlink(p.c_str());
    check(rmdir(directory)==0);
}
