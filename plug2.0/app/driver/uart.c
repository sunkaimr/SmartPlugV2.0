/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "uart.h"
#include "user_common.h"
#include "esp_common.h"

enum {
    UART_EVENT_RX_CHAR,
    UART_EVENT_MAX
};

typedef struct _os_event_ {
    uint32 event;
    uint32 param;
} os_event_t;

xTaskHandle xUartTaskHandle;
xQueueHandle xQueueUart;

#define UART_RECVMAXCNT          (100)      	//接收缓冲区的最大值
#define UART_SENDMAXCNT 		 (1024*3)

uint8_t  Uart_RecvBuf[UART_RECVMAXCNT+1];
uint8_t  Uart_SendBuf[UART_SENDMAXCNT+1];
uint32_t Uart_RecvCnt = 0;

LOCAL STATUS
uart_tx_one_char(uint8 uart, uint8 TxChar)
{
    while (true) {
        uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(uart)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);

        if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
            break;
        }
    }

    WRITE_PERI_REG(UART_FIFO(uart) , TxChar);
    return OK;
}

LOCAL void
uart1_write_char(char c)
{
    if (c == '\n') {
        uart_tx_one_char(UART1, '\r');
        uart_tx_one_char(UART1, '\n');
    } else if (c == '\r') {
    } else {
        uart_tx_one_char(UART1, c);
    }
}

LOCAL void
uart0_write_char(char c)
{
    if (c == '\n') {
        uart_tx_one_char(UART0, '\r');
        uart_tx_one_char(UART0, '\n');
    } else if (c == '\r') {
    } else {
        uart_tx_one_char(UART0, c);
    }
}

LOCAL void
uart_rx_intr_handler_ssc(void *arg)
{
    /* uart0 and uart1 intr combine togther, when interrupt occur, see reg 0x3ff20020, bit2, bit0 represents
      * uart1 and uart0 respectively
      */
    os_event_t e;
    portBASE_TYPE xHigherPriorityTaskWoken;

    uint8 RcvChar;
    uint8 uart_no = 0;

    if (UART_RXFIFO_FULL_INT_ST != (READ_PERI_REG(UART_INT_ST(uart_no)) & UART_RXFIFO_FULL_INT_ST)) {
        return;
    }

    RcvChar = READ_PERI_REG(UART_FIFO(uart_no)) & 0xFF;

    WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_RXFIFO_FULL_INT_CLR);

    e.event = UART_EVENT_RX_CHAR;
    e.param = RcvChar;

    xQueueSendFromISR(xQueueUart, (void *)&e, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

#if 0
LOCAL void
uart_config(uint8 uart_no, UartDevice *uart)
{
    if (uart_no == UART1) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
    } else {
        /* rcv_buff size if 0x100 */
        _xt_isr_attach(ETS_UART_INUM, uart_rx_intr_handler_ssc, NULL);
        PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
    }

    uart_div_modify(uart_no, UART_CLK_FREQ / (uart->baut_rate));

    WRITE_PERI_REG(UART_CONF0(uart_no), uart->exist_parity
                   | uart->parity
                   | (uart->stop_bits << UART_STOP_BIT_NUM_S)
                   | (uart->data_bits << UART_BIT_NUM_S));

    //clear rx and tx fifo,not ready
    SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);

    if (uart_no == UART0) {
        //set rx fifo trigger
        WRITE_PERI_REG(UART_CONF1(uart_no),
                       ((0x01 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S));
    } else {
        WRITE_PERI_REG(UART_CONF1(uart_no),
                       ((0x01 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S));
    }

    //clear all interrupt
    WRITE_PERI_REG(UART_INT_CLR(uart_no), 0xffff);
    //enable rx_interrupt
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA);
}
#endif

LOCAL void
uart_task(void *pvParameters)
{
    os_event_t e;

    for (;;) {
        if (xQueueReceive(xQueueUart, (void *)&e, (portTickType)portMAX_DELAY)) {
            switch (e.event) {
                case UART_EVENT_RX_CHAR:
                    printf("%c", e.param);
                    break;

                default:
                    break;
            }
        }
    }

    vTaskDelete(NULL);
}

#if 0
void
uart_init(void)
{
    while (READ_PERI_REG(UART_STATUS(0)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S));

    while (READ_PERI_REG(UART_STATUS(1)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S));

    UART_ConfigTypeDef uart;

    uart.baut_rate    = BIT_RATE_74880;
    uart.data_bits    = UART_WordLength_8b;
    uart.flow_ctrl    = USART_HardwareFlowControl_None;
    // uart.exist_parity = PARITY_DIS;
    uart.parity       = USART_Parity_None;
    uart.stop_bits    = USART_StopBits_1;

    uart_config(UART0, &uart);
    uart_config(UART1, &uart);

    os_install_putc1(uart1_write_char);

    _xt_isr_unmask(1 << ETS_UART_INUM);

    xQueueUart = xQueueCreate(32, sizeof(os_event_t));

    xTaskCreate(uart_task, (uint8 const *)"uTask", 512, NULL, tskIDLE_PRIORITY + 2, &xUartTaskHandle);
}
#endif

