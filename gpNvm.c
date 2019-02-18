/*
 * File gpNvm.c
 * 
 * Basic non-volatile memory storage component.
 * For simplicity, the underlying non-volatile memory is modeled as a file
 * 
 * Implemented by: Mahfoudhi Farouk
 * 
 * Date: 01/02/2019
 * 
 */

/* ==================================================================== */
/* ====================== Component Description  ====================== */
/* ==================================================================== */

/*
 * This is an implementation of a non-volatile memory component emulated on a file defined by GPNVM_FILE_NAME.
 * Data is stored in the non-volatile memory as attributes with unique ids from 0 to 255 (supported values by one UINT8).
 *
 * 1) Layout
 *
 * The non volatile memory is divided into 3 areas:
 *     a- User attributes data area: stores the attributes set by User.
 *        Each attribute concists on:
 *           - length: length of attribute data (1 byte)
 *           - value: attribute value (length bytes)
 *
 *        User attributes data area is loaded in cache in gpNvm_MemoryCache buffer when initializing the component.
 *        Setting attributes is made in this buffer then written in the file GPNVM_FILE_NAME.
 *        Getting an attribute is done from this buffer.
 *        When uninitilizing the component this bufffer is written in the file.
 *          _________________________________________________________________________________
 *          |length5|       value5         |length0|     value0    | ... |lengthN|  valueN   |
 *          |_______|______________________|_______|_______________|_____|_______|___________|
 *                                  Layout of User attributes data area
 *
 *     b- Attribute index table area: Table containing the offset of each attribute stored in user attributes area.
 *        This table has 256 element since we have 256 attributes max, 2 bytes each having in total 512 bytes.
 *                           ________________________________________________
 *                           |offset0|offset1|0xFFFF |0xFFFF| ... |offset255|
 *                           |_______|_______|_______|______|_____|_________|
 *                                Layout of attribute index table area
 *
 *        Attribute index table area is loaded in cache in gpNvm_MemoryIndexTable buffer when initializing the component.
 *        By default, this tables is set to 0xFFFF. When getting/setting an attribute with attrid, this buffer is accessed at attrid
 *        index to get the offset in gpNvm_MemoryCache. If offset is 0xFFFF, then the attribute is not stored in non-volatile
 *        memory. If not the value will be the index in gpNvm_MemoryCache where this attribute is stored.
 *        When setting a new attribute attrId, this buffer is browsed to calculate its offset by adding length of already stored attributes.
 *        When uninitilizing the component this bufffer is written in the file.
 *
 *     c- Attribute CRC table area: Table containing the CRC8 of each attribute data stored in user attributes area.
 *        This table has 256 element since we have 256 attributes max, 1 bytes each having in total 256 bytes.
 *                                  __________________________________
 *                                  |crc0|crc1|0xFF|0xFF| ... |crc255|
 *                                  |____|____|____|____|_____|______|
 *                                  Layout of attribute CRC table area
 *
 *       Attribute CRC table area is loaded in cache in gpNvm_AttributesCrcTable buffer when initializing the component.
 *       When getting an attribute with attrId, gpNvm_AttributesCrcTable[attrId] is the crc of this attribute. Comparing it to
 *       the calculated one of attribute data in gpNvm_MemoryCache can check if it is corrupted or not.
 *       When setting an attribute, the corresponding crc is calculated and stored in this buffer then written into the file.
 *       When uninitilizing the component this bufffer is written in the file.
 *
 * So the non volatile memory layout will be as below. The size of non-volatile memory is set in GPNVM_MEMORY_SIZE.
 * The size of user attributes data area = GPNVM_MEMORY_SIZE - 2*GPNVM_MEMORY_INDEX_TABLE_SIZE - GPNVM_ATTRIBUTES_CRCS_SIZE
 *                                       = GPNVM_MEMORY_SIZE - 768 bytes
 *               ____________________________________________________________________________________
 *               |Attribute index table area|Attribute CRC table area|   User attributes data area   |
 *               |        (512 bytes)       |      (256 bytes)       |(GPNVM_MEMORY_SIZE - 768 bytes)|
 *               |__________________________|________________________|_______________________________|
 *                                          Non-volatile memory layout
 *
 *
 * 2) Init
 *
 * When initializing the component, first we check if the file emulating the non-volatile memory defined by GPNVM_FILE_NAME is present.
 * If it is the case, data from User attributes data area, Attribute index table area and Attribute CRC table area are loaded respectively to
 * gpNvm_MemoryCache, gpNvm_MemoryIndexTable and gpNvm_AttributesCrcTable buffers.
 * If not, we initialize the cache by setting gpNvm_MemoryIndexTable buffer to 0xFFFF, gpNvm_AttributesCrcTable buffer to 0xFF and
 * gpNvm_MemoryCache buffer to zeros. Then this file is created and the cache is written there.
 *
 *
 * 3) Uninit
 *
 * When uninitializing the component the cache gpNvm_MemoryIndexTable, gpNvm_AttributesCrcTable and gpNvm_MemoryCache is written in the file
 * defined by GPNVM_FILE_NAME then the file is closed.
 *
 * 4) Get Attribute
 *
 * When getting an attribute with id attrId. We check first its offset in user data are stored in gpNvm_MemoryIndexTable[attrId].
 * If it is 0xFFFF then this attribute is not stored so an error is reported.
 * If not, we get the attribute crc, length and value as below:
 *    - offset = gpNvm_MemoryIndexTable[attrId]
 *    - crc8 = gpNvm_AttributesCrcTable[attrId]
 *    - length = gpNvm_MemoryCache[offset]
 *    - value = [gpNvm_MemoryCache[offset + 1] => gpNvm_MemoryCache[offset + length]]
 * Then we calculate crc of value and compare it to crc8. If they are different, then attribute data is corrupted. An error is reported in this case.
 * If not, value and length are copied to provided args pointers.
 *
 * 5) Set attribute
 *
 * When setting an attribute with id attrId. We check first its offset in user data are stored in gpNvm_MemoryIndexTable[attrId].
 * If it is 0xFFFF then this attribute is not stored. The following is done:
 *   - We calculate crc8 of attribute value and store it in gpNvm_AttributesCrcTable[attrId]
 *   - we calculate the attribute offset by doing the sum of lengths of already stored attributes, then store it in gpNvm_MemoryIndexTable[attrId]
 *   - gpNvm_MemoryCache[offset] = length
 *   - copy attribute value to gpNvm_MemoryCache[offset + 1] => gpNvm_MemoryCache[offset + length]
 *   - the cache gpNvm_MemoryIndexTable, gpNvm_AttributesCrcTable and gpNvm_MemoryCache is written in the file defined by GPNVM_FILE_NAME
 * If not, the attribute is already stored, we will update the new value. First we compare the old and new values. If they are the same then nothing
 * to be done. If not:
 *   - offset = gpNvm_MemoryIndexTable[attrId]
 *   - We calculate crc8 of attribute value and store it in gpNvm_AttributesCrcTable[attrId]
 *   - copy attribute value to gpNvm_MemoryCache[offset + 1] => gpNvm_MemoryCache[offset + length]
 *   - the cache gpNvm_MemoryIndexTable, gpNvm_AttributesCrcTable and gpNvm_MemoryCache is written in the file defined by GPNVM_FILE_NAME
 * Here both old and new attributes must have the same length.
 */

