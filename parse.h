/*
 * parse.h
 *
 *  Created on: May 4, 2021
 *      Author: ktmg8
 */


#ifndef PARSE_H_
#define PARSE_H_
#include <stdint.h>
#include <stdbool.h>
#define MAX_CHARS 80
#define MAX_FIELDS 5 //max args
#define DELIMITERS_SIZE 4

//structure that holds user data
typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;//argument count
    uint8_t fieldPosition[MAX_FIELDS]; //index
    char fieldType[MAX_FIELDS];//type
}USER_DATA;

void getsUart0(USER_DATA* data);
void parseFields(USER_DATA* data);
char* getFieldString(USER_DATA* data, uint8_t fieldNumber);
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber);
bool isCommand(USER_DATA* data, const char strCommand[],uint8_t minArguments);
#endif /* PARSE_H_ */
