#include <string>
#include <regex>
#include <string>
#include <iostream>
#include "libconfig.h++"

#define S "M[TA]+\\+[SL][0-9]+<;>.*"
using namespace std;
using namespace libconfig;

int main (){

/*
   string s = "MT+L300<;>L300";
   string se = S;
   regex e = regex (se);
   cout << "start" << endl;
   if (regex_match(s,e)){
      cout << "got a match" << endl;
   }
*/

   Config cfg;
   string key,value;
    try
    {
       cfg.readFile("teste.cfg");

       cout << "key: ";
       cin >> key;
       cout <<"value: ";
       cin >> value;
       cout << "key :" << key << " value: " << value << endl;

       if (cfg.exists(key)){
            libconfig::Setting &varkey = cfg.lookup(key);
            varkey = atoi(value.c_str());
            cfg.writeFile("teste.cfg");
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
