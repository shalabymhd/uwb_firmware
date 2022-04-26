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
#include "usb_device.h"
/* Typedefs ------------------------------------------------------------------*/
typedef enum {INT=1, STR=2, BOOL=3, FLOAT=4, BYTES=5} FieldTypes;

/* Variables -----------------------------------------------------------------*/
// VARIABLE FIELD NAMES MUST BE LESS THAN 10 CHARACTERS
static int command_number = -1;
static uint8_t retry_count = 0;

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

static osMailQDef(MsgBox, USB_QUEUE_SIZE, UsbMsg); 
static osMailQId MsgBox;             

static uint8_t usb_rx_buffer[USB_BUFFER_SIZE]; 
static uint32_t buffer_len;
static uint8_t temp_buffer[USB_BUFFER_SIZE];

/* Private Functions ----------------------------------------------------------*/
static char* parseMessageIntoHashTables(char *msg);
static void deleteOldParams();
static char* getNextKeyChar(char*);
static void loadBuffer(void);
static void slideBuffer(uint8_t*);

/**
 * @brief USB interface initialization procedure. Gets called once on startup. 
 * Currently used to initialize the USB interrupt queue object.
 * 
 */
void interfaceInit(void){
  MsgBox = osMailCreate(osMailQ(MsgBox), NULL);  // create msg queue
  memset(usb_rx_buffer, 0, USB_BUFFER_SIZE); 
  buffer_len = 0;
}

/**
 * @brief Gets the handle to the underlying message queue that is used to 
 * transfer data from the USB reception interrupt to other threads.
 * 
 * @return USB message queue handle 
 */
osMailQId getMailQId(void){
  return MsgBox;
}

/**
 * @brief Consumes all items on the USB message interrupt queue, and 
 * concats them to a large USB message buffer, which also acts like a queue.
 * 
 */
void loadBuffer(void){
    osMailQId MsgBox = getMailQId();// Get queue handle
    osEvent evt;
    UsbMsg *msg_ptr;

    evt = osMailGet(MsgBox, 0);  // Get message on queue. No waiting.
    while (evt.status == osEventMail) {
        msg_ptr = evt.value.p;

        if (buffer_len + msg_ptr-> len > USB_BUFFER_SIZE){
            usb_print("USB Buffer full! Message rejected to prevent overflow.");
            osMailFree(MsgBox, msg_ptr); // IMPORTANT: free message memory
            break;
        }
        else{
            memcpy(usb_rx_buffer + buffer_len, msg_ptr->msg, msg_ptr->len);
            buffer_len += msg_ptr->len;
            osMailFree(MsgBox, msg_ptr); // IMPORTANT: free message memory
            osDelay(5); // Give a bit of time for the queue to fill up
            evt = osMailGet(MsgBox, 0); 
        }
    }

}
/**
 * @brief Consumes part of the buffer and slides the remaining contents up.
 * 
 * @param idx Final index of chunk of buffer that is to be consumed.
 */
void slideBuffer(uint8_t* idx){
    uint8_t len = idx - &usb_rx_buffer[0] + 1;

    // copy REMAINING content into temp buffer
    memcpy(temp_buffer, usb_rx_buffer + len, buffer_len - len);

    // zero-out contents of the buffer
    memset(usb_rx_buffer, 0, buffer_len);
    
    // load remaining contents back in
    memcpy(usb_rx_buffer, temp_buffer, buffer_len - len); 

    buffer_len -= len;
}


/**
 * @brief  The core USB message processing function and command executor.
 * 
 */
void readUsb(){
  
    decaIrqStatus_t stat;
    stat = decamutexon();

    uint8_t* msg_start;
    char* msg_end; // TODO: to be consistent, change everything to uint8_t?

    // Load buffer from interrupt message queue.
    loadBuffer();

    /* address where to start reading the message. Search for 'C' char as
    beginning of official message */
    msg_start =  memchr(usb_rx_buffer, 'C', USB_BUFFER_SIZE); 

    while (msg_start != NULL){

        msg_end = parseMessageIntoHashTables((char*) msg_start); 
        

        if (*msg_end != '\r'){
            slideBuffer((uint8_t*) msg_end);
            usb_print("ERROR parsing message from USB.");
            command_number = -1; // Dont attempt executing a command.
            break;
        }

        slideBuffer((uint8_t*) msg_end);
        // Go to next message. Dont search past end of buffer.
        msg_start = memchr(usb_rx_buffer, 'C', USB_BUFFER_SIZE); 
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
        // Give up re-trying after 10 attempts.
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
            msg_bytes); // Call the command function

        if (success){
            command_number = -1; // stops entering the above if statement
            retry_count = 0;
        }
        else{
            retry_count += 1; 
        }
    }
} // end readUsb()


/**
 * @brief This is the main message parsing function where the message is split into 
 * its corresponding datatypes.
 * 
 * @param msg a pointer to the very beginning of the message, 
 * so it should point to a 'C' 
 * @return char* a pointer to the last character of the message that was just 
 * parsed.
 */
