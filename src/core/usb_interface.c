/**
  ******************************************************************************
  * @file    usb_interface.c
  * @brief   This file provides code used to interface with python through USB.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usb_interface.h"
#include "stm32f4xx_it.h"
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include <stdarg.h>
#include <stdio.h>
#include "commands.h"
#include "dwt_iqr.h"
#include "cmsis_os.h"
/* Typedefs ------------------------------------------------------------------*/
typedef enum {INT=1, STR=2, BOOL=3, FLOAT=4, BYTES=5} FieldTypes;

/* Variables -----------------------------------------------------------------*/
// VARIABLE FIELD NAMES MUST BE LESS THAN 10 CHARACTERS
static int command_number = -1;
static uint8_t retry_count = 0;
static char temp_receive_buffer[USB_BUFFER_SIZE];

static const char *c00_fields[1];             // No fields. Empty array of size 1
static const FieldTypes c00_types[1];         // No fields. Empty array of size 1
static const int c00_num_fields = 0;

static const char *c01_fields[1];             // No fields. Empty array of size 1
static const FieldTypes c01_types[1];         // No fields. Empty array of size 1
static const int c01_num_fields = 0;

static const char *c02_fields[1];             // No fields. Empty array of size 1
static const FieldTypes c02_types[1];         // No fields. Empty array of size 1
static const int c02_num_fields = 0;

static const char *c03_fields[] = {"test_int", "test_str", "test_bool","test_flt", "test_byte"};          
static const FieldTypes c03_types[] = {INT, STR, BOOL, FLOAT, BYTES};       
static const int c03_num_fields = 5;

static const char *c04_fields[] = {"toggle"}; 
static const FieldTypes c04_types[] = {BOOL};
static const int c04_num_fields = 1;

static const char *c05_fields[] = {"target", "targ_meas", "mult_twr"}; 
static const FieldTypes c05_types[] = {INT, BOOL, INT};
static const int c05_num_fields = 3;

static const char *c06_fields[] = {"data"}; 
static const FieldTypes c06_types[] = {BYTES};
static const int c06_num_fields = 1;

static const char *c07_fields[1];     // No fields. Empty array of size 1
static const FieldTypes c07_types[1]; // No fields. Empty array of size 1
static const int c07_num_fields = 0;

static const char **all_command_fields[] = {
    c00_fields,
    c01_fields,
    c02_fields,
    c03_fields,
    c04_fields,
    c05_fields,
    c06_fields,
    c07_fields,
};

static const FieldTypes *all_command_types[] = {
    c00_types,
    c01_types,
    c02_types,
    c03_types,
    c04_types,
    c05_types,
    c06_types,
    c07_types,
};

static const int (*all_command_funcs[])(IntParams *, FloatParams *, BoolParams *, StrParams *, ByteParams *) = {
    c00_set_idle,
    c01_get_id,
    c02_reset,
    c03_do_tests,
    c04_toggle_passive,
    c05_initiate_twr,
    c06_broadcast,
    c07_get_max_frame_len,
};

static const int all_command_num_fields[] = {
    c00_num_fields,
    c01_num_fields,
    c02_num_fields,
    c03_num_fields,
    c04_num_fields,
    c05_num_fields,
    c06_num_fields,
    c07_num_fields,
};


/* TODO: can below be made local variables if defined in
 parseMessageIntoHashTables and passed to deleteOldParams() ? Or would we lose
 the handle to these variables after the function exits, and they would exist in 
 the hash tables without any way for us to free the memory. 
*/
static IntParams *msg_ints; 
static FloatParams *msg_floats;
static BoolParams *msg_bools;
static StrParams *msg_strs;
static ByteParams *msg_bytes;

/* Private Functions ----------------------------------------------------------*/
static char* parseMessageIntoHashTables(char *msg);
static void deleteOldParams();

/*! ----------------------------------------------------------------------------
 * Function: readUsb()
 *
 * The purpose of this function is to read the oldest message at the USB port,
 * and remove the message from the USB-receive buffer.
 * Additionally, this function updates the status and parameters associated with 
 * the finite-state machine.
 * 
 */
