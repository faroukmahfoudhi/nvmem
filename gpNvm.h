/*
 * File gpNvm.h
 * 
 * Basic non-volatile memory storage component.
 * For simplicity, the underlying non-volatile memory is modeled as a file
 *
 * Implemented by: Mahfoudhi Farouk
 * 
 * Date: 01/02/2019
 * 
 */
 
#ifndef _GPNVM_H_
#define _GPNVM_H_
/* ==================================================================== */
/* ========================== Include files =========================== */
/* ==================================================================== */

#include <stdint.h>

/* ==================================================================== */
/* ====================== Macros and constants  ======================= */
/* ==================================================================== */
#define GPNVM_MEMORY_SIZE                    2048     /* Total non-volatile memory data size */
#define GPNVM_FILE_NAME                      "gpNvm"  /* File to be used to emulate non-volaltile memory */

enum gpNvm_ErrorStatus
{
	GPNVM_OK,                           /* Function result is OK */
	GPNVM_ERROR_OPENING_FILE,           /* Error while opening the file error */
	GPNVM_ERROR_ALREADY_INITIALIZED,    /* The component is already initialized  error */
	GPNVM_ERROR_NOT_INITIALIZED,        /* The component is not initialized  error */
	GPNVM_ERROR_INVALID_PARAMETERS,     /* Invalid parameters error */
	GPNVM_ERROR_INVALID_ATTRIBUTE_ID,   /* Attribute id not found error */
	GPNVM_ERROR_CORRUPTED_ATTRIBUTE,    /* Corrupted attribute data error */
    GPNVM_ERROR_MEMORY_FULL,            /* Memory full error */
	GPNVM_ERROR_UNKNOWN                 /* Unknown error */
};

/* ==================================================================== */
/* ========================== Types Definition ======================== */
/* ==================================================================== */

typedef uint16_t UInt16;
typedef uint32_t UInt32;
typedef unsigned char UInt8;
typedef UInt8 gpNvm_AttrId;
typedef UInt8 gpNvm_Result;

/* ==================================================================== */
/* ======================= Functions prototypes ======================= */
/* ==================================================================== */

/*
 * Name: gpNvm_Init
 *
 * Description: Initialize non-volatile memory component. First this function
 * will check if the component is already initialized. If not, it will open the
 * file emulating the non-volatile memory. If the file is empty, it will initialize
 * cache and write it into the file. If not, it will load file content into cache.
 *
 * Parameters: None
 *
 * Return value: gpNvm_Result: GPNVM_OK: the component is initialized successfully
 *                             GPNVM_ERROR_ALREADY_INITIALIZED: the component is already initialized
 *                             GPNVM_ERROR_OPENING_FILE: the is a problem when opening the file emulating the non-volatile memory
 */
gpNvm_Result gpNvm_Init(void);

/*
 * Name: gpNvm_Uninit
 *
 * Description: Uninitialize non-volatile memory component. First this function
 * will check if the component is already unitiliazed. Then it will write cache into
 * the file emulating non-volatile memory.
 *
 * Parameters: None
 *
 * Return value: gpNvm_Result: GPNVM_OK: the component is uninitialized successfully
 *                             GPNVM_ERROR_NOT_INITIALIZED: the component is not initialized
 */
gpNvm_Result gpNvm_Uninit(void);

/*
 * Name: gpNvm_GetAttribute
 *
 * Description: Get attribute data from non-volatile memory.
 * This function check if the component is already initialized, if the provided arguments are valid, if the attribute
 * id is already stored in the non-volatile memory then check if the attribute data is corrupted or not by comparing
 * its stored crc by the calculated one. If data is sane, it will copy it into provided args.
 *
 * Parameters:
 *            gpNvm_AttrId attrId: attribute id
 *            UInt8* pLength: pointer to a variable that will store the length of attribute data
 *            UInt8* pValue: pointer to store attribute data
 *
 * Return value: gpNvm_Result: GPNVM_OK: attribute data is read successfully
 *                             GPNVM_ERROR_NOT_INITIALIZED: the component is not initialized
 *                             GPNVM_ERROR_INVALID_PARAMETERS: pointers provided as arguments to the function are not valid
 *                             GPNVM_ERROR_INVALID_ATTRIBUTE_ID: the provided attribute is not in non-volatile memory
 *                             GPNVM_ERROR_CORRUPTED_ATTRIBUTE: attribute data are corrupted
 */
gpNvm_Result gpNvm_GetAttribute(gpNvm_AttrId attrId, UInt8* pLength, UInt8* pValue);

/*
 * Name: gpNvm_SetAttribute
 *
 * Description: Set attribute data to non-volatile memory.
 * This function checks if the component is already initialized and if the provided arguments are valid. Then it
 * calculates its crc and offset in the user non-volatile memory, update cache then write the cache into the file in case
 * the attributes is already stored. If not, it checks if there is still place to store a new attribute there. Then it
 * calculates its crc and offset in the user non-volatile memory, update cache then write the cache into the file.
 *
 * Parameters:
 *            gpNvm_AttrId attrId: attribute id
 *            UInt8 pLength: length of attribute data
 *            UInt8* pValue: pointer to attribute data
 *
 * Return value: gpNvm_Result: GPNVM_OK: attribute data is written successfully
 *                             GPNVM_ERROR_NOT_INITIALIZED: the component is not initialized
 *                             GPNVM_ERROR_INVALID_PARAMETERS: pointers provided as arguments to the function are not valid
 *                             GPNVM_ERROR_INVALID_ATTRIBUTE_ID: the provided attribute is not in non-volatile memory
 *                             GPNVM_ERROR_MEMORY_FULL: Memory is full
 */
gpNvm_Result gpNvm_SetAttribute(gpNvm_AttrId attrId, UInt8 length, UInt8* pValue);

#endif //_GPNVM_H_
