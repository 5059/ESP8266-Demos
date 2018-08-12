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


#include "eagle_soc.h"
#include "osapi.h"
#include "user_interface.h"
#include "driver/spi.h"
#include "driver/spi_interface.h"


/*
 * ESP8266 SPI ����ͨ�Ÿ�ʽΪ ����+��ַ+��/д����
 * ���������ڡ�����1~16λ
 * ��ַ����ѡ������0~32λ
 * ���ݶ�д����ѡ������0~512λ��64�ֽڣ�
 */

#define SPI_TEST_MASTER
//#define SPI_TEST_SLAVE


// Show the spi registers.
#define SHOWSPIREG(i) __ShowRegValue(__func__, __LINE__);

/**
 * @brief Print debug information.
 *
 */
void __ShowRegValue(const char * func, uint32_t line)
{

    int i;
    uint32_t regAddr = 0x60000140; // SPI--0x60000240, HSPI--0x60000140;
    os_printf("\r\n FUNC[%s],line[%d]\r\n", func, line);
    os_printf(" SPI_ADDR      [0x%08x]\r\n", READ_PERI_REG(SPI_ADDR(SpiNum_HSPI)));
    os_printf(" SPI_CMD       [0x%08x]\r\n", READ_PERI_REG(SPI_CMD(SpiNum_HSPI)));
    os_printf(" SPI_CTRL      [0x%08x]\r\n", READ_PERI_REG(SPI_CTRL(SpiNum_HSPI)));
    os_printf(" SPI_CTRL2     [0x%08x]\r\n", READ_PERI_REG(SPI_CTRL2(SpiNum_HSPI)));
    os_printf(" SPI_CLOCK     [0x%08x]\r\n", READ_PERI_REG(SPI_CLOCK(SpiNum_HSPI)));
    os_printf(" SPI_RD_STATUS [0x%08x]\r\n", READ_PERI_REG(SPI_RD_STATUS(SpiNum_HSPI)));
    os_printf(" SPI_WR_STATUS [0x%08x]\r\n", READ_PERI_REG(SPI_WR_STATUS(SpiNum_HSPI)));
    os_printf(" SPI_USER      [0x%08x]\r\n", READ_PERI_REG(SPI_USER(SpiNum_HSPI)));
    os_printf(" SPI_USER1     [0x%08x]\r\n", READ_PERI_REG(SPI_USER1(SpiNum_HSPI)));
    os_printf(" SPI_USER2     [0x%08x]\r\n", READ_PERI_REG(SPI_USER2(SpiNum_HSPI)));
    os_printf(" SPI_PIN       [0x%08x]\r\n", READ_PERI_REG(SPI_PIN(SpiNum_HSPI)));
    os_printf(" SPI_SLAVE     [0x%08x]\r\n", READ_PERI_REG(SPI_SLAVE(SpiNum_HSPI)));
    os_printf(" SPI_SLAVE1    [0x%08x]\r\n", READ_PERI_REG(SPI_SLAVE1(SpiNum_HSPI)));
    os_printf(" SPI_SLAVE2    [0x%08x]\r\n", READ_PERI_REG(SPI_SLAVE2(SpiNum_HSPI)));

    // ����ʹ��W0��ʼ��SPI������
    // �ӻ�ʹ��W8��ʼ�Ļ�����
    for (i = 0; i < 16; ++i) {
        os_printf(" ADDR[0x%08x],Value[0x%08x]\r\n", regAddr, READ_PERI_REG(regAddr));
        regAddr += 4;
    }
}


