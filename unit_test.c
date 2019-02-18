/*
 * File unit_test.c
 *
 * Unitary test for testing basic non-volatile memory storage component.
 * We will test storing and reading different types of attributes types
 *
 * Implemented by: Mahfoudhi Farouk
 *
 * Date: 01/02/2019
 *
 */

/* ==================================================================== */
/* ========================== Include files =========================== */
/* ==================================================================== */
#include <stdio.h>
#include <string.h>
#include "gpNvm.h"

/* ==================================================================== */
/* ====================== Macros and constants  ======================= */
/* ==================================================================== */

#define MAX_LENGTH                20
#define ATTRIBUTE_ID_1            0x01
#define ATTRIBUTE_ID_2            0x02
#define ATTRIBUTE_ID_3            0x03
#define ATTRIBUTE_ID_4            0x04
#define ATTRIBUTE_ID_5            0x05

/* ==================================================================== */
/* ========================== Types Definition ======================== */
/* ==================================================================== */

typedef struct {
    UInt8  id;
    UInt32 options;
    UInt8  length;
    UInt8  data[MAX_LENGTH];
} gpTestData_t;

/* ==================================================================== */
/* ======================= Functions Definition ======================= */
/* ==================================================================== */

int main(int argc, char* argv[])
{
    UInt8 attr1[MAX_LENGTH];
    UInt8 attr2;
    UInt16 attr3;
    UInt32 attr4;
    gpTestData_t attr5;
    UInt8 writeData[2*MAX_LENGTH];
    UInt8 readData[2*MAX_LENGTH];
    UInt32 outVar = 0;
    gpTestData_t outTestData;
    UInt8 length;
    gpNvm_Result result = GPNVM_OK;

    //Fill attr1, attr2, attr3, attr4 and attr5 with some data
    for(UInt8 cpt=0;cpt <MAX_LENGTH;cpt++)
    {
        attr1[cpt]=cpt;
    }
    attr2 = 0xaa;
    attr3 = 0xbbbb;
    attr4 = 0xcccccccc;
    attr5.id = 55;
    attr5.options = 0;
    memset(attr5.data,0xee,MAX_LENGTH);
    attr5.length = MAX_LENGTH;
    memset(writeData,0, sizeof(writeData));
    memset(readData,0, sizeof(readData));
    //Init non-volatile memory component
    result = gpNvm_Init();

    if(result != GPNVM_OK)
    {
        printf("Cannot initialize non-volatile memory!\n");
        return -1;
    }
    /* Set/Get attribute 1 data */
    //Write attribute 1 data

    result = gpNvm_SetAttribute(ATTRIBUTE_ID_1, sizeof(attr1),attr1);

    if(result != GPNVM_OK)
    {
        printf("Cannot set attribute 1 data into non-volatile memory!\n");
        return -1;
    }
    //Read attribute 1 data
    result = gpNvm_GetAttribute(ATTRIBUTE_ID_1, &length,readData);

    if(result != GPNVM_OK)
    {
        printf("Cannot get attribute 1 from non-volatile memory!\n");
        return -1;
    }
    //Compare written data and data read from non-volatile memory

    if((length != sizeof(attr1)) || (memcmp(attr1,readData, sizeof(attr1))))
    {
        printf("Mismatch between written/read data of attribute 1!\n");
    }
    else
    {
        printf("Written/read data of attribute 1 match!\n");
    }
    //print Written/read data of attribute 1
    printf("Set attribute 1 data:");

    for(UInt8 cpt=0;cpt< sizeof(attr1);cpt++)
    {
        printf("%x ",attr1[cpt]);
    }
    printf("\nGet attribute 1 data:");

    for(UInt8 cpt=0;cpt< length;cpt++)
    {
        printf("%x ",readData[cpt]);
    }
    printf("\n");
    /* Set/Get attribute 2 data */
    //Write attribute 2 data
    memset(writeData,0, sizeof(writeData));
    memset(readData,0, sizeof(readData));
    result = gpNvm_SetAttribute(ATTRIBUTE_ID_2, sizeof(attr2),&attr2);

    if(result != GPNVM_OK)
    {
        printf("Cannot set attribute 2 data into non-volatile memory!\n");
        return -1;
    }
    //Read attribute 2 data
    result = gpNvm_GetAttribute(ATTRIBUTE_ID_2, &length,readData);

    if(result != GPNVM_OK)
    {
        printf("Cannot get attribute 2 from non-volatile memory!\n");
        return -1;
    }
    //Compare written data and data read from non-volatile memory
    memcpy(&outVar,readData, sizeof(attr2));

    if((length != sizeof(attr2)) || (outVar != attr2))
    {
        printf("Error! Mismatch between written/read data of attribute 2!\n");
    }
    else
    {
        printf("Written/read data of attribute 2 match!\n");
    }
    //print Written/read data of attribute 2
    printf("Set attribute 2 data: %x\n",attr2);
    printf("Get attribute 2 data: %x\n",readData[0]);
    /* Set/Get attribute 3 data */
    //Write attribute 3 data
    memset(writeData,0, sizeof(writeData));
    memset(readData,0, sizeof(readData));
    memcpy(writeData, &attr3,sizeof(attr3));

    result = gpNvm_SetAttribute(ATTRIBUTE_ID_3, sizeof(attr3),writeData);

    if(result != GPNVM_OK)
    {
        printf("Cannot set attribute 3 data into non-volatile memory!\n");
        return -1;
    }
    //Read attribute 3 data
    result = gpNvm_GetAttribute(ATTRIBUTE_ID_3, &length,readData);

    if(result != GPNVM_OK)
    {
        printf("Cannot get attribute 3 from non-volatile memory!\n");
        return -1;
    }
    //Compare written data and data read from non-volatile memory
    memcpy(&outVar,readData, sizeof(attr3));

    if((length != sizeof(attr3)) || (outVar != attr3))
    {
        printf("Error! Mismatch between written/read data of attribute 3!\n");
    }
    else
    {
        printf("Written/read data of attribute 3 match!\n");
    }
    //print Written/read data of attribute 3
    printf("Set attribute 3 data: %x\n",attr3);
    printf("Get attribute 3 data: %x\n",outVar);
    /* Set/Get attribute 4 data */
    //Write attribute 4 data
    memset(writeData,0, sizeof(writeData));
    memset(readData,0, sizeof(readData));
    memcpy(writeData, &attr4,sizeof(attr4));

    result = gpNvm_SetAttribute(ATTRIBUTE_ID_4, sizeof(attr4),writeData);

    if(result != GPNVM_OK)
    {
        printf("Cannot set attribute 4 data into non-volatile memory!\n");
        return -1;
    }
    //Read attribute 4 data
    result = gpNvm_GetAttribute(ATTRIBUTE_ID_4, &length,readData);

    if(result != GPNVM_OK)
    {
        printf("Cannot get attribute 4 from non-volatile memory!\n");
        return -1;
    }
    //Compare written data and data read from non-volatile memory
    memcpy(&outVar,readData, sizeof(attr4));

    if((length != sizeof(attr4)) || (outVar != attr4))
    {
        printf("Error! Mismatch between written/read data of attribute 4!\n");
    }
    else
    {
        printf("Written/read data of attribute 4 match!\n");
    }
    //print Written/read data of attribute 4
    printf("Set attribute 4 data: %x\n",attr4);
    printf("Get attribute 4 data: %x\n",outVar);
    /* Set/Get attribute 5 data */
    //Write attribute 5 data
    memset(writeData,0, sizeof(writeData));
    memset(readData,0, sizeof(readData));
    memcpy(writeData, &attr5,sizeof(gpTestData_t));

    result = gpNvm_SetAttribute(ATTRIBUTE_ID_5, sizeof(gpTestData_t),writeData);

    if(result != GPNVM_OK)
    {
        printf("Cannot set attribute 5 data into non-volatile memory!\n");
        return -1;
    }
    //Read attribute 5 data
    result = gpNvm_GetAttribute(ATTRIBUTE_ID_5, &length,readData);

    if(result != GPNVM_OK)
    {
        printf("Cannot get attribute 5 from non-volatile memory!\n");
        return -1;
    }
    //Compare written data and data read from non-volatile memory
    memcpy(&outTestData,readData, sizeof(gpTestData_t));

    if((length != sizeof(gpTestData_t)) || (memcmp(writeData,readData, sizeof(gpTestData_t))))
    {
        printf("Error! Mismatch between written/read data of attribute 5!\n");
    }
    else
    {
        printf("Written/read data of attribute 5 match!\n");
    }
    memcpy(&outTestData,writeData, sizeof(gpTestData_t));
    //print Written/read data of attribute 5
    printf("Set attribute 5 data:\n");
    printf(" id=%x\n options=%d\n length=%d\n data=",attr5.id,attr5.options,attr5.length);

    for(UInt8 cpt=0;cpt< MAX_LENGTH;cpt++)
    {
        printf("%x ",attr5.data[cpt]);
    }
    printf("\nGet attribute 5 data:\n");
    printf(" id=%x\n options=%d\n length=%d\n data=",outTestData.id,outTestData.options,outTestData.length);

    for(UInt8 cpt=0;cpt< MAX_LENGTH;cpt++)
    {
        printf("%x ",outTestData.data[cpt]);
    }
    printf("\n");
    //Uninit non-volatile memory component
    result = gpNvm_Uninit();

    if(result != GPNVM_OK)
    {
        printf("Cannot uninitialize non-volatile memory!\n");
        return -1;
    }
    return 0;
}
