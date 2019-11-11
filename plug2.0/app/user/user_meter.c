/*
 * user_key.c
 *
 *  Created on: 2018年10月21日
 *      Author: lenovo
 */
#include "esp_common.h"
#include "user_common.h"


UINT VolParReg  	= 0;    // 电压参数寄存器
UINT VolPar   		= 0;    // 电压寄存器
UINT CurrentParReg  = 0;    // 电流参数寄存器
UINT CurrentPar   	= 0;    // 电流寄存器
UINT PowerParReg	= 0;    // 功率参数寄存器
UINT PowerPar   	= 0;    // 功率寄存器
UINT PFReg   		= 0;	// PF寄存器
UINT DataUpdateReg	= 0;	// 数据更新寄存器
UINT StateReg		= 0;	// 状态寄存器
UINT CheckSumReg	= 0;	// 校验和寄存器

static UINT8  ucRecvData[48];
static UINT8  ucRecvDataCnt = 0;

//重新上电后的计量信息
METER_MerterInfo stRealMeterInfo;

//历史计量信息
METER_MerterInfo stRecordMeterInfo;

UINT uiPowerDownFlag = 0;

LOCAL VOID METER_RecvData(void *para);
VOID METER_DataAnalysis( UINT8 *ucBuf );
VOID METER_PowerUpHandle( VOID );
VOID METER_PowerDownHandle( VOID );
VOID MTER_StartEraseMeterDataTimer();
VOID MTER_EraseMeterData();
UINT METER_WriteMeterDataToFlash(METER_MerterInfo *pstMeter);


VOID METER_DeinitData( VOID )
{
	UINT uiRet = 0;

	stRecordMeterInfo.fVoltage = 0;
	stRecordMeterInfo.fCurrent = 0;
	stRecordMeterInfo.fPower = 0;
	stRecordMeterInfo.fApparentPower = 0;
	stRecordMeterInfo.fPowerFactor = 0;
	stRecordMeterInfo.fElectricity = 0;
	stRecordMeterInfo.fRunTime = 0;

	uiRet = FlASH_Write(FLASH_METER_ADDR, (CHAR*)&stRecordMeterInfo, sizeof(stRecordMeterInfo));
	if ( OK != uiRet )
	{
		LOG_OUT(LOGOUT_ERROR, "FlASH_Write meter data failed.");
	}

	//保存到备份分区
	uiRet = FlASH_Write(FLASH_METER_BK_ADDR, (CHAR*)&stRecordMeterInfo, sizeof(stRecordMeterInfo));
	if ( OK != uiRet )
	{
		LOG_OUT(LOGOUT_ERROR, "FlASH_Write meter backup data failed.");
	}
}

//从FLASH加载计量数据
UINT METER_GetMeterData( VOID )
{
	UINT uiRet = 0;

    uiRet = FlASH_Read((UINT32)FLASH_METER_ADDR, (CHAR*)&stRecordMeterInfo, sizeof(stRecordMeterInfo));
	if ( OK != uiRet )
	{
	    LOG_OUT(LOGOUT_ERROR, "FlASH_Read meter data failed.");
	}

	if ( (UINT32)stRecordMeterInfo.fVoltage != 0 ||  (UINT32)stRecordMeterInfo.fCurrent != 0 || (UINT32)stRecordMeterInfo.fPower != 0 ||
		 (UINT32)stRecordMeterInfo.fApparentPower != 0 || (UINT32)stRecordMeterInfo.fPowerFactor != 0 )
	{
		LOG_OUT(LOGOUT_ERROR, "meter data is unavailable");

	    uiRet = FlASH_Read((UINT32)FLASH_METER_BK_ADDR, (CHAR*)&stRecordMeterInfo, sizeof(stRecordMeterInfo));
		if ( OK != uiRet )
		{
			METER_DeinitData();
		    LOG_OUT(LOGOUT_ERROR, "FlASH_Read meter backup data failed.");
		}
		else if ( stRecordMeterInfo.fVoltage != 0 ||  stRecordMeterInfo.fCurrent != 0 || stRecordMeterInfo.fPower != 0 ||
			      stRecordMeterInfo.fApparentPower != 0 || stRecordMeterInfo.fPowerFactor != 0 )
		{
			METER_DeinitData();
			LOG_OUT(LOGOUT_ERROR, "meter backup data is unavailable");
			uiRet = FAIL;
		}
	}

	return uiRet;
}

