#ifndef __UTILS_H
#define __UTILS_H

#include <stdexcept>
#include <sstream>

//byte definition
typedef unsigned char byte;
enum ClientType {ED,GRID};
enum TurnoutState {CLOSED,THROWN,UNKNOWN};

//custom exception class
class my_exception : public std::runtime_error {
    std::string msg;
public:
    my_exception(const std::string &arg, const char *file, int line) :
    std::runtime_error(arg) {
        std::ostringstream o;
        o << file << ":" << line << ": " << arg;
        msg = o.str();
    }
    ~my_exception() throw() {}
    const char *what() const throw() {
        return msg.c_str();
    }
};
#define throw_line(arg) throw my_exception(arg, __FILE__, __LINE__);

#endif