//=================================================================

void
UART_SetWordLength(UART_Port uart_no, UART_WordLength len)
{
    SET_PERI_REG_BITS(UART_CONF0(uart_no), UART_BIT_NUM, len, UART_BIT_NUM_S);
}

void
UART_SetStopBits(UART_Port uart_no, UART_StopBits bit_num)
{
    SET_PERI_REG_BITS(UART_CONF0(uart_no), UART_STOP_BIT_NUM, bit_num, UART_STOP_BIT_NUM_S);
}

void
UART_SetLineInverse(UART_Port uart_no, UART_LineLevelInverse inverse_mask)
{
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_LINE_INV_MASK);
    SET_PERI_REG_MASK(UART_CONF0(uart_no), inverse_mask);
}

void
UART_SetParity(UART_Port uart_no, UART_ParityMode Parity_mode)
{
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_PARITY | UART_PARITY_EN);

    if (Parity_mode == USART_Parity_None) {
    } else {
        SET_PERI_REG_MASK(UART_CONF0(uart_no), Parity_mode | UART_PARITY_EN);
    }
}

void
UART_SetBaudrate(UART_Port uart_no, uint32 baud_rate)
{
    uart_div_modify(uart_no, UART_CLK_FREQ / baud_rate);
}

//only when USART_HardwareFlowControl_RTS is set , will the rx_thresh value be set.
void
UART_SetFlowCtrl(UART_Port uart_no, UART_HwFlowCtrl flow_ctrl, uint8 rx_thresh)
{
    if (flow_ctrl & USART_HardwareFlowControl_RTS) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);
        SET_PERI_REG_BITS(UART_CONF1(uart_no), UART_RX_FLOW_THRHD, rx_thresh, UART_RX_FLOW_THRHD_S);
        SET_PERI_REG_MASK(UART_CONF1(uart_no), UART_RX_FLOW_EN);
    } else {
        CLEAR_PERI_REG_MASK(UART_CONF1(uart_no), UART_RX_FLOW_EN);
    }

    if (flow_ctrl & USART_HardwareFlowControl_CTS) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);
        SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_TX_FLOW_EN);
    } else {
        CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_TX_FLOW_EN);
    }
}

void
UART_WaitTxFifoEmpty(UART_Port uart_no) //do not use if tx flow control enabled
{
    while (READ_PERI_REG(UART_STATUS(uart_no)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S));
}

void
UART_ResetFifo(UART_Port uart_no)
{
    SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
}

void
UART_ClearIntrStatus(UART_Port uart_no, uint32 clr_mask)
{
    WRITE_PERI_REG(UART_INT_CLR(uart_no), clr_mask);
}

void
UART_SetIntrEna(UART_Port uart_no, uint32 ena_mask)
{
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), ena_mask);
}

void
UART_intr_handler_register(void *fn, void *arg)
{
    _xt_isr_attach(ETS_UART_INUM, fn, arg);
}

void
UART_SetPrintPort(UART_Port uart_no)
{
    if (uart_no == 1) {
        os_install_putc1(uart1_write_char);
    } else {
        os_install_putc1(uart0_write_char);
    }
}

void
UART_ParamConfig(UART_Port uart_no,  UART_ConfigTypeDef *pUARTConfig)
{
    if (uart_no == UART1) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
    } else {
        PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
    }

    UART_SetFlowCtrl(uart_no, pUARTConfig->flow_ctrl, pUARTConfig->UART_RxFlowThresh);
    UART_SetBaudrate(uart_no, pUARTConfig->baud_rate);

    WRITE_PERI_REG(UART_CONF0(uart_no),
                   ((pUARTConfig->parity == USART_Parity_None) ? 0x0 : (UART_PARITY_EN | pUARTConfig->parity))
                   | (pUARTConfig->stop_bits << UART_STOP_BIT_NUM_S)
                   | (pUARTConfig->data_bits << UART_BIT_NUM_S)
                   | ((pUARTConfig->flow_ctrl & USART_HardwareFlowControl_CTS) ? UART_TX_FLOW_EN : 0x0)
                   | pUARTConfig->UART_InverseMask);

    UART_ResetFifo(uart_no);
}

