#include "Turnout.h"

Turnout::Turnout(log4cpp::Category *logger)
{
    //ctor
    closed_code = 2;
    throw_code = 4;
    this->logger = logger;
}

Turnout::~Turnout()
{
    //dtor
}

int Turnout::load(string fileName){
    ifstream infile(fileName);
    string key;
    int value;

    logger->debug("Turnout opening file %s",fileName.c_str());

    while (infile >> key >> value){
        logger->debug("Adding turnout %s %d",key.c_str(),value);
        addTurnout(key,value);
    }
    infile.close();
    return this->size();
}

string Turnout::getStartInfo(){
    stringstream ss;
    ss << "PTT";
    ss << DELIM_BRACET;
    ss << "Turnouts";
    ss << DELIM_KEY;
    ss << "Turnout";
    ss << DELIM_BRACET;
    ss << "Closed";
    ss << DELIM_KEY;
    ss << closed_code;
    ss << DELIM_BRACET;
    ss << "Thrown";
    ss << DELIM_KEY;
    ss << throw_code;
    //PTT]\[Turnouts}|{Turnout]\[Closed}|{2]\[Thrown}|{4

    if (turnouts_code.size() == 0){
        ss << "\n";
        logger->debug("Turnout start info %s",ss.str().c_str());
        return ss.str();
    }
    ss << "\nPTL";
    std::map<string,int>::iterator it = turnouts_code.begin();
    while(it != turnouts_code.end())
    {
        ss << DELIM_BRACET;
        ss << "MT+";
        ss << it->second;
        ss << ";-";
        ss << it->second;
        ss << DELIM_KEY;
        ss << it -> first;
        ss << DELIM_KEY;
        ss << closed_code;
        it++;
    }
    logger->debug("Turnout start info %s",ss.str().c_str());

    return ss.str();
}

string Turnout::getTurnoutMsg(int tcode){
    int v = closed_code;
    if (getTurnoutState(tcode) == TurnoutState::THROWN){
        v = throw_code;
    }
    stringstream ss;
    ss << "PTA";
    ss << v;
    ss << "MT+";
    ss << tcode;
    ss << ";-";
    ss << tcode;
    logger->debug("Turnout message %s",ss.str().c_str());
    return ss.str();
}

int Turnout::getCloseCode(){
    return closed_code;
}

int Turnout::getThrownCode(){
    return throw_code;
}

void Turnout::CloseTurnout(int tcode){
    if (turnouts.count(tcode) > 0){
        turnouts[tcode] = TurnoutState::CLOSED;
    }
}

void Turnout::ThrownTurnout(int tcode){
    if (turnouts.count(tcode) > 0){
        turnouts[tcode] = TurnoutState::THROWN;
    }
}

TurnoutState Turnout::getTurnoutState(int tcode){
    if (turnouts.count(tcode) > 0){
        return turnouts[tcode];
    }
    return TurnoutState::UNKNOWN;
}

int Turnout::getTurnoutCode(string tname){
    if (turnouts_code.count(tname) > 0){
        return turnouts_code[tname];
    }
    return 0;
}

int Turnout::size(){
    return turnouts.size();
}

void Turnout::addTurnout(string tname,int tcode){
    if (turnouts.count(tcode)==0){
        turnouts_code.insert(pair<string,int>(tname,tcode));
        turnouts.insert(pair<int,TurnoutState>(tcode,TurnoutState::UNKNOWN));
    }
}

bool Turnout::exists(int tcode){
    if (turnouts.count(tcode)>0){return true;}
    return false;
}
