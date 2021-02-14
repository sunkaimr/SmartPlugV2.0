/*
 * user_temp.c
 *
 *  Created on: 2018骞�11鏈�17鏃�
 *      Author: lenovo
 */

#include "user_common.h"
#include "esp_common.h"

const INT8 TempTab[] =
{
    0,  1,   2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
    60, 61, 62, 63, 64, 65
};

const float AdTab[] =
{
    142, 144, 147, 152, 159, 164, 170, 176, 184, 190,
    198, 205, 212, 219, 226, 232, 241, 250, 258, 266,
    276, 285, 296, 306, 318, 327, 337, 347, 360, 375,
    388, 400, 412, 428, 442, 454, 466, 480, 495, 513,
    527, 540, 554, 569, 583, 600, 615, 630, 646, 660,
    676, 692, 711, 729, 746, 766, 782, 804, 823, 843,
    862, 882, 900, 920, 942, 961
};

static UINT32 uiTempAdcValue = 0;

VOID TEMP_TempCallBack( VOID )
{
    STATIC UINT uiLastAdc = 0;
    UINT uiCurAdc = 0;

    if ( uiLastAdc == 0 )
    {
        uiLastAdc = system_adc_read();
    }
    portENTER_CRITICAL();
    uiCurAdc = system_adc_read();
    portEXIT_CRITICAL();

    uiTempAdcValue = ( uiLastAdc * 5 +  uiCurAdc * 5 ) / 10;
    uiLastAdc = uiTempAdcValue;
}

float TEMP_GetTemperature( VOID )
{
    UINT8 i = 0;
    float fTemp = 0;

    for( i = 0; i < sizeof(AdTab)/sizeof(AdTab[0]); i++ )
    {
        if ( uiTempAdcValue < AdTab[i] )
        {
            break;
        }
    }

    if ( uiTempAdcValue > AdTab[sizeof(AdTab)/sizeof(AdTab[0])-1] )
    {
        fTemp = 66;
    }
    else if ( uiTempAdcValue <= AdTab[0] )
    {
        fTemp = -1;
    }
    else
    {
        fTemp = TempTab[i-1] + ( uiTempAdcValue - AdTab[i-1] ) * 1.0 / (AdTab[i] - AdTab[i-1]);
    }

    return fTemp;

}
