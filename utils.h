#ifndef __UTILS_H
#define __UTILS_H

#include <stdexcept>
#include <sstream>
#include <string>
#include "libconfig.h++"

using namespace libconfig;
using namespace std;

//byte definition
typedef unsigned char byte;
enum ClientType {ED,GRID};
enum TurnoutState {CLOSED,THROWN,UNKNOWN};

#define INTERROR 323232


/**
* Save configuration item
**/
/*
int saveConfig(string key,string val){
    Config cfg;
    string filename="canpi.cfg";
    try
    {
       cfg.readFile(filename.c_str());

       if (cfg.exists(key)){
            libconfig::Setting &varkey = cfg.lookup(key);
            varkey = val;
            cfg.writeFile(filename.c_str());
       }

    }
    catch(const FileIOException &fioex)
    {
        std::cerr << "File I/O error" << std::endl;
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
    }
}
*/
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