UINT METER_SetMeterData( METER_MerterInfo *pstMeter )
{
	if ( pstMeter == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstMeter is NULL");
		return FAIL;
	}

	stRecordMeterInfo.fRunTime     = pstMeter->fRunTime - PLUG_GetRunTime() * 1.0 / 3600 + 0.0001;
	stRecordMeterInfo.fElectricity = pstMeter->fElectricity - stRealMeterInfo.fElectricity + 0.0001;

	METER_WriteMeterDataToFlash(&stRecordMeterInfo);

	//擦除保存计量数据的扇区下电时没有时间擦除，提前擦除数据下电时可以直接保存数据不用再擦除了
	MTER_EraseMeterData();

	return OK;
}


//上电进行的操作.1,从flash读取历史数据  2,提前擦除保存计量信息的FLASH，以便于掉电时直接写入
VOID METER_PowerUpHandle( VOID )
{
	UINT uiRet = 0;

	uiRet = METER_GetMeterData();
	if ( OK != uiRet )
	{
	    LOG_OUT(LOGOUT_ERROR, "METER_GetMeterData data failed.");
	}
	else
	{
		//保存到备份分区
		uiRet = FlASH_Write(FLASH_METER_BK_ADDR, (CHAR*)&stRecordMeterInfo, sizeof(stRecordMeterInfo));
		if ( OK != uiRet )
		{
			LOG_OUT(LOGOUT_ERROR, "FlASH_Write meter backup data failed.");
		}
	}

	//擦除保存计量数据的扇区下电时没有时间擦除，提前擦除数据下电时可以直接保存数据不用再擦除了
	MTER_EraseMeterData();
}

VOID METER_PowerDownHandle( VOID )
{
	UINT uiRet = 0;
	METER_MerterInfo stMeter;

	memset(&stMeter, 0, sizeof(stMeter));
	stMeter.fRunTime     = stRecordMeterInfo.fRunTime + PLUG_GetRunTime() * 1.0 / 3600;
	stMeter.fElectricity = stRecordMeterInfo.fElectricity + stRealMeterInfo.fElectricity;

	uiRet = spi_flash_write((UINT)FLASH_METER_ADDR, (UINT*)&stMeter, sizeof(stMeter));
	if ( SPI_FLASH_RESULT_OK != uiRet )
	{
		LOG_OUT(LOGOUT_ERROR, "write meter data failed, addr:0x%X, uiRet:%d", FLASH_METER_ADDR, uiRet);
	}
}

VOID METER_RestartHandle( VOID )
{
	UINT uiRet = 0;
	METER_MerterInfo stMeter;

	memset(&stMeter, 0, sizeof(stMeter));
	stMeter.fRunTime     = stRecordMeterInfo.fRunTime + PLUG_GetRunTime() * 1.0 / 3600;
	stMeter.fElectricity = stRecordMeterInfo.fElectricity;

	METER_WriteMeterDataToFlash(&stMeter);
}

UINT METER_WriteMeterDataToFlash(METER_MerterInfo *pstMeter)
{
	UINT uiRet = 0;

	if ( pstMeter == NULL )
	{
		LOG_OUT(LOGOUT_ERROR, "pstMeter is NULL");
		return FAIL;
	}

	uiRet = FlASH_Write(FLASH_METER_ADDR, (CHAR*)pstMeter, sizeof(METER_MerterInfo));
	if ( OK != uiRet )
	{
		LOG_OUT(LOGOUT_ERROR, "FlASH_Write meter data failed.");
	}

	uiRet = FlASH_Write(FLASH_METER_BK_ADDR, (CHAR*)pstMeter, sizeof(METER_MerterInfo));
	if ( OK != uiRet )
	{
		LOG_OUT(LOGOUT_ERROR, "FlASH_Write meter backup data failed.");
	}

	return uiRet;
}

