#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

//read n bytes from fd
ssize_t read_all(int fd, void * const buf, size_t n)
{
        size_t nleft = n;
        ssize_t nread;
        char *p = buf;

        while (nleft > 0 ) {
                if ((nread = read(fd, p, nleft)) == -1) {
                        if (errno == EINTR)
                                nread = 0;
                        else
                                return -1;
                } else if (nread == 0) //EOF
                        break; 
                
                nleft -= nread;
                p += nread;
        }

        return n - nleft; //this should be zero
}


