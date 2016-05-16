//software vesion
#define SOFT_VERSION "VN2.0"
// no locos
#define ROASTER_INFO  "RL0]|["
//first info to send to ED
#define START_INFO  "VN2.0\n\rRL0]|[\n\rPPA1\n\rPTT]|[\n\rPRT]|[\n\rRCC0\n\rPW12080\n\r"
//
#define DELIM_BRACET  "]|["
//
#define DELIM_BTLT  "<;>"
//
#define DELIM_KEY  "}|{"
//
#define EMPTY_LABELS  "<;>]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[]\[\n"
//MTS3<......MTLS1<;>]\[]\[]

//regex to identify a set speed message
#define RE_SPEED "M[STA]+[SL\\*]([0-9]+)?<;>[VX]([0-9]+)?"
//regex to identify a create session message
#define RE_SESSION  "M[STA]+\\+[SL][0-9]+<;>.*"
//regex to identify a release session message
#define RE_REL_SESSION "M[STA]+\\-[SL\\*]([0-9]+)?<;>.*"
//regex to identify a create session message
#define RE_DIR "M[STA]+[SL\\*]([0-9]+)?<;>R[0-1]"
//regex to identify a query speed
#define RE_QRY_SPEED "M[STA]+[SL\\*]([0-9]+)?<;>qV"
//regex to identify a query direction
#define RE_QRY_DIRECTION "M[STA]+[SL\\*]([0-9]+)?<;>qR"
//regex to identify a query direction
#define RE_FUNC "M[STA]+[SL\\*]([0-9]+)?<;>F[0-9]+"