VOID METER_GpioInterrupt( VOID* Para )
{
	_xt_isr_mask(1 << ETS_GPIO_INUM);

	UINT uiStartTime, uiStopTime;

	uiStartTime = system_get_time();
	METER_PowerDownHandle();
	uiStopTime = system_get_time();

	LOG_OUT(LOGOUT_INFO, "Power down detected, saved meter data cost %d us", (uiStopTime - uiStartTime));

	MTER_StartEraseMeterDataTimer();

    GPIO_REG_WRITE( GPIO_STATUS_W1TC_ADDRESS, GPIO_Pin_4 );
    _xt_isr_unmask(1 << ETS_GPIO_INUM);
}

VOID MTER_MeterDataRecvTimerHandle()
{
	ETS_UART_INTR_ENABLE();
}

VOID MTER_StartMeterDataRecvTimer()
{
	xTimerHandle xTimers = NULL;

	xTimers = xTimerCreate("MTER_StartMeterDataRecvTimer", 500/portTICK_RATE_MS, TRUE, NULL, MTER_MeterDataRecvTimerHandle);
	if ( !xTimers )
	{
		LOG_OUT(LOGOUT_ERROR, "xTimerCreate MTER_MeterDataRecvTimerHandle failed.");
	}
	else
	{
		if(xTimerStart(xTimers, 0) != pdPASS)
	    {
			LOG_OUT(LOGOUT_ERROR, "xTimerCreate MTER_MeterDataRecvTimerHandle start failed.");
		}
	}
}

//初始化串口中断用于接收计量信息
VOID METER_InitUart()
{
	ETS_UART_INTR_DISABLE();
    UART_WaitTxFifoEmpty(UART0);

    UART_ConfigTypeDef uart_config;
    uart_config.baud_rate			= BIT_RATE_4800;
    uart_config.data_bits			= UART_WordLength_8b;
    uart_config.parity				= USART_Parity_Odd;
    uart_config.stop_bits			= USART_StopBits_1;
    uart_config.flow_ctrl			= USART_HardwareFlowControl_None;
    uart_config.UART_RxFlowThresh 	= 120;
    uart_config.UART_InverseMask	= UART_None_Inverse;
    UART_ParamConfig(UART0, &uart_config);

    UART_IntrConfTypeDef uart_intr;
    uart_intr.UART_IntrEnMask = UART_RXFIFO_TOUT_INT_ENA | UART_FRM_ERR_INT_ENA | UART_RXFIFO_FULL_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA;
    uart_intr.UART_RX_FifoFullIntrThresh = 24;
    uart_intr.UART_RX_TimeOutIntrThresh = 2;
    uart_intr.UART_TX_FifoEmptyIntrThresh = 20;
    UART_IntrConfig(UART0, &uart_intr);

    UART_intr_handler_register( METER_RecvData, NULL );
}

//初始化外部中断，断电时进行数据的保存
VOID METER_InitGpioInterrupt()
{
	GPIO_ConfigTypeDef stGpioCfg;
	stGpioCfg.GPIO_IntrType = GPIO_PIN_INTR_NEGEDGE;
	stGpioCfg.GPIO_Mode 	= GPIO_Mode_Input;
	stGpioCfg.GPIO_Pullup 	= GPIO_PullUp_EN;
	stGpioCfg.GPIO_Pin 		= GPIO_Pin_4;
	gpio_config( &stGpioCfg );

	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, GPIO_Pin_4);
	gpio_intr_handler_register(METER_GpioInterrupt, NULL);
}