/* ==================================================================== */
/* ========================== Include files =========================== */
/* ==================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpNvm.h"

/* ==================================================================== */
/* ====================== Macros and constants  ======================= */
/* ==================================================================== */

#define GPNVM_MEMORY_INDEX_TABLE_SIZE        256      /* non-volatile memory index table size */
#define GPNVM_ATTRIBUTES_CRCS_SIZE           256      /* Attributes data crc */
/* User non-volatile memory data size */
#define GPNVM_USER_MEMORY_SIZE               (GPNVM_MEMORY_SIZE - (sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE + GPNVM_ATTRIBUTES_CRCS_SIZE))

/* ==================================================================== */
/* ========================= Global variables ========================= */
/* ==================================================================== */

/* File descriptor of the file emulating non-volaltile memory */
static FILE* gpNvm_FileDescriptor = NULL;

/* User non-volatile memory (attributes) data cache */
static UInt8 gpNvm_MemoryCache[GPNVM_USER_MEMORY_SIZE];

/* Table containing the offset of each attribute stored in user non-volatile memory cache gpNvm_MemoryCache */
static UInt16 gpNvm_MemoryIndexTable[GPNVM_MEMORY_INDEX_TABLE_SIZE];

/* Table containing the CRC8 of each attribute data in non-volatile memory */
static UInt8 gpNvm_AttributesCrcTable[GPNVM_ATTRIBUTES_CRCS_SIZE];

