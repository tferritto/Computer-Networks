FILE *logFile;
time_t _LOG_CT;
char* _LOG_STR;
#define log(format,...) time(&_LOG_CT);\
_LOG_STR = asctime(localtime(&_LOG_CT));\
_LOG_STR[strlen(_LOG_STR)-1] = 0;\
printf("%s -- ",_LOG_STR);\
printf(format,##__VA_ARGS__);\
pthread_mutex_lock(&lock);\
fprintf(logFile,"%s -- ",_LOG_STR);\
fprintf(logFile,format,##__VA_ARGS__);\
fflush(logFile);\
pthread_mutex_unlock(&lock)