VOID METER_Init()
{
	METER_PowerUpHandle();

	METER_InitUart();
	METER_InitGpioInterrupt();

    //每隔一定时间打开一次串口中断来接收一次数据
    MTER_StartMeterDataRecvTimer();
}


LOCAL VOID METER_RecvData(void *para)
{
    uint8 ucFifoLen = 0;
    uint8 i, j;

    uint32 uiStatus = READ_PERI_REG(UART_INT_ST(UART0)) ;

    while (uiStatus != 0x0)
    {
        if (UART_FRM_ERR_INT_ST == (uiStatus & UART_FRM_ERR_INT_ST))
        {
        	printf("FRM_ERR\r\n");
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_FRM_ERR_INT_CLR);
        }
        else if ( (UART_RXFIFO_FULL_INT_ST == (uiStatus & UART_RXFIFO_FULL_INT_ST)) ||
        		  (UART_RXFIFO_TOUT_INT_ST == (uiStatus & UART_RXFIFO_TOUT_INT_ST)) )
        {
        	ucFifoLen = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;

        	for ( i = 0; i < ucFifoLen && ucRecvDataCnt < sizeof(ucRecvData); i++ )
        	{
        		ucRecvData[ ucRecvDataCnt++ ] = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
        	}

            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);

            if ( ucRecvDataCnt < 48 )
            {
            	return;
            }

            //寻找数据头
        	for ( i = 0; i < ucRecvDataCnt; i++ )
        	{
        		if ( (((ucRecvData[i] & 0xF0) == 0xF0) || (ucRecvData[i] == 0xAA) || (ucRecvData[i] == 0x55)) && (ucRecvData[i+1] == 0x5A))
				{
        			break;
				}
        	}

        	//没有找到数据头
        	if ( ucRecvDataCnt == i )
        	{
        		// do nothing
        	}
        	//找到数据头,且数据完整
        	else if ( ucRecvDataCnt - i >= 23)
        	{
        	    ETS_UART_INTR_DISABLE();
        		METER_DataAnalysis( ucRecvData+i );

        		//for ( j = 0; j < ucRecvDataCnt-i-24; j++ )
        		//{
        		//	ucRecvData[j] = ucRecvData[i+j+24];
        		//}
        		//ucRecvDataCnt = j;
        		ucRecvDataCnt = 0;
        	}
        	// 找到数据头，但数据不完整
        	else
        	{
        		//for ( j = 0; j < ucRecvDataCnt-i; j++ )
        		//{
        		//	ucRecvData[j] = ucRecvData[i+j];
        		//}
        		//ucRecvDataCnt = j;
        	}

        	if ( ucRecvDataCnt >= sizeof(ucRecvData) )
    		{
    			printf("ucRecvData full:");
    			for ( i = 0; i < ucRecvDataCnt; i++ )
    			{
    				printf("%02X ", ucRecvData[i]);
    			}
    			printf("\r\n");
    			ucRecvDataCnt = 0;
    		}
        }
        else if (UART_TXFIFO_EMPTY_INT_ST == (uiStatus & UART_TXFIFO_EMPTY_INT_ST))
        {
        	printf("empty\r\n");
            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_TXFIFO_EMPTY_INT_CLR);
            CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
        }
        else
        {
        	printf("unkown\n");
        }

        uiStatus = READ_PERI_REG(UART_INT_ST(UART0)) ;
    }
}

