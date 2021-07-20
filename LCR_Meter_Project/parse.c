/*
 * parse.c
 *
 *  Created on: May 4, 2021
 *      Author: ktmg8
 */
#include <parse.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"


void getsUart0(USER_DATA* data)
{
    int count; // count the no of characters received from the terminal
    char c; // char received from UART
    bool loop = true;
    count = 0;
    while(loop)
    {
        c = getcUart0(); // get a char from UART0
        if(c == 8 | c ==127 ) // check if the char received is a backspace
        {
            if(count > 0)
            {
                count--;
            }
        }
        else // char received is not a backspace
        {
            if(c==13) // char is a carriage return <ENTER KEY>
            {
                data->buffer[count] = NULL; // indicate the end of the string
                return; // return from the function
            }
            else // char is not an enter key
            {
                if(c >= 32) // char c is a printable character
                {
                    data->buffer[count] = c; // store character in the buffer
                    count++; // increment the count
                    if(count == MAX_CHARS) // no more room in the buffer
                    {
                        // terminate the string and return from the function
                        data->buffer[count] = NULL;
                        return;
                    }
                }
            }
         }
    }
return; // never gets out of the while loop
}

void parseFields(USER_DATA* data)
{
    data->fieldCount = 0;
    bool transition = true;
    //find index and type of the important part excluding delimiters in the buffer
    uint8_t i,j;
    j=0;
    for(i=0;i<MAX_CHARS+1;i++)
    {
        //return if <ENTER KEY>/NULL is found
        if(data->buffer[i] == 13 || data->buffer[i] == NULL)
        {
            return;
        }

        //char is an alphabet
        if((data->buffer[i] >= 'a' && data->buffer[i] <= 'z') || (data->buffer[i] >= 'A' && data->buffer[i] <= 'Z'))
        {
            if(transition) //if last char was a delimiter
            {
                data->fieldPosition[j] = i;
                data->fieldType[j] = 'a';
                (data->fieldCount)++;
                transition = false;
                j++;
            }
        }
        //char is a number
        else if((data->buffer[i] >= '0' && data->buffer[i] <= '9'))
        {
            if(transition) //if last char was a delimiter
            {
                data->fieldPosition[j] = i;
                data->fieldType[j] = 'n';
                (data->fieldCount)++;
                transition = false;
                j++;
            }
        }
        //char is a delimiter
        else
        {
            data->buffer[i] = NULL; //convert delimiter to NULL
            transition = true;
        }
        //return if fieldCount = MAX_FIELDS
        if(data->fieldCount == MAX_FIELDS)
        {
            return;
        }
    }
}

//return the value of a field requested if the field number is in range
char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
    if(fieldNumber <= data->fieldCount)
    {
        return &(data->buffer[data->fieldPosition[fieldNumber]]);
    }
    else
    {
        return NULL;
    }
}

//return a pointer to the field requested if the field number is in range and the field type is numeric
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    if(fieldNumber <= data->fieldCount && (data->buffer[data->fieldPosition[fieldNumber]] > '0' && data->buffer[data->fieldPosition[fieldNumber]] > '9'))
    {
        return data->buffer[data->fieldPosition[fieldNumber]];
    }
    else
    {
        return 0;
    }
}

//returns true if the command matches the first field and the number of arguments (excluding the command field) is greater than or equal to the requested number of minimum arguments.
bool isCommand(USER_DATA* data, const char strCommand[],uint8_t minArguments)
{
    if((data->fieldCount)-1 >= minArguments)
    {
        int i;
        i = 0;
        uint8_t indexofbufferchar = data->fieldPosition[0];
        while(data->buffer[indexofbufferchar] != '\0' || strCommand[i] !='\0')
        {
            //putcUart0(strCommand[i]);
            //putcUart0(data->buffer[indexofbufferchar]);
            if(strCommand[i] == data->buffer[indexofbufferchar])
            {
                i++;
                indexofbufferchar++;
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
    return true;
}