void readUsb(){
  
    decaIrqStatus_t stat;
    stat = decamutexon();
    char *idx_start;
    char *idx_end;
    
    idx_start = strstr(CdcReceiveBuffer, "C"); // address where to start reading the message
    if (idx_start > 0)
    {
        /* ----------------------- PROCESS COMMUNICATED INFORMATION ----------------------- */
        memset(temp_receive_buffer, '\0', USB_BUFFER_SIZE); // clear temp buffer

        // Copy receive buffer into temp buffer.
        memcpy(temp_receive_buffer, CdcReceiveBuffer + 1, USB_BUFFER_SIZE - 1);

        idx_end = parseMessageIntoHashTables(temp_receive_buffer);   
        if (*idx_end != '\r'){
            usb_print("ERROR parsing message from USB.");
            osDelay(1);
        }
        idx_start = strstr(temp_receive_buffer, "C"); // address where to start reading the message
        uint16_t len = idx_end - idx_start; // Removing the first entry 

        /* ------------------------------ UPDATE THE BUFFER ------------------------------ */
        memset(temp_receive_buffer, '\0', USB_BUFFER_SIZE); // clear temp buffer
        memcpy(temp_receive_buffer, CdcReceiveBuffer + len + 2, USB_BUFFER_SIZE - len - 2); // copy unread buffer into temp memory
        memset(CdcReceiveBuffer + 1, '\0', USB_BUFFER_SIZE - 1); // clear the buffer
        memcpy(CdcReceiveBuffer + 1, temp_receive_buffer, USB_BUFFER_SIZE - len - 1); // move data back to buffer
        CdcReceiveBuffer[0] = CdcReceiveBuffer[0] - len - 1; // adjust where to continue writing
    }
    
    decamutexoff(stat);
    
    /* 
    NOTE: currently ALL commands are endlessly retried until they return a 
    value of 1, or until a new command sent over USB overwrites. This might not 
    be the desired behavior for some future functions, where they might just 
    want to report a failure and not retry. 

    A simple solution is to extend the possible return values of the commands:
    -1: Fail, retry me.
    0: Fail
    1: success
    */

    // Call if a valid command number was detected
    int num_commands = sizeof(all_command_funcs) / sizeof(all_command_funcs[0]);
    if (retry_count > 10){
        command_number = -1;
        retry_count = 0;
    }
    else if (command_number >= 0 && command_number <= num_commands){
        bool success;
        success = (*all_command_funcs[command_number])(
            msg_ints,
            msg_floats,
            msg_bools,
            msg_strs,
            msg_bytes);

        if (success){
            command_number = -1; // stops entering the above if statement
        }
        else{
            retry_count += 1; 
        }
    }
} // end readUsb()


/*! -----------------------------------------------------------------------------------------
 * @fn: parseMessageIntoHashTables()
 *
 * This is the main message parsing function where the message is split into 
 * its corresponding datatypes.
 *
 */