VOID METER_DataAnalysis( UINT8 *ucBuf )
{
	UINT8 i = 0;
	UINT8 ucSum = 0;

	//上一次 数据更新寄存器(Data Updata REG)bit7状态
	static UINT8 ucPFRegBit7Last = 0xFF;


	//寄存器1: 状态寄存器(1Byte, 0) 不需参与计算校验值
	StateReg = ucBuf[0];
	//寄存器2: 检测寄存器(1Byte, 1) 不需参与计算校验值
	//ucBuf[2] = 0x5A 固定值

	//寄存器9: 数据更新寄存器(1Byte, 20)
	DataUpdateReg = ucBuf[20];
	//寄存器11: 校验和寄存器(1Byte, 23) 不需参与计算校验值
	CheckSumReg = ucBuf[23];

	//状态寄存器为0xAA芯片误差修正功能失效，此时电压参数寄存器、电流参数寄存器和功率参数寄存器不可用
	//状态寄存器为0x55芯片误差修正功能正常，此时电压参数寄存器、电流参数寄存器和功率参数寄存器可用,且电压寄存器、电流寄存器和功率寄存器未溢出
	//状态寄存器为0xFx芯片误差修正功能正常, 此时电压参数寄存器、电流参数寄存器和功率参数寄存器可用,相应位为1时表示相应的寄存器溢出，溢出表示电流、电压或功率值非常小，接近0;Bit0=1时电压参数寄存器、电流参数寄存器和功率参数器寄存器不能使用
	if ( StateReg == 0xAA || (StateReg != 0x55 && (StateReg & 1)) )
	{
		LOG_OUT(LOGOUT_ERROR, "VolParReg, CurrentParReg and PowerParReg is unavailable, StateReg:0X%X", StateReg );
		return;
	}

	//除状态寄存器(State REG)、检测寄存器(Check REG)和校验和寄存器(CheckSum REG)之外的寄存器的数据之和的低8bit
	for ( i = 2; i < 23; i++ )
	{
		ucSum = ucSum + ucBuf[i];
	}
	if ( ucSum != CheckSumReg )
	{
		//LOG_OUT(LOGOUT_INFO, "ucSum(%X) != CheckSumReg(%X)", ucSum, CheckSumReg);
		//printf("ucBuf: ");
		//for ( i = 0; i < 48; i++ )
		//{
		//	printf("%02X ", ucBuf[i]);
		//}
		//printf("\r\n");
		return;
	}

	if ( ucPFRegBit7Last == 0xFF )
	{
		ucPFRegBit7Last = DataUpdateReg & (1<<7);
	}

	//寄存器3: 电压参数寄存器(3Byte, 2-4)
	VolParReg = (ucBuf[2]<<16) + (ucBuf[3]<<8) + ucBuf[4];
	//寄存器4: 电压寄存器(3Byte, 5-7)
	VolPar = (ucBuf[5]<<16) + (ucBuf[6]<<8) + ucBuf[7];
	//寄存器5: 电流参数寄存器(3Byte, 8-10)
	CurrentParReg =(ucBuf[8]<<16) + (ucBuf[9]<<8) + ucBuf[10];
	//寄存器6: 电流寄存器(3Byte, 11-13)
	CurrentPar  = (ucBuf[11]<<16) + (ucBuf[12]<<8) + ucBuf[13];
	//寄存器7: 功率参数寄存器(3Byte, 14-16)
	PowerParReg = (ucBuf[14]<<16) + (ucBuf[15]<<8) + ucBuf[16];
	//寄存器8: 功率寄存器(3Byte, 17-19)
	PowerPar  = (ucBuf[17]<<16) + (ucBuf[18]<<8) + ucBuf[19];
	//寄存器10: PF寄存器(2Byte, 21-22)
	PFReg = (ucBuf[21]<<8) + ucBuf[22];

	//Bit6 电压寄存器状态标志位  0:数据未更新 1:数据己更新
	if ( DataUpdateReg & (1<<6) )
	{
		stRealMeterInfo.fVoltage = VolParReg * 1.88 / VolPar;
	}

	//Bit5 电流寄存器状态标志位    0:数据未更新 1:数据己更新
	if ( DataUpdateReg & (1<<5) )
	{
		stRealMeterInfo.fCurrent = CurrentParReg * 1.0 / CurrentPar;
	}

	//有功功率, Bit4 功率寄存器状态标志位    0:数据未更新 1:数据己更新
	if ( DataUpdateReg & (1<<4) )
	{
		stRealMeterInfo.fPower = PowerParReg * 1.88 / PowerPar;
	}

	//电流寄存器溢出,接近0
	if ( (StateReg != 0x55) && (StateReg & (1<<2)) )
	{
		stRealMeterInfo.fCurrent = 0;
	}

	//电压寄存器溢出,接近0
	if ( (StateReg != 0x55) && (StateReg & (1<<3)) )
	{
		stRealMeterInfo.fVoltage = 0;
	}

	//功率寄存器溢出,接近0
	if ( (StateReg != 0x55) && (StateReg & (1<<1)) )
	{
		stRealMeterInfo.fPower = 0;
		stRealMeterInfo.fCurrent = stRealMeterInfo.fCurrent - 0.077;
		stRealMeterInfo.fCurrent = (stRealMeterInfo.fCurrent < 0) ? 0.00001 : stRealMeterInfo.fCurrent;
	}
	else
	{
		stRealMeterInfo.fCurrent = stRealMeterInfo.fCurrent - 0.0045;
		stRealMeterInfo.fCurrent = (stRealMeterInfo.fCurrent < 0) ? 0.00001 : stRealMeterInfo.fCurrent;
	}

	//视在功率 = 有效电压×有效电流, 当电压和电流寄存器都更新时才计算视在功率
	if ( (DataUpdateReg & (1<<6)) && (DataUpdateReg & (1<<5)))
	{
		stRealMeterInfo.fApparentPower = stRealMeterInfo.fVoltage * stRealMeterInfo.fCurrent;
	}

	//PFReg已满清零，需要将更新stRecordMeterInfo信息并保存在falsh
	if ( (DataUpdateReg & (1<<7)) != ucPFRegBit7Last)
	{
		ucPFRegBit7Last = DataUpdateReg & (1<<7);

		stRecordMeterInfo.fElectricity += (65536 * 1.0/3600) * (PowerParReg/1000000*1.88);
		METER_WriteMeterDataToFlash(&stRecordMeterInfo);

		LOG_OUT(LOGOUT_INFO, "Update stRecordMeterInfo.fElectricity: %3.3f", stRecordMeterInfo.fElectricity);

		//擦除保存计量数据的扇区下电时没有时间擦除，提前擦除数据下电时可以直接保存数据不用再擦除了
		MTER_EraseMeterData();
	}

	//计算电量
	stRealMeterInfo.fElectricity = (PFReg*1.0/3600) * (PowerParReg/1000000*1.88);
	//功率因数 = 有功功率 / 视在功率
	stRealMeterInfo.fPowerFactor = stRealMeterInfo.fPower / stRealMeterInfo.fApparentPower;
	stRealMeterInfo.fPowerFactor = stRealMeterInfo.fPowerFactor > 1 ? 1 : stRealMeterInfo.fPowerFactor;

	LOG_OUT(LOGOUT_INFO, "ucPFRegBit7Last:%X, PFReg:%X, PowerParReg:%X, stRecordMeterInfo:%3.3f, stRealMeterInfo:%3.3f",
			ucPFRegBit7Last, PFReg, PowerParReg, stRecordMeterInfo.fElectricity, stRealMeterInfo.fElectricity);
}