void
UART_IntrConfig(UART_Port uart_no,  UART_IntrConfTypeDef *pUARTIntrConf)
{

    uint32 reg_val = 0;
    UART_ClearIntrStatus(uart_no, UART_INTR_MASK);
    reg_val = READ_PERI_REG(UART_CONF1(uart_no)) & ((UART_RX_FLOW_THRHD << UART_RX_FLOW_THRHD_S) | UART_RX_FLOW_EN) ;

    reg_val |= ((pUARTIntrConf->UART_IntrEnMask & UART_RXFIFO_TOUT_INT_ENA) ?
                ((((pUARTIntrConf->UART_RX_TimeOutIntrThresh)&UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S) | UART_RX_TOUT_EN) : 0);

    reg_val |= ((pUARTIntrConf->UART_IntrEnMask & UART_RXFIFO_FULL_INT_ENA) ?
                (((pUARTIntrConf->UART_RX_FifoFullIntrThresh)&UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) : 0);

    reg_val |= ((pUARTIntrConf->UART_IntrEnMask & UART_TXFIFO_EMPTY_INT_ENA) ?
                (((pUARTIntrConf->UART_TX_FifoEmptyIntrThresh)&UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S) : 0);

    WRITE_PERI_REG(UART_CONF1(uart_no), reg_val);
    CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_INTR_MASK);
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), pUARTIntrConf->UART_IntrEnMask);
}

LOCAL void
uart0_rx_intr_handler(void *para)
{
    /* uart0 and uart1 intr combine togther, when interrupt occur, see reg 0x3ff20020, bit2, bit0 represents
    * uart1 and uart0 respectively
    */
    uint8 RcvChar;
    uint8 uart_no = UART0;//UartDev.buff_uart_no;
    uint8 fifo_len = 0;
    uint8 buf_idx = 0;
    uint8 fifo_tmp[128] = {0};

    uint32 uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no)) ;

    while (uart_intr_status != 0x0) {
        if (UART_FRM_ERR_INT_ST == (uart_intr_status & UART_FRM_ERR_INT_ST)) {
            printf("FRM_ERR\r\n");
            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_FRM_ERR_INT_CLR);
        } else if ((UART_RXFIFO_FULL_INT_ST == (uart_intr_status & UART_RXFIFO_FULL_INT_ST)) ||
        		(UART_RXFIFO_TOUT_INT_ST == (uart_intr_status & UART_RXFIFO_TOUT_INT_ST))) {
            //printf("full\r\n");
            fifo_len = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
            buf_idx = 0;

            while (buf_idx < fifo_len) {
                //uart_tx_one_char(UART0, READ_PERI_REG(UART_FIFO(UART0)) & 0xFF);
                Uart_RecvBuf[ Uart_RecvCnt++ ] = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
                buf_idx++;
            }

    		if( Uart_RecvBuf[ Uart_RecvCnt -1 ] == '\n' && Uart_RecvBuf[ Uart_RecvCnt - 2 ] == '\r' )// 接收完成
    		{
                WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
                WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);

    			Uart_RecvBuf[ Uart_RecvCnt ] = 0;
    			Uart_RecvCnt = 0;
    			printf("\r\n> ");
    			printf(Uart_RecvBuf);



    			if ( 0 == strcmp((char*)Uart_RecvBuf, "help\r\n") )
				{
    				printf("===> help <===\r\n" );
    				printf("get timer\r\n" );
    				printf("get delay\r\n" );
    				printf("get sys\r\n" );
    				printf("get date\r\n" );
    				//printf("get wifi\r\n" );
    				printf("get device info\r\n" );
    				printf("get html header\r\n" );
    				printf("get relay status\r\n" );
    				printf("debug free heap\r\n" );
    				printf("debug show malloc\r\n" );
    				printf("set reboot\r\n" );
    				printf("set log level[debug|info|error|none]\r\n" );
    				printf("\r\n" );
				}
    			else if ( 0 == strcmp((char*)Uart_RecvBuf, "get device info\r\n") )
    			{
					WIFI_DeviceInfoMarshalJson(Uart_SendBuf, UART_SENDMAXCNT);
					printf(Uart_SendBuf);
    			}
    			else if ( 0 == strcmp((char*)Uart_RecvBuf, "get sys\r\n") )
    			{
					PLUG_MarshalJsonSystemSet(Uart_SendBuf, UART_SENDMAXCNT);
					printf(Uart_SendBuf);
    			}
    			else if ( 0 == strcmp((char*)Uart_RecvBuf, "get timer\r\n") )
    			{
    				PLUG_MarshalJsonTimer(Uart_SendBuf, UART_SENDMAXCNT, PLUG_TIMER_ALL);
					printf(Uart_SendBuf);
    			}
    			else if ( 0 == strcmp((char*)Uart_RecvBuf, "get delay\r\n") )
    			{
    				PLUG_MarshalJsonDelay(Uart_SendBuf, UART_SENDMAXCNT, PLUG_DELAY_ALL);
					printf(Uart_SendBuf);
    			}
    			else if ( 0 == strcmp((char*)Uart_RecvBuf, "get html header\r\n") )
    			{
    				PLUG_MarshalJsonHtmlData(Uart_SendBuf, UART_SENDMAXCNT);
					printf(Uart_SendBuf);
    			}
    			else if ( 0 == strcmp((char*)Uart_RecvBuf, "get wifi\r\n") )
    			{
    				//WIFI_WifiScanMarshalJson(pcBuf, UART_SENDMAXCNT);
					printf(Uart_SendBuf);
    			}
      			else if ( 0 == strcmp((char*)Uart_RecvBuf, "get relay status\r\n") )
    			{
      				PLUG_MarshalJsonRelayStatus(Uart_SendBuf, UART_SENDMAXCNT);
    				printf(Uart_SendBuf);
    			}
      			else if ( 0 == strcmp((char*)Uart_RecvBuf, "get date\r\n") )
    			{
      				PLUG_MarshalJsonDate(Uart_SendBuf, UART_SENDMAXCNT);
      				printf(Uart_SendBuf);
    			}
      			else if ( 0 == strcmp((char*)Uart_RecvBuf, "debug free heap\r\n") )
    			{
      				printf("Free heap Size:%d", system_get_free_heap_size());
    			}
      			else if ( 0 == strcmp((char*)Uart_RecvBuf, "debug show malloc\r\n") )
    			{
      				system_show_malloc();
    			}
      			else if ( 0 == strcmp((char*)Uart_RecvBuf, "set reboot\r\n") )
    			{
      				UPGRADE_StartRebootTimer();
    			}
      			else if ( 0 != strstr((char*)Uart_RecvBuf, "set log level") )
    			{
      				if ( strstr(Uart_RecvBuf,"debug" ) != 0 ){
      					LOG_SetLogLevel(LOGOUT_DEBUG);
      					printf("set log level:debug\r\n");
      				}
      				else if (strstr(Uart_RecvBuf, "info" ) != 0){
      					LOG_SetLogLevel(LOGOUT_INFO);
      					printf("set log level:info\r\n");
      				}
      				else if (strstr(Uart_RecvBuf, "error" ) != 0){
      					LOG_SetLogLevel(LOGOUT_ERROR);
      					printf("set log level:error\r\n");
      				}
      				else if (strstr(Uart_RecvBuf, "none" ) != 0){
      					LOG_SetLogLevel(LOGOUT_NONE);
      					printf("set log level:none\r\n");
      				}
    			}
    			else
    			{
    				printf("unknow cmd,send \"help\" to list all cmd." );
    			}

    			printf("\r\n");
    		}
    		else if( Uart_RecvCnt >= UART_RECVMAXCNT ) {  //缓冲区满
    			Uart_RecvBuf[ Uart_RecvCnt ] = '\0';
    			printf(Uart_RecvBuf);
    			Uart_RecvCnt = 0;
    		}

        } else if (UART_TXFIFO_EMPTY_INT_ST == (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST)) {
            printf("empty\n\r");
            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_TXFIFO_EMPTY_INT_CLR);
            CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
        } else {
            //skip
        }

        uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no)) ;
    }
}