char * parseMessageIntoHashTables(char *msg)
{

    const FieldTypes *msg_types; // Pointer to array
    const char **msg_fields;     // Pointer to array of pointers
    int num_fields;
    
    char *current_pt = msg ;

    /* Find the next '|' delimiter. Usually, this will denote where the next
    field is, but will not be true in general when the field type is float 
    or bytes. 
    
    Warning: following line
    if the USB buffer is full, there might not be a NULL terminator.
    */ 
    char *next_sep_pt = strchr(current_pt, '|'); // 

    /* Find the next '\r' message terminator. Again, these could randomly appear
    in float or byte fields, but for those we know the length of these fields
    and hence we know whether '\r' or '|' is actually significant or not. 
    
    Warning: following line
    if the USB buffer is full, there might not be a NULL terminator.
    */
    char *next_end_pt = strchr(current_pt, '\r'); 
   
    // Find which of '|' or '\r' occurs first.
    char *next_pt;
    if (next_sep_pt == NULL && next_end_pt == NULL){
        usb_print("Did you just send a USB message without any | or \\r?");
        return NULL;
    }
    else if (next_sep_pt == NULL){
        next_pt = next_end_pt;
    }
    else if (next_end_pt == NULL){
        next_pt = next_sep_pt;
    }
    else if (next_sep_pt < next_end_pt){
        next_pt = next_sep_pt;
    }
    else if (next_end_pt < next_sep_pt)
    {
        next_pt = next_end_pt;
    }
    else
    {
        usb_print("Charles wrote some shitty code.");
        return NULL;
    }

    // delete (free memory) of all old params in the hash tables
    deleteOldParams();

    // TODO: add some checks here on validity of prefix.
    // the first 3 bytes of "msg" should always be of the form "Cxx"
    char command_number_str[3];
    strncpy(command_number_str, current_pt+1, 2 * sizeof(char));
    //char command_number_str[] = {*current_pt+1, *current_pt+2, '\0'};
    command_number = atoi(command_number_str + 1);

    // Get the relevant info about the command.
    msg_types = all_command_types[command_number];
    msg_fields = all_command_fields[command_number];
    num_fields = all_command_num_fields[command_number];
    current_pt = next_pt + 1;

    int i;
    FieldTypes type;
    for (i=0; i<num_fields; ++i)
    {     
        type = msg_types[i];    
        if (type != FLOAT && type != BYTES){
            // next_pt only required for for non-float/byte fields.

            next_sep_pt = strchr(current_pt, '|'); 
            next_end_pt = strchr(current_pt, '\r'); 
            if (next_sep_pt == NULL && next_end_pt == NULL){
                /* We sometimes enter this condition when the next field is
                either a float or byte. Since those fields could contain the
                string terminator '\0', then this will prematurely exit the 
                strchr() functions.

                But since i just wrapped this in another if statement, we 
                should never actually enter this.
                */
                usb_print("No next key character found.");
                osDelay(1);
            }
            else if (next_sep_pt == NULL){
                next_pt = next_end_pt;
            }
            else if (next_end_pt == NULL){
                next_pt = next_sep_pt;
            }
            else if (next_sep_pt < next_end_pt){
                next_pt = next_sep_pt;
            }
            else if (next_end_pt < next_sep_pt)
            {
                next_pt = next_end_pt;
            }
            else
            {
                usb_print("Impossible situation with next_pt");
                osDelay(1);
                return NULL;
            }
        }
        
        switch (type)
        {
        case INT:
        {
            IntParams *param_temp;

            param_temp = malloc(sizeof(IntParams));
            if (param_temp == NULL)
            {
                MemManage_Handler();
            } // if the memory has not been allocated, interrupt operations

            strcpy(param_temp->key, msg_fields[i]);

            /* Everything up to the next delimiter (located at next_pt), is
            assumed to be a big int represented as a character array */
            uint8_t len = next_pt - current_pt;
            char *int_as_string = calloc(len + 1, sizeof(char));
            memcpy(int_as_string, current_pt, len * sizeof(char));
            param_temp->value = atoi(int_as_string);

            HASH_ADD_STR(msg_ints, key, param_temp);
            current_pt += len + 1;
            free(int_as_string);
            break;
        }
        case STR:
        {
            /* TODO: strings cannot contain '|' or '\r', and this will cause a
            serious error. Is there a way to safeguard against this on the C
            side? Probably not, as we wont know whether its actually the end
            of the string. This can easily be done on the python side though. */
            StrParams *param_temp;

            param_temp = malloc(sizeof(StrParams));
            if (param_temp == NULL)
            {
                MemManage_Handler();
            } // if the memory has not been allocated, interrupt operations

            strcpy(param_temp->key, msg_fields[i]);

            /* Everything up to the next delimiter (located at next_pt), is
            assumed to be part of the string. */
            uint16_t len = next_pt - current_pt;
            strncpy(param_temp->value, current_pt, len * sizeof(char));

            HASH_ADD_STR(msg_strs, key, param_temp);
            current_pt += len + 1;
            break;
        }
        case BOOL:
        {
            BoolParams *param_temp;

            param_temp = malloc(sizeof(BoolParams));
            if (param_temp == NULL)
            {
                MemManage_Handler();
            } // if the memory has not been allocated, interrupt operations

            strcpy(param_temp->key, msg_fields[i]);

            uint8_t len = next_pt - current_pt;
            if (len != 1){
                usb_print("Error in USB message parsing. Was a BOOL specified as '1' or '0'?");
                osDelay(1);
            }
            char *bool_as_string = calloc(len + 1,  sizeof(char));
            memcpy(bool_as_string, current_pt, len * sizeof(char));
            param_temp->value = atoi(bool_as_string);

            HASH_ADD_STR(msg_bools, key, param_temp);
            free(bool_as_string);
            current_pt += len + 1;
            break;
        }
        case FLOAT:
        {   /* STM32 processors are all LITTLE-ENDIAN. This means, to pack a 
            single-precision float variable into a compatible array of 4 bytes, 
            one must use the following on the python side:

             float_bytes = struct.pack("<f", x)

            where x is the python float.

            A single-precision float requires 4 bytes. Do not send doubles
            (8 bytes) or you will probably have a hard fault.

            To send doubles, a dedicated data type is probably best.
            */

            FloatParams *param_temp;
            uint8_t len = 4;

            param_temp = malloc(sizeof(FloatParams));
            if (param_temp == NULL)
            {
                MemManage_Handler();
            } // if the memory has not been allocated, interrupt operations

            strcpy(param_temp->key, msg_fields[i]);
            memcpy(&(param_temp->value), current_pt, len);

            HASH_ADD_STR(msg_floats, key, param_temp);

            /* For the float and byte fields, the search for the seperator 
            or terminator characters was possibly wrong as they could have 
            been part of the field itself. Hence, now that we are through the
            field, find the location of the next terminator just in case this
            was the last field. */
            current_pt += len;
            next_end_pt = strchr(current_pt, '\r'); 
            current_pt += 1;

            break;
        }
        case BYTES:
        {
            /* Processing arbitary bytes is a little special. It is slightly
            complicated because the byte array could contain the reserved
            characters '|', which is our field seperator, and '\0' which is
            the C string terminator. Therefore, we have no choice to
            also specify the length of the byte array, so that it can be parsed
            properly. We store this length as a 16-bit unsigned integer and it
            is found as the first two bytes of the byte array */
            ByteParams *param_temp;

            param_temp = malloc(sizeof(ByteParams));
            if (param_temp == NULL)
            {
                MemManage_Handler();
            } // if the memory has not been allocated, interrupt operations
            strcpy(param_temp->key, msg_fields[i]);
            
            // Extract the length of remaining data (in number of bytes) 
            // as the first two bytes.
            uint16_t len;
            memcpy(&len, current_pt, sizeof len);
            param_temp->len = len;

            current_pt = current_pt + 2;
            memset(&(param_temp->value[0]), 0, USB_BUFFER_SIZE);
            memcpy(&(param_temp->value[0]), current_pt, len);

            HASH_ADD_STR(msg_bytes, key, param_temp);

            /* For the float and byte fields, the search for the seperator 
            or terminator characters was possibly wrong as they could have 
            been part of the field itself. Hence, now that we are through the
            field, find the location of the next terminator just in case this
            was the last field. */
            current_pt += len;
            next_end_pt = strchr(current_pt, '\r'); 
            current_pt += 1;
            break;
        }
        default:
            break;
        }

    }
    if (next_end_pt == 0){
        // Should never get here, but you never know.
        usb_print("Failed to detect end of message. Trying my best.");
        next_end_pt = current_pt;
    }
    return next_end_pt;
} // end parseMessageIntoHashTables()