UINT METER_MarshalJsonMeter( CHAR* pcBuf, UINT uiBufLen )
{
	cJSON  *pJson = NULL;
	CHAR *pJsonStr = NULL;
	CHAR szBuf[20] = {0};

	pJson = cJSON_CreateObject();

	sprintf(szBuf, "%3.3f", stRealMeterInfo.fVoltage);
	cJSON_AddStringToObject( pJson, "Voltage", 			szBuf);

	sprintf(szBuf, "%3.3f", stRealMeterInfo.fCurrent);
	cJSON_AddStringToObject( pJson, "Current", 			szBuf);

	sprintf(szBuf, "%3.3f", stRealMeterInfo.fPower);
	cJSON_AddStringToObject( pJson, "Power", 			szBuf);

	sprintf(szBuf, "%3.3f", stRealMeterInfo.fApparentPower);
	cJSON_AddStringToObject( pJson, "ApparentPower", 	szBuf);

	sprintf(szBuf, "%3.3f", stRealMeterInfo.fPowerFactor);
	cJSON_AddStringToObject( pJson, "PowerFactor", 		szBuf);

	sprintf(szBuf, "%3.3f", stRecordMeterInfo.fElectricity + stRealMeterInfo.fElectricity);
	cJSON_AddStringToObject( pJson, "Electricity", 		szBuf);

	sprintf(szBuf, "%3.3f", stRecordMeterInfo.fRunTime + PLUG_GetRunTime() * 1.0 / 3600);
	cJSON_AddStringToObject( pJson, "RunTime", 			szBuf);

    pJsonStr = cJSON_PrintUnformatted(pJson);
    strncpy(pcBuf, pJsonStr, uiBufLen);
    cJSON_Delete(pJson);
    FREE_MEM(pJsonStr);

	return strlen(pcBuf);
}