char * parseMessageIntoHashTables(char *msg)
{

    const FieldTypes *msg_types; // Pointer to array
    const char **msg_fields;     // Pointer to array of pointers
    int num_fields;
    
    // Current pointer. will be used as working variable throughout parsing.
    char *current_pt = msg ;

    // delete (free memory) of all old params in the hash tables
    deleteOldParams();

    // the first 3 bytes of "msg" should always be of the form "Cxx"
    // Hence read them directly and convert to actual integer.
    char command_number_str[] = {*(current_pt+1), *(current_pt+2), '\0'};
    command_number = atoi(command_number_str);

    // Get the relevant info about the command.
    msg_types = all_command_types[command_number];
    msg_fields = all_command_fields[command_number];
    num_fields = all_command_num_fields[command_number];
    current_pt += 3; // Move to the first field.

    int i;
    FieldTypes type;
    char *next_pt;
    for (i=0; i<num_fields; ++i)
    {     
        type = msg_types[i];    
        
        /* At this point, current pointer will be at a '|', so move forward by
        1 to be at the first char of the field.*/
        current_pt += 1; 
        
        /* Each case of this switch statement must move the pointer to the next
        delimiter */
        switch (type)
        {
        case INT:
        {
            IntParams *param_temp;

            param_temp = malloc(sizeof(IntParams));
            if (param_temp == NULL)
            {
                MemManage_Handler(); /* TODO: should handle this in a better way,
                                              as well as in other cases. */
            } // if the memory has not been allocated, interrupt operations

            strcpy(param_temp->key, msg_fields[i]);

            // Get length of field
            next_pt = getNextKeyChar(current_pt);
            uint8_t len = next_pt - current_pt;

            // Extract into struct
            char *int_as_string = calloc(len + 1, sizeof(char));
            memcpy(int_as_string, current_pt, len * sizeof(char));
            param_temp->value = atoi(int_as_string);

            HASH_ADD_STR(msg_ints, key, param_temp);
            current_pt = next_pt;
            free(int_as_string);
            break;
        }
        case STR:
        {
            /* Strings cannot contain '|' or '\r', or this will cause an 
            error. */
            StrParams *param_temp;

            param_temp = calloc(1, sizeof(StrParams));
            if (param_temp == NULL)
            {
                MemManage_Handler();
            } // if the memory has not been allocated, interrupt operations

            strcpy(param_temp->key, msg_fields[i]);

            // Get length of field
            next_pt = getNextKeyChar(current_pt);
            uint16_t len = next_pt - current_pt;

            memcpy(param_temp->value, current_pt, len * sizeof(char));

            HASH_ADD_STR(msg_strs, key, param_temp);
            current_pt = next_pt;
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

            // Get length of field
            next_pt = getNextKeyChar(current_pt);
            uint8_t len = next_pt - current_pt; // Should ALWAYS be 1.

            // Extract into struct
            char *bool_as_string = calloc(len + 1,  sizeof(char));
            memcpy(bool_as_string, current_pt, len * sizeof(char));
            param_temp->value = atoi(bool_as_string);

            HASH_ADD_STR(msg_bools, key, param_temp);
            free(bool_as_string);
            current_pt = next_pt;
            break;
        }
        case FLOAT:
        {   /* This assumes that a float was packed on the python side using

             float_bytes = struct.pack("<f", x)

            where x is the python float. This corresponds to a little-endian 
            convention, which is what all STM32 processors use.

            A single-precision float requires 4 bytes.
            */

            FloatParams *param_temp;

            param_temp = malloc(sizeof(FloatParams));
            if (param_temp == NULL)
            {
                MemManage_Handler();
            } // if the memory has not been allocated, interrupt operations

            strcpy(param_temp->key, msg_fields[i]);

            // Get length of field
            uint8_t len = 4;

            // Extract into struct
            memcpy(&(param_temp->value), current_pt, len);

            HASH_ADD_STR(msg_floats, key, param_temp);
            current_pt += len;

            break;
        }
        case BYTES:
        {
            /* Processing arbitary bytes is a little special. It is slightly
            complicated because the byte array could contain the reserved
            characters '|', '\r', and '\0' which is
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
            
            // Get length of field
            uint16_t len;
            memcpy(&len, current_pt, sizeof(len)); // sizeof(len) always 2 bytes
            param_temp->len = len;

            // Move to the actual byte data
            current_pt = current_pt + 2;

            // Extract into struct
            memset(&(param_temp->value[0]), 0, USB_BUFFER_SIZE);
            memcpy(&(param_temp->value[0]), current_pt, len);

            HASH_ADD_STR(msg_bytes, key, param_temp);
            current_pt += len;
            break;
        }
        default:
            break;
        }
    }
    
    /* Now that we have gone through all the fields, we definitely expect the 
    message terminator to be here */
    char *end_pt = strchr(current_pt, '\r'); 
    if (*end_pt != '\r'){
        usb_print("Failed to detect USB command terminator char! Trying my best");
        end_pt = current_pt;
    }

    return end_pt;

} // end parseMessageIntoHashTables()


/**
 * @brief Get the next key character (either | or \r) after the current pointer
 * location.
 * 
 * @param msg 
 * @return char* pointer to the next key character
 */
char* getNextKeyChar(char* msg){
    char *next_sep_pt = strchr(msg, '|'); // Index of next seperator
    char *next_end_pt = strchr(msg, '\r'); // Index of next terminator
    char *next_pt = NULL;

    if (next_sep_pt == NULL && next_end_pt == NULL){
        /* We sometimes enter this condition when the next field is
        either a float or byte. Since those fields could contain the
        string terminator '\0', then this will prematurely exit the 
        strchr() functions.
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

    return next_pt;
}


/**
 * @brief Frees memory from the hash tables.
 * 
 */
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