void deleteOldParams() {

    /* Delete int params */
    IntParams *current_int, *tmp_int;

    HASH_ITER(hh, msg_ints, current_int, tmp_int) {
        HASH_DEL(msg_ints, current_int);  /* delete; advances to next param */
        free(current_int);    
    }

    /* Delete str params */
    StrParams *current_str, *tmp_str;

    HASH_ITER(hh, msg_strs, current_str, tmp_str) {
        HASH_DEL(msg_strs, current_str);  /* delete; advances to next param */
        free(current_str);    
    }

    /* Delete bool params */
    BoolParams *current_bool, *tmp_bool;

    HASH_ITER(hh, msg_bools, current_bool, tmp_bool) {
        HASH_DEL(msg_bools, current_bool);  /* delete; advances to next param */
        free(current_bool);    
    }

    /* Delete float params */
    FloatParams *current_float, *tmp_float;

    HASH_ITER(hh, msg_floats, current_float, tmp_float) {
        HASH_DEL(msg_floats, current_float);  /* delete; advances to next param */
        free(current_float);    
    }

    /* Delete byte params */
    ByteParams *current_bytes, *tmp_bytes;

    HASH_ITER(hh, msg_bytes, current_bytes, tmp_bytes)
    {
        HASH_DEL(msg_bytes, current_bytes); /* delete; advances to next param */
        free(current_bytes);
    }
}