/* ==================================================================== */
/* ==================== Local functions Definition ==================== */
/* ==================================================================== */

/*
 * Name: gpNvm_CalculateChecksum
 * 
 * Description: Simple implementation of CRC8 hash function. This functions
 * will be used to calculate crc of attributes to be stored in non-volatile memory
 * and detect if it is corrupted or not.
 * This is a generic implementation from the internet that can be changed by other implementations.
 * 
 * Parameters: 
 *           UInt8* ptr: Pointer to the data calculate its CRC
 *           UInt8 length: Length of data
 * 
 * Return value: UInt8: calculated CRC
 */
static UInt8 gpNvm_CalculateChecksum(UInt8* ptr, UInt8 length)
{
    UInt8 crc = 0xFF;
    UInt8 i, j;
    
    for (i = 0; i < length; i++) 
    {
        crc ^= ptr[i];
        
        for (j = 0; j < 8; j++) 
        {
            if ((crc & 0x80) != 0)
                crc = (UInt8)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* ==================================================================== */
/* ======================= Functions Definition ======================= */
/* ==================================================================== */

/*
 * Name: gpNvm_Init
 * 
 * Description: Initilize non-volatile menory component. First this function
 * will check if the component is already initilized. If not, it will open the
 * file emulating the non-volatil memory. If the file is empty, it will initilize
 * cache and write it into the file. If not, it will load file content into cache.
 * 
 * Parameters: None
 * 
 * Return value: gpNvm_Result: GPNVM_OK: the component is initilized successfully
 *                             GPNVM_ERROR_ALREADY_INITIALIZED: the component is already initilized
 *                             GPNVM_ERROR_OPENING_FILE: the is a problem when opening the file emulating the non-volatile memory
 */
gpNvm_Result gpNvm_Init(void)
{
	//Check if the component is already initialized
	if(gpNvm_FileDescriptor != NULL)
	{
		printf("[gpNvm][%s] Component already initialized! Abort.\n",__FUNCTION__);
		return GPNVM_ERROR_ALREADY_INITIALIZED;
	}
	//Open the non-volatile memory file
	gpNvm_FileDescriptor = fopen(GPNVM_FILE_NAME,"r+b");

	if(gpNvm_FileDescriptor == NULL)
	{
		//File does not exist, create it
		gpNvm_FileDescriptor = fopen(GPNVM_FILE_NAME, "wb");

		if(gpNvm_FileDescriptor == NULL)
		{
			printf("[gpNvm][%s] Cannot oppen file %s! Abort.\n",__FUNCTION__,GPNVM_FILE_NAME);
			return GPNVM_ERROR_OPENING_FILE;
		}
	}
	//Check if the non-volatile memory file is empty
	fseek(gpNvm_FileDescriptor, 0, SEEK_END);

	if(ftell(gpNvm_FileDescriptor) == 0)
	{
		/* Initialize non-volatile memory file and the cache */
		//Set Memory index table section to 0xFF in file and cache
		memset(gpNvm_MemoryIndexTable,0xFF,sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE);
		fseek(gpNvm_FileDescriptor, 0, SEEK_SET);
		fwrite(gpNvm_MemoryIndexTable, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE,1,gpNvm_FileDescriptor);
		//Set attributes CRC table section to 0xFF in file and cache
		memset(gpNvm_AttributesCrcTable,0xFF,GPNVM_ATTRIBUTES_CRCS_SIZE);
		fseek(gpNvm_FileDescriptor, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE, SEEK_SET);
		fwrite(gpNvm_AttributesCrcTable, GPNVM_ATTRIBUTES_CRCS_SIZE,1,gpNvm_FileDescriptor);
		//Set user attributes data section to zeros in file and cache
		memset(gpNvm_MemoryCache,0xFF,GPNVM_USER_MEMORY_SIZE);
		fseek(gpNvm_FileDescriptor, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE + GPNVM_ATTRIBUTES_CRCS_SIZE, SEEK_SET);
		fwrite(gpNvm_MemoryCache,GPNVM_USER_MEMORY_SIZE,1,gpNvm_FileDescriptor);
	}
	else
	{
		/* Load non-volatile memory file data into cache */
		//Load Memory index table
		fseek(gpNvm_FileDescriptor, 0, SEEK_SET);
		fread(gpNvm_MemoryIndexTable,sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE, 1, gpNvm_FileDescriptor);
		//Load attributes CRC table
		fseek(gpNvm_FileDescriptor, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE, SEEK_SET);
		fread(gpNvm_AttributesCrcTable, GPNVM_ATTRIBUTES_CRCS_SIZE,1,gpNvm_FileDescriptor);
		//Load user attributes data
		fseek(gpNvm_FileDescriptor, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE + GPNVM_ATTRIBUTES_CRCS_SIZE, SEEK_SET);
		fread(gpNvm_MemoryCache,GPNVM_USER_MEMORY_SIZE,1,gpNvm_FileDescriptor);
	}
	fseek(gpNvm_FileDescriptor, 0, SEEK_SET);
	return GPNVM_OK;
}

/*
 * Name: gpNvm_Uninit
 * 
 * Description: Uninitilize non-volatile menory component. First this function
 * will check if the component is already initilized. Then it will write cache into
 * the file emulating non-volatile memory.
 * 
 * Parameters: None
 * 
 * Return value: gpNvm_Result: GPNVM_OK: the component is uninitilized successfully
 *                             GPNVM_ERROR_NOT_INITIALIZED: the component is not initilized
 */
gpNvm_Result gpNvm_Uninit(void)
{
	//Check if the component is initialized
	if(gpNvm_FileDescriptor == NULL)
	{
		printf("[gpNvm][%s] Component not initialized! Abort.\n",__FUNCTION__);
		return GPNVM_ERROR_NOT_INITIALIZED;
	}
	/* Write cache into non-volatile memory file */
	//Write Memory index table section to the file
	fseek(gpNvm_FileDescriptor, 0, SEEK_SET);
	fwrite(gpNvm_MemoryIndexTable, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE,1,gpNvm_FileDescriptor);
	//Write attributes CRC table into the file
	fseek(gpNvm_FileDescriptor, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE, SEEK_SET);
	fwrite(gpNvm_AttributesCrcTable, GPNVM_ATTRIBUTES_CRCS_SIZE,1,gpNvm_FileDescriptor);
	//Write user attributes data section into the file
	fseek(gpNvm_FileDescriptor, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE + GPNVM_ATTRIBUTES_CRCS_SIZE, SEEK_SET);
	fwrite(gpNvm_MemoryCache,GPNVM_USER_MEMORY_SIZE,1,gpNvm_FileDescriptor);
	//Close the non-volatile memory file
	fclose(gpNvm_FileDescriptor);
	return GPNVM_OK;
}

/*
 * Name: gpNvm_GetAttribute
 * 
 * Description: Get attribute data from non-volatile memory.
 * This function check if the component is already initialized, if the provided argument sare valid, if the attribute
 * id is already stored in the non-volatile memory then check if the attribute data is corrupted or not by comparing 
 * its stored crc by the calculated one. If data is sane, it will copy it into provided args.
 * 
 * Parameters: 
 *            gpNvm_AttrId attrId: attribute id
 *            UInt8* pLength: pointer to a variable that will store the length of attribute data
 *            UInt8* pValue: pointer to store attribute data
 * 
 * Return value: gpNvm_Result: GPNVM_OK: attribute data is read successfully
 *                             GPNVM_ERROR_NOT_INITIALIZED: the component is not initilized
 *                             GPNVM_ERROR_INVALID_PARAMETERS: pointers provided as arguments to the function are not valid
 *                             GPNVM_ERROR_INVALID_ATTRIBUTE_ID: the provided attribute is not in non-volatile memory
 *                             GPNVM_ERROR_CORRUPTED_ATTRIBUTE: attribute data are corrupted
 */
gpNvm_Result gpNvm_GetAttribute(gpNvm_AttrId attrId, UInt8* pLength, UInt8* pValue)
{
	UInt16 attributeOffset = 0;
	UInt8 attributeLength = 0;
	UInt8 attributeCrc = 0;

	//Check if the component is initialized
	if(gpNvm_FileDescriptor == NULL)
	{
		printf("[gpNvm][%s] Component not initialized! Abort.\n",__FUNCTION__);
		return GPNVM_ERROR_NOT_INITIALIZED;
	}
	//Validate input pointers
	if((pLength == NULL) || (pValue == NULL))
	{
		printf("[gpNvm][%s] Invalid input pointers! Abort.\n",__FUNCTION__);
		return GPNVM_ERROR_INVALID_PARAMETERS;
	}
	//Check if attribute is in non-volatile memory
	attributeOffset = gpNvm_MemoryIndexTable[attrId];

	if(attributeOffset == 0xFFFF)
	{
		printf("[gpNvm][%s] Invalid attribute! Abort.\n",__FUNCTION__);
		return GPNVM_ERROR_INVALID_ATTRIBUTE_ID;
	}
	//Validate attribute data by comparing attribute crc stored in gpNvm_AttributesCrcTable and the calculated crc of the attribute data in gpNvm_MemoryCache
	attributeCrc = gpNvm_AttributesCrcTable[attrId];
	attributeLength = gpNvm_MemoryCache[attributeOffset];

	if(gpNvm_CalculateChecksum(&gpNvm_MemoryCache[attributeOffset + 1],attributeLength) != attributeCrc)
	{
		printf("[gpNvm][%s] Corrupted attribute data! Abort.\n",__FUNCTION__);
		return GPNVM_ERROR_CORRUPTED_ATTRIBUTE;
	}
	*pLength = attributeLength;
	memcpy(pValue,&gpNvm_MemoryCache[attributeOffset + 1],attributeLength);
	return GPNVM_OK;
}

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
 *                             GPNVM_ERROR_NOT_INITIALIZED: the component is not initilized
 *                             GPNVM_ERROR_INVALID_PARAMETERS: pointers provided as arguments to the function are not valid
 *                             GPNVM_ERROR_INVALID_ATTRIBUTE_ID: the provided attribute is not in non-volatile memory
 *                             GPNVM_ERROR_MEMORY_FULL: Memory is full
 */
gpNvm_Result gpNvm_SetAttribute(gpNvm_AttrId attrId, UInt8 length, UInt8* pValue)
{
	UInt16 attributeOffset = 0;
	UInt8 attributeLength = 0;
	UInt8 attributeCrc = 0;

	//Check if the component is initialized
	if(gpNvm_FileDescriptor == NULL)
	{
		printf("[gpNvm][%s] Component not initialized! Abort.\n",__FUNCTION__);
		return GPNVM_ERROR_NOT_INITIALIZED;
	}
	//Validate input pointer
	if(pValue == NULL)
	{
		printf("[gpNvm][%s] Invalid input pointers! Abort.\n",__FUNCTION__);
		return GPNVM_ERROR_INVALID_PARAMETERS;
	}
	//Check if attribute is in non-volatile memory
	attributeOffset = gpNvm_MemoryIndexTable[attrId];

	if(attributeOffset != 0xFFFF)
	{
		//Attribute is in non-volatile memory, compare old and new values
		attributeLength = gpNvm_MemoryCache[attributeOffset];

		if(length != attributeLength)
		{
			printf("[gpNvm][%s] Invalid attribute length (%d != %d)! Abort.\n",__FUNCTION__,length,attributeLength);
			return GPNVM_ERROR_INVALID_PARAMETERS;
		}

		if(memcmp(pValue,&gpNvm_MemoryCache[attributeOffset + 1],length) == 0)
		{
			//New attribute value is identical to the stored one, do no thing
			return GPNVM_OK;
		}
		else
		{
			//Update attribute value
			memcpy(&gpNvm_MemoryCache[attributeOffset + 1],pValue,length);
			//Calculate new CRC and update gpNvm_AttributesCrcTable
			gpNvm_AttributesCrcTable[attrId] = gpNvm_CalculateChecksum(pValue,length);
			/* Write cache into non-volatile memory file */
			//Write Memory index table section to the file
			fseek(gpNvm_FileDescriptor, 0, SEEK_SET);
			fwrite(gpNvm_MemoryIndexTable, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE,1,gpNvm_FileDescriptor);
			//Write attributes CRC table into the file
			fseek(gpNvm_FileDescriptor, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE, SEEK_SET);
			fwrite(gpNvm_AttributesCrcTable, GPNVM_ATTRIBUTES_CRCS_SIZE,1,gpNvm_FileDescriptor);
			//Write user attributes data section into the file
			fseek(gpNvm_FileDescriptor, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE + GPNVM_ATTRIBUTES_CRCS_SIZE, SEEK_SET);
			fwrite(gpNvm_MemoryCache,GPNVM_USER_MEMORY_SIZE,1,gpNvm_FileDescriptor);
		}
	}
	else
	{
		//Will add new attribute in non-volatile memory, calculate attribute crc and update crc attribute table
		gpNvm_AttributesCrcTable[attrId] = gpNvm_CalculateChecksum(pValue,length);
		//Calculate attribute offset in non-volatile memory cache
		attributeOffset = 0;

		for(UInt16 cpt=0;cpt<GPNVM_MEMORY_INDEX_TABLE_SIZE;cpt++)
		{
			if(gpNvm_MemoryIndexTable[cpt] != 0xFFFF)
			{
				attributeOffset += gpNvm_MemoryCache[gpNvm_MemoryIndexTable[cpt]] + 1;
			}
		}
		//Check if we have spare place in non-volatile memory
		if(attributeOffset >= GPNVM_USER_MEMORY_SIZE)
        {
            printf("[gpNvm][%s] Memory full! Abort.\n",__FUNCTION__);
            return GPNVM_ERROR_MEMORY_FULL;
        }
		//Update non-volatile memory index table
		gpNvm_MemoryIndexTable[attrId] = attributeOffset;
        //Update non-volatile memory cache
		gpNvm_MemoryCache[attributeOffset] = length;
		memcpy(&gpNvm_MemoryCache[attributeOffset + 1],pValue,length);
		/* Write cache into non-volatile memory file */
		//Write Memory index table section to the file
		fseek(gpNvm_FileDescriptor, 0, SEEK_SET);
		fwrite(gpNvm_MemoryIndexTable, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE,1,gpNvm_FileDescriptor);
		//Write attributes CRC table into the file
		fseek(gpNvm_FileDescriptor, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE, SEEK_SET);
		fwrite(gpNvm_AttributesCrcTable, GPNVM_ATTRIBUTES_CRCS_SIZE,1,gpNvm_FileDescriptor);
		//Write user attributes data section into the file
		fseek(gpNvm_FileDescriptor, sizeof(UInt16)*GPNVM_MEMORY_INDEX_TABLE_SIZE + GPNVM_ATTRIBUTES_CRCS_SIZE, SEEK_SET);
		fwrite(gpNvm_MemoryCache,GPNVM_USER_MEMORY_SIZE,1,gpNvm_FileDescriptor);
	}
	return GPNVM_OK;
}
