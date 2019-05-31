/*
 * user_flash.c
 *
 *  Created on: 2018年11月21日
 *      Author: lenovo
 */


#include "esp_common.h"
#include "user_common.h"


UINT FlASH_Write( UINT uiAddr, CHAR* pcBuf, UINT uiLen )
{
	UINT uiRet = 0;
	UINT uiLoop = 0;
	CHAR *pcFlashBuf = NULL;
	UINT uiSectorPos = 0;
	UINT uiSectorOff = 0;
	UINT uiRemain = 0;

	uiSectorPos = uiAddr / FLASH_SEC_SIZE;	//写入的扇区
	uiSectorOff = uiAddr % FLASH_SEC_SIZE;	//扇区内的偏移地址
	uiRemain = FLASH_SEC_SIZE - uiSectorOff;	//扇区剩余大小

	if ( NULL == pcBuf )
	{
	    LOG_OUT(LOGOUT_ERROR, "FlASH_Write failed, pcBuf is NULL.");
		return FAIL;
	}

	if ( uiLen == 0 )
	{
		return OK;
	}

	pcFlashBuf = malloc( FLASH_SEC_SIZE );
	if ( NULL == pcFlashBuf )
	{
	    LOG_OUT(LOGOUT_ERROR, "malloc failed, pcFlashBuf is NULL.");
		return FAIL;
	}

	//判断写入数据长度是否大于扇区剩余大小，如果是首次只写入剩余扇区大小剩余的下次再写入，否则就写入要写入的数据长度
	uiRemain = uiLen >= uiRemain ? uiRemain : uiLen;
	while( 1 )
	{
		//因为只能按扇区大小为单位操作，所先要读出整个扇区的数据原数据不动只修改要写入的数据
		uiRet = spi_flash_read((UINT)uiSectorPos*FLASH_SEC_SIZE, (UINT*)pcFlashBuf, FLASH_SEC_SIZE);
		if ( SPI_FLASH_RESULT_OK != uiRet )
		{
		    LOG_OUT(LOGOUT_ERROR, "spi_flash_read failed, addr:0x%X, uiRet:%d", uiSectorPos*FLASH_SEC_SIZE, uiRet);
		    FREE_MEM(pcFlashBuf);
		    return FAIL;
		}

		//修改要写入的数据
		for ( uiLoop = 0; uiLoop < uiRemain; uiLoop ++ )
		{
			pcFlashBuf[uiSectorOff + uiLoop] = pcBuf[uiLoop];
		}

		//写入前先擦除整个扇区
		uiRet = spi_flash_erase_sector(uiSectorPos);
		if ( SPI_FLASH_RESULT_OK != uiRet )
		{
		    LOG_OUT(LOGOUT_ERROR, "spi_flash_erase_sector failed, uiSectorPos:%d, uiRet:%d", uiSectorPos, uiRet);
		    FREE_MEM(pcFlashBuf);
		    return FAIL;
		}

		//写入修改后的数据
		uiRet = spi_flash_write((UINT)uiSectorPos*FLASH_SEC_SIZE, (UINT*)pcFlashBuf, FLASH_SEC_SIZE);
		if ( SPI_FLASH_RESULT_OK != uiRet )
		{
		    LOG_OUT(LOGOUT_ERROR, "spi_flash_write failed, addr:0x%X, uiRet:%d", uiSectorPos*FLASH_SEC_SIZE, uiRet);
		    FREE_MEM(pcFlashBuf);
		    return FAIL;
		}

		if ( uiRemain == uiLen )
		{
			break;
		}
		else
		{
			uiSectorPos ++;
			uiSectorOff = 0;

			pcBuf 	+= uiRemain;
			uiAddr 	+= uiRemain;
			uiLen   -= uiRemain;

			uiRemain = uiLen > FLASH_SEC_SIZE ? FLASH_SEC_SIZE : uiLen;
		}
	}

	FREE_MEM(pcFlashBuf);
	return OK;
}