void
uart_init_new(void)
{
    UART_WaitTxFifoEmpty(UART0);
    UART_WaitTxFifoEmpty(UART1);

    UART_ConfigTypeDef uart_config;
    //uart_config.baud_rate    = BIT_RATE_74880;
    uart_config.baud_rate    = BIT_RATE_115200;
    uart_config.data_bits     = UART_WordLength_8b;
    uart_config.parity          = USART_Parity_None;
    uart_config.stop_bits     = USART_StopBits_1;
    uart_config.flow_ctrl      = USART_HardwareFlowControl_None;
    uart_config.UART_RxFlowThresh = 120;
    uart_config.UART_InverseMask = UART_None_Inverse;
    UART_ParamConfig(UART0, &uart_config);

    UART_IntrConfTypeDef uart_intr;
    uart_intr.UART_IntrEnMask = UART_RXFIFO_TOUT_INT_ENA | UART_FRM_ERR_INT_ENA | UART_RXFIFO_FULL_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA;
    uart_intr.UART_RX_FifoFullIntrThresh = 10;
    uart_intr.UART_RX_TimeOutIntrThresh = 2;
    uart_intr.UART_TX_FifoEmptyIntrThresh = 20;
    UART_IntrConfig(UART0, &uart_intr);

    UART_SetPrintPort(UART0);
    UART_intr_handler_register(uart0_rx_intr_handler, NULL);
    ETS_UART_INTR_ENABLE();

    /*
    UART_SetWordLength(UART0,UART_WordLength_8b);
    UART_SetStopBits(UART0,USART_StopBits_1);
    UART_SetParity(UART0,USART_Parity_None);
    UART_SetBaudrate(UART0,74880);
    UART_SetFlowCtrl(UART0,USART_HardwareFlowControl_None,0);
    */

}