// SPI interrupt callback function.
void spi_slave_isr_sta(void *para)
{
    uint32 regvalue;
    uint32 statusW, statusR, counter;

    //0x3ff00020 is isr flag register, bit4 is for spi isr
    if (READ_PERI_REG(0x3ff00020)&BIT4) {
        //following 3 lines is to clear isr signal
        CLEAR_PERI_REG_MASK(SPI_SLAVE(SpiNum_SPI), 0x3ff);

    //bit7 is for hspi isr
    } else if (READ_PERI_REG(0x3ff00020)&BIT7) {
        regvalue = READ_PERI_REG(SPI_SLAVE(SpiNum_HSPI));
        os_printf("spi_slave_isr_sta SPI_SLAVE[0x%08x]\n\r", regvalue);
        SPIIntClear(SpiNum_HSPI);
        SET_PERI_REG_MASK(SPI_SLAVE(SpiNum_HSPI), SPI_SYNC_RESET);
        SPIIntClear(SpiNum_HSPI);
        
        SPIIntEnable(SpiNum_HSPI, SpiIntSrc_WrStaDone
                 | SpiIntSrc_RdStaDone 
                 | SpiIntSrc_WrBufDone 
                 | SpiIntSrc_RdBufDone);

        if (regvalue & SPI_SLV_WR_BUF_DONE) {
            // User can get data from the W0~W7
            os_printf("spi_slave_isr_sta : SPI_SLV_WR_BUF_DONE\n\r");
        } else if (regvalue & SPI_SLV_RD_BUF_DONE) {
            // TO DO 
            os_printf("spi_slave_isr_sta : SPI_SLV_RD_BUF_DONE\n\r");
        }

        if (regvalue & SPI_SLV_RD_STA_DONE) {
            statusR = READ_PERI_REG(SPI_RD_STATUS(SpiNum_HSPI));
            statusW = READ_PERI_REG(SPI_WR_STATUS(SpiNum_HSPI));
            os_printf("spi_slave_isr_sta : SPI_SLV_RD_STA_DONE[R=0x%08x,W=0x%08x]\n\r", statusR, statusW);
        }
        if (regvalue & SPI_SLV_WR_STA_DONE) {
            statusR = READ_PERI_REG(SPI_RD_STATUS(SpiNum_HSPI));
            statusW = READ_PERI_REG(SPI_WR_STATUS(SpiNum_HSPI));
            os_printf("spi_slave_isr_sta : SPI_SLV_WR_STA_DONE[R=0x%08x,W=0x%08x]\n\r", statusR, statusW);
        }
        if ((regvalue & SPI_TRANS_DONE) && ((regvalue & 0xf) == 0)) {
            os_printf("spi_slave_isr_sta : SPI_TRANS_DONE\n\r");
        }

        SHOWSPIREG(SpiNum_HSPI);

    }
}

// Test spi master interfaces.
void ICACHE_FLASH_ATTR
spi_master_test()
{
    SpiAttr hSpiAttr;
    hSpiAttr.bitOrder = SpiBitOrder_MSBFirst;
    hSpiAttr.speed = SpiSpeed_10MHz;
    hSpiAttr.mode = SpiMode_Master;
    hSpiAttr.subMode = SpiSubMode_0;

    // Init HSPI GPIO
    WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);//configure io to spi mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);//configure io to spi mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);//configure io to spi mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);//configure io to spi mode

    SPIInit(SpiNum_HSPI, &hSpiAttr);
    uint32_t addr = 0xD3D4D5D6;
    uint32_t sendData[8] ={ 0 };
    SpiData spiData;

    os_printf("\r\n =============   spi init master   ============= \r\n");

//  Test 8266 slave.Communication format: 1byte command + 1bytes address + x bytes Data.
    os_printf("\r\n Master send 32 bytes data to slave(8266)\r\n");
    os_memset(sendData, 0, sizeof(sendData));

    sendData[0] = 0x00010203;
    sendData[1] = 0x04050607;
    sendData[2] = 0x08090A0B;
    sendData[3] = 0x0C0D0E0F;
    sendData[4] = 0x10111213;
    sendData[5] = 0x14151617;
    sendData[6] = 0x18191A1B;
    sendData[7] = 0x1C1D1E1F;

    spiData.cmd = MASTER_WRITE_DATA_TO_SLAVE_CMD;
    spiData.cmdLen = 1;
    spiData.addr = &addr;
    spiData.addrLen = 1;
    spiData.data = sendData;
    spiData.dataLen = 32;
    SPIMasterSendData(SpiNum_HSPI, &spiData);
  

    os_printf("\r\n Master receive 24 bytes data from slave(8266)\r\n");
    spiData.cmd = MASTER_READ_DATA_FROM_SLAVE_CMD;
    spiData.cmdLen = 1;
    spiData.addr = &addr;
    spiData.addrLen = 1;
    spiData.data = sendData;
    spiData.dataLen = 32;

    // Ӧ�û��ӡSlave���������õ�����
    os_memset(sendData, 0, sizeof(sendData));
    SPIMasterRecvData(SpiNum_HSPI, &spiData);
    os_printf(" Recv Slave data0[0x%08x]\r\n", sendData[0]);	// [0x35343332]
    os_printf(" Recv Slave data1[0x%08x]\r\n", sendData[1]);	// [0x39383736]
    os_printf(" Recv Slave data2[0x%08x]\r\n", sendData[2]);	// [0x3d3c3b3a]
    os_printf(" Recv Slave data3[0x%08x]\r\n", sendData[3]);	// [0x11103f3e]
    os_printf(" Recv Slave data4[0x%08x]\r\n", sendData[4]);	// [0x15141312]
    os_printf(" Recv Slave data5[0x%08x]\r\n", sendData[5]);	// [0x19181716]
    os_printf(" Recv Slave data6[0x%08x]\r\n", sendData[6]);	// [0x1d1c1b1a]
    os_printf(" Recv Slave data7[0x%08x]\r\n", sendData[7]);	// [0x21201f1e]

    addr = SPIMasterRecvStatus(SpiNum_HSPI);
    os_printf("\r\n Master read slave(8266) status[0x%02x]\r\n", addr);

    SPIMasterSendStatus(SpiNum_HSPI, 0x99);
    os_printf("\r\n Master write status[0x99] to slave(8266).\r\n");
    //SHOWSPIREG(SpiNum_HSPI);