UINT METER_ParseMeterData( CHAR* pDataStr)
{
	cJSON *pJsonRoot = NULL;
	cJSON *pJson = NULL;
	METER_MerterInfo stMeter;

	if ( pDataStr == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "pDataStr is NULL.");
	    return FAIL;
	}

	pJsonRoot = cJSON_Parse( pDataStr );
	if ( pJsonRoot == NULL )
	{
	    LOG_OUT(LOGOUT_ERROR, "cJSON_Parse failed, pDateStr:%s.", pDataStr);
	    goto error;
	}

	pJson = cJSON_GetObjectItem(pJsonRoot, "Electricity");
	if ( pJson != NULL && pJson->type == cJSON_String )
	{
		LOG_OUT(LOGOUT_INFO, "%s", pJson->valuestring);
		sscanf(pJson->valuestring, "%f", &stMeter.fElectricity);
		LOG_OUT(LOGOUT_INFO, "%3.3f", stMeter.fElectricity);
	}

	pJson = cJSON_GetObjectItem(pJsonRoot, "RunTime");
	if ( pJson != NULL && pJson->type == cJSON_String )
	{
		LOG_OUT(LOGOUT_INFO, "%s", pJson->valuestring);
		sscanf(pJson->valuestring, "%f", &stMeter.fRunTime);
		LOG_OUT(LOGOUT_INFO, "%3.3f", stMeter.fRunTime);
	}

	if ( METER_SetMeterData(&stMeter) != OK )
	{
		LOG_OUT(LOGOUT_ERROR, "METER_SetMeterData failed.");
		goto error;
	}

succ:
	cJSON_Delete(pJsonRoot);
	return OK;

error:
	cJSON_Delete(pJsonRoot);
	return FAIL;
}

//擦除保存计量数据的扇区下电时没有时间擦除，提前擦除数据下电时可以直接保存数据不用再擦除了
VOID MTER_EraseMeterData()
{
	if ( SPI_FLASH_RESULT_OK != spi_flash_erase_sector(FLASH_METER_ADDR/FLASH_SEC_SIZE) )
	{
	    LOG_OUT(LOGOUT_ERROR, "spi_flash_erase_sector failed, addr:0X%x", FLASH_METER_ADDR);
	}
	else
	{
		LOG_OUT(LOGOUT_INFO, "spi_flash_erase_sector success, addr:0X%x", FLASH_METER_ADDR);
	}
}


VOID MTER_StartEraseMeterDataTimer()
{
	xTimerHandle xTimers = NULL;

	xTimers = xTimerCreate("MTER_StartEraseMeterDataTimer", 3000/portTICK_RATE_MS, FALSE, NULL, MTER_EraseMeterData);
	if ( !xTimers )
	{
		LOG_OUT(LOGOUT_ERROR, "xTimerCreate MTER_EraseMeterData failed.");
	}
	else
	{
		if(xTimerStart(xTimers, 0) != pdPASS)
	    {
			LOG_OUT(LOGOUT_ERROR, "xTimerCreate MTER_EraseMeterData start failed.");
		}
	}
}