UINT FlASH_Read( UINT uiAddr, CHAR* pcBuf, UINT uiLen )
{
	UINT uiRet = 0;
	UINT uiLoop = 0;
	CHAR *pcFlashBuf = NULL;
	UINT uiSectorPos = 0;
	UINT uiSectorOff = 0;
	UINT uiRemain = 0;

	uiSectorPos = uiAddr / FLASH_SEC_SIZE;	//读出的扇区
	uiSectorOff = uiAddr % FLASH_SEC_SIZE;	//扇区内的偏移地址
	uiRemain = FLASH_SEC_SIZE - uiSectorOff;	//扇区剩余大小

	if ( NULL == pcBuf )
	{
	    LOG_OUT(LOGOUT_ERROR, "FlASH_Read failed, pcBuf is NULL.");
		return FAIL;
	}

	if ( uiLen == 0 )
	{
		return OK;
	}

	pcFlashBuf = malloc( FLASH_SEC_SIZE );
	if ( NULL == pcFlashBuf )
	{
	    LOG_OUT(LOGOUT_ERROR, "malloc failed, pcFlashBuf is NULL.");
		return FAIL;
	}

	//判断读出数据长度是否大于扇区剩余大小，如果是一次就可以读出完成，否则需要连续读取多次
	uiRemain = uiLen >= uiRemain ? uiRemain : uiLen;
	while( 1 )
	{
		//因为只能按扇区大小为单位操作，所先要读出整个扇区的数据
		uiRet = spi_flash_read((UINT)uiSectorPos*FLASH_SEC_SIZE, (UINT*)pcFlashBuf, FLASH_SEC_SIZE);
		if ( SPI_FLASH_RESULT_OK != uiRet )
		{
		    LOG_OUT(LOGOUT_ERROR, "spi_flash_read failed, addr:0x%X, uiRet:%d", uiSectorPos*FLASH_SEC_SIZE, uiRet);
		    FREE_MEM(pcFlashBuf);
		    return FAIL;
		}

		//读出数据
		for ( uiLoop = 0; uiLoop < uiRemain; uiLoop ++ )
		{
			pcBuf[uiLoop] = pcFlashBuf[uiSectorOff + uiLoop];
		}

		if ( uiRemain == uiLen )
		{
			break;
		}
		else
		{
			uiSectorPos ++;
			uiSectorOff = 0;

			pcBuf 	+= uiRemain;
			uiAddr 	+= uiRemain;
			uiLen   -= uiRemain;

			uiRemain = uiLen > FLASH_SEC_SIZE ? FLASH_SEC_SIZE : uiLen;
		}
	}

	FREE_MEM(pcFlashBuf);
	return OK;
}


UINT FlASH_Erase( UINT uiAddr, UINT uiLen )
{
	UINT uiRet = 0;
	UINT uiLoop = 0;
	CHAR *pcFlashBuf = NULL;
	UINT uiSectorPos = 0;
	UINT uiSectorOff = 0;
	UINT uiRemain = 0;

	uiSectorPos = uiAddr / FLASH_SEC_SIZE;	//擦除的扇区
	uiSectorOff = uiAddr % FLASH_SEC_SIZE;	//扇区内的偏移地址
	uiRemain = FLASH_SEC_SIZE - uiSectorOff;//扇区剩余大小

	pcFlashBuf = malloc( FLASH_SEC_SIZE );
	if ( NULL == pcFlashBuf )
	{
	    LOG_OUT(pcFlashBuf, "malloc pcFlashBuf is NULL.");
		return FAIL;
	}

	//判断写入数据长度是否大于扇区剩余大小，如果是首次只写入剩余扇区大小剩余的下次再写入，否则就写入要写入的数据长度
	uiRemain = uiLen >= uiRemain ? uiRemain : uiLen;
	while( 1 )
	{
		if ( uiRemain == FLASH_SEC_SIZE)
		{
			uiRet = spi_flash_erase_sector(uiSectorPos);
			if ( SPI_FLASH_RESULT_OK != uiRet )
			{
			    LOG_OUT(LOGOUT_ERROR, "spi_flash_erase_sector failed, uiSectorPos:%d, uiRet:%d", uiSectorPos, uiRet);
			    FREE_MEM(pcFlashBuf);
			    return FAIL;
			}
		}
		else
		{
			uiRet = spi_flash_read((UINT)uiSectorPos*FLASH_SEC_SIZE, (UINT*)pcFlashBuf, FLASH_SEC_SIZE);
			if ( SPI_FLASH_RESULT_OK != uiRet )
			{
			    LOG_OUT(LOGOUT_ERROR, "spi_flash_read failed, addr:0x%X, uiRet:%d", uiSectorPos*FLASH_SEC_SIZE, uiRet);
			    FREE_MEM(pcFlashBuf);
			    return FAIL;
			}

			//修改要写入的数据
			for ( uiLoop = 0; uiLoop < uiRemain; uiLoop ++ )
			{
				pcFlashBuf[uiSectorOff + uiLoop] = 0xFF;
			}

			//写入前先擦除整个扇区
			uiRet = spi_flash_erase_sector(uiSectorPos);
			if ( SPI_FLASH_RESULT_OK != uiRet )
			{
			    LOG_OUT(LOGOUT_ERROR, "spi_flash_erase_sector failed, uiSectorPos:%d, uiRet:%d", uiSectorPos, uiRet);
			    FREE_MEM(pcFlashBuf);
			    return FAIL;
			}

			//写入修改后的数据
			uiRet = spi_flash_write((UINT)uiSectorPos*FLASH_SEC_SIZE, (UINT*)pcFlashBuf, FLASH_SEC_SIZE);
			if ( SPI_FLASH_RESULT_OK != uiRet )
			{
			    LOG_OUT(LOGOUT_ERROR, "spi_flash_write failed, addr:0x%X, uiRet:%d", uiSectorPos*FLASH_SEC_SIZE, uiRet);
			    FREE_MEM(pcFlashBuf);
			    return FAIL;
			}
		}

		if ( uiRemain == uiLen )
		{
			break;
		}
		else
		{
			uiSectorPos ++;
			uiSectorOff = 0;

			uiAddr 	+= uiRemain;
			uiLen   -= uiRemain;

			uiRemain = uiLen > FLASH_SEC_SIZE ? FLASH_SEC_SIZE : uiLen;
		}
	}

	FREE_MEM(pcFlashBuf);
	return OK;
}






