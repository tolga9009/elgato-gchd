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
#include <fstream>
#include <stdexcept>
#include <thread>
static void check(bool ok) { if (!ok) throw std::runtime_error("output regression"); }
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
    pid_t child=fork(); check(child>=0);
    if (child==0) { Process p; Process::setActive(true); raise(SIGHUP); _exit(Process::isActive() ? 1 : 0); }
    int status=0; check(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
    child=fork(); check(child>=0);
    if (child==0) { Process p; raise(SIGTERM); raise(SIGTERM); _exit(1); }
    check(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==128+SIGTERM);
    for (const auto &p: {existing,existing+".lock",alias,alias+".lock",fifoPath+".lock"}) unlink(p.c_str());
    check(rmdir(directory)==0);
}
