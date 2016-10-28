/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <csignal>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>

#include <sys/stat.h>

#include "fifo.hpp"

int Fifo::enable(std::string output) {
	output_ = output;

	// ignore SIGPIPE, else program terminates on unsuccessful write()
	struct sigaction ignore;
	ignore.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &ignore, nullptr);

    struct stat statInfo;
    if (stat(output_.c_str(), &statInfo)!=0) {
        if (mkfifo(output_.c_str(), 0644)) {
            std::cerr << "Error creating FIFO." << std::endl;
            return 1;
	    }
	    std::cerr << "FIFO: " << output << " has been created." << std::endl;
    } else {
        if (!S_ISFIFO( statInfo.st_mode ))  {
            std::cerr << output_.c_str() << " exists, but is not fifo. Cannot use it." << std::endl;
            return 1;
        }
    }


    //Check that we can access it.
    fd_ = open(output_.c_str(), O_WRONLY | O_NONBLOCK );
    if(fd_ == -1) {
        if( errno == EACCES ) {
            throw output_error( "Do not have permission to access fifo." );
        } else if ( errno != ENXIO ) {
            throw output_error( "Cannot open fifo." );
        }
    } else {
        this->setOpen(true);
    }
	return 0;
}

void Fifo::disable() {
    if (fd_) {
        if (fd_ != -1) {
            close(fd_);
        }
        fd_ = -1;
        //We no longer delete the fifo, add this back
        //if we want to add this back.
        //unlink(output_.c_str());
    }
    paused_=true;
    open_=false;
    neverOpened_=true;
}

void Fifo::setOpen(bool open) {
    if( (open_ != open ) || neverOpened_) {
        if( open ) {
	        std::cerr << "FIFO " << output_ << ": Opened by other program." << std::endl;
            neverOpened_=false;
            paused_=false;
        } else {
            if( !neverOpened_) {
                std::cerr << "FIFO " << output_ << ": Closed by other program." << std::endl;
            }
            std::cerr << "FIFO " << output_ << ": Waiting for other program to open it." << std::endl;
            neverOpened_=false;
            paused_=true;
        }
    }
    open_=open;
}

void Fifo::setPaused(bool paused) {
    if (paused_ != paused ) {
        if (paused) {
	        std::cerr << "FIFO " << output_ << ": Other program has stopped reading from it (paused)." << std::endl;
        } else {
	        std::cerr << "FIFO " << output_ << ": Other program has resumed reading from it (unpaused)." << std::endl;
        }
    }
    paused_=paused;
}

void Fifo::output(std::vector<unsigned char> *buffer) {
    if(fd_ == -1) {
        fd_ = open(output_.c_str(), O_WRONLY | O_NONBLOCK );
    }
    if(fd_ == -1) {
        this->setOpen(false);
        if( errno == EACCES ) {
            throw output_error( "Do not have permission to access fifo." );
        } else if ( errno != ENXIO ) {
            throw output_error( "Error opening fifo." );
        }
        return; //otherwise return, and let buffer run.
    }
    this->setOpen(true);

    unsigned char *current=buffer->data();
    unsigned char *end=current+buffer->size();

    while(current < end) {
        struct pollfd descriptor = { fd_, POLLOUT, 0 };

        int timeout;
        if( !this->paused_ ) {
            timeout=5000; //5 seconds is more than reasonable to wait for remote.
        } else {
            timeout=0; //If not unpaused dump immediately.
        }
        int value=poll( &descriptor, 1, timeout ); 
        switch( value ) {
            case 0: //We have a timeout
                this->setPaused(true);
                current=end; //Dump buffer
                break;
            case 1: //We have an event
                this->setPaused(false);
                //This is either POLLOUT or error. If error
                //we'll let write report it.
                break;
            case -1: //We have an error.
                throw output_error( "Output fifo error." );
                break;
        }
        if( current < end ) {
            int size=write(fd_, current, end-current);
            if( size == -1 ) {
                if( errno == EPIPE ) {
                    close(fd_);
                    fd_=-1;
                    this->setOpen(false);
                } else {
                    throw output_error( "Output fifo error." );
                }
            }
            current+=size;
        }
    }
}

Fifo::Fifo() {
	fd_ = -1;
    disable();
}

Fifo::~Fifo() {
	disable();
}
