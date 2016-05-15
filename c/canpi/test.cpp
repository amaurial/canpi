#include <string>
#include <regex>
#include <string>
#include <iostream>

#define S "M[TA]+\\+[SL][0-9]+<;>.*"
using namespace std;


int main (){
   string s = "MT+L300<;>L300";
   string se = S;
   regex e = regex (se);
   cout << "start" << endl;
   if (regex_match(s,e)){
      cout << "got a match" << endl;
   }
}