//  Test others slave.Communication format:0bytes command + 0 bytes address + x bytes Data
#if 0
    u32 addr2 = 0x06;
    os_printf("\r\n Master send 4 bytes data to slave\r\n");
    os_memset(sendData, 0, sizeof(sendData));
    sendData[0] = 0x2D3E4F50;
    spiData.cmd = MASTER_WRITE_DATA_TO_SLAVE_CMD;
    spiData.cmdLen = 0;
    spiData.addr = &addr2;
    spiData.addrLen = 0;
    spiData.data = sendData;
    spiData.dataLen = 4;
    SPIMasterSendData(SpiNum_HSPI, &spiData);

    os_printf("\r\n Master receive 4 bytes data from slaver\n");
    spiData.cmd = MASTER_READ_DATA_FROM_SLAVE_CMD;
    spiData.cmdLen = 0;
    spiData.addr = &addr2;
    spiData.addrLen = 0;
    spiData.data = sendData;
    spiData.dataLen = 4;
    os_memset(sendData, 0, sizeof(sendData));
    SPIMasterRecvData(SpiNum_HSPI, &spiData);
    os_printf(" Recv Slave data[0x%08x]\r\n", sendData[0]);
#endif

    // TODO:

}


// Test spi slave interfaces.
void ICACHE_FLASH_ATTR
spi_slave_test()
{
    //
    SpiAttr hSpiAttr;
    hSpiAttr.bitOrder = SpiBitOrder_MSBFirst;
    hSpiAttr.speed = 0;
    hSpiAttr.mode = SpiMode_Slave;
    hSpiAttr.subMode = SpiSubMode_0;

    // Init HSPI GPIO
    WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);//configure io to spi mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);//configure io to spi mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);//configure io to spi mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);//configure io to spi mode

    os_printf("\r\n ============= spi init slave =============\r\n");
    SPIInit(SpiNum_HSPI, &hSpiAttr);
    
    // Set spi interrupt information.
    SpiIntInfo spiInt;
    spiInt.src = (SpiIntSrc_TransDone 
        | SpiIntSrc_WrStaDone 
        |SpiIntSrc_RdStaDone 
        |SpiIntSrc_WrBufDone 
        |SpiIntSrc_RdBufDone);
    spiInt.isrFunc = spi_slave_isr_sta;
    SPIIntCfg(SpiNum_HSPI, &spiInt);
    
    SPISlaveRecvData(SpiNum_HSPI);
    //SHOWSPIREG(SpiNum_HSPI);

    // Master���յ����������
    u32 sndData[8] = { 0 };
    sndData[0] = 0x35343332;
    sndData[1] = 0x39383736;
    sndData[2] = 0x3d3c3b3a;
    sndData[3] = 0x11103f3e;
    sndData[4] = 0x15141312;
    sndData[5] = 0x19181716;
    sndData[6] = 0x1d1c1b1a;
    sndData[7] = 0x21201f1e;

    SPISlaveSendData(SpiNum_HSPI, sndData, 8);
    WRITE_PERI_REG(SPI_RD_STATUS(SpiNum_HSPI), 0x8A);
    WRITE_PERI_REG(SPI_WR_STATUS(SpiNum_HSPI), 0x83);
}

void spi_interface_test(void)
{
    // Test spi interfaces.
    os_printf("\r\n =======================================================\r\n");
    os_printf("\t ESP8266 %s application \n\r", __func__);
    os_printf("\t\t SDK version:%s    \n\r", system_get_sdk_version());
    os_printf("\t\t Complie time:%s  \n\r", __TIME__);
    os_printf("\r\n =======================================================\r\n");

#if defined(SPI_TEST_MASTER)
    spi_master_test();
#elif defined(SPI_TEST_SLAVE)
    spi_slave_test();
#endif

}
