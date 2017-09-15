/***************************************************************************

 *

 * Copyright (C) 2008-2009  SMSC

 *

 * This program is free software; you can redistribute it and/or

 * modify it under the terms of the GNU General Public License

 * as published by the Free Software Foundation; either version 2

 * of the License, or (at your option) any later version.

 *

 * This program is distributed in the hope that it will be useful,

 * but WITHOUT ANY WARRANTY; without even the implied warranty of

 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

 * GNU General Public License for more details.

 *

 * You should have received a copy of the GNU General Public License

 * along with this program; if not, write to the Free Software

 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 *

 ***************************************************************************

 * File: cmd9500.c

 */

#ifdef USING_LINT

#include "lint.h"

#else //not USING_LINT

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#endif //not USING_LINT

#include "version.h"
#include "ioctl_9500.h"

char *iam=NULL;

typedef enum _bool {

  false=0,

  true=1

} bool;



typedef struct _COMMAND_DATA {

	int hSockFD;

	struct ifreq IfReq;

	SMSC9500_IOCTL_DATA IoctlData;

} COMMAND_DATA, *PCOMMAND_DATA;



#define SOCKET	int

#define INVALID_SOCKET	(-1)

#define SOCKET_ERROR	(-1)

#define DEFAULT_PORT_NUMBER		(11500U)

SOCKET server_sock=INVALID_SOCKET;



typedef struct _FLOW_PARAMS

{

    unsigned long MeasuredMaxThroughput;

	unsigned long MeasuredMaxPacketCount;

	unsigned long MaxThroughput;

	unsigned long MaxPacketCount;

	unsigned long PacketCost;

	unsigned long BurstPeriod;

	unsigned long MaxWorkLoad;

	unsigned long IntDeas;

} FLOW_PARAMS, * PFLOW_PARAMS;



bool ParseNumber(const char *str,unsigned long *number);

void DisplayUsage(void);

void GetMacAddress(PCOMMAND_DATA commandData);

void SetMacAddress(PCOMMAND_DATA commandData,unsigned long addrh,unsigned long addrl);

void LoadMacAddress(PCOMMAND_DATA commandData);

void SaveMacAddress(PCOMMAND_DATA commandData,unsigned long addrh,unsigned long addrl);

void LanDumpRegs(PCOMMAND_DATA commandData);

void MacDumpRegs(PCOMMAND_DATA commandData);

void DumpEEPROM(PCOMMAND_DATA commandData);

void DumpTemp(PCOMMAND_DATA commandData);

void PhyDumpRegs(PCOMMAND_DATA commandData);

void SetDebugMode(PCOMMAND_DATA commandData,

				  unsigned long debug_mode);

void SetLinkMode(PCOMMAND_DATA commandData,

				 unsigned long link_mode);

void SetPowerMode(PCOMMAND_DATA commandData,

				  unsigned long power_mode);

void GetLinkMode(PCOMMAND_DATA commandData);

void CheckLink(PCOMMAND_DATA commandData);

void GetPowerMode(PCOMMAND_DATA commandData);

void GetFlowParams(PCOMMAND_DATA commandData);

void GetConfiguration(PCOMMAND_DATA commandData);

void SetAutoMdixSts(PCOMMAND_DATA commandData,unsigned int AutoMdix);

void GetAutoMdixSts(PCOMMAND_DATA commandData);

#ifdef RW_MEM
void ReadByte(PCOMMAND_DATA commandData,unsigned long address);
void ReadWord(PCOMMAND_DATA commandData, unsigned long address);
void ReadDWord(PCOMMAND_DATA commandData,unsigned long address);
void WriteByte(PCOMMAND_DATA commandData,unsigned long address, unsigned long data);
void WriteWord(PCOMMAND_DATA commandData,unsigned long address, unsigned long data);
void WriteDWord(PCOMMAND_DATA commandData,unsigned long address, unsigned long data);
#endif //RW_MEM

void LanGetReg(PCOMMAND_DATA commandData,unsigned long address);

void LanSetReg(PCOMMAND_DATA commandData, unsigned long address, unsigned long data);

void MacGetReg(PCOMMAND_DATA commandData, unsigned long address);

void MacSetReg(

	PCOMMAND_DATA commandData,

	unsigned long address, unsigned long data);

void PhyGetReg(PCOMMAND_DATA commandData, unsigned long address);

void PhySetReg(

	PCOMMAND_DATA commandData,

	unsigned long address, unsigned long data);

bool Initialize(PCOMMAND_DATA commandData,const char *ethName);

bool ReceiveULong(SOCKET sock,unsigned long * pDWord);

bool SendULong(SOCKET sock,unsigned long data);

unsigned long ReadThroughput(char * fileName);



bool ParseNumber(const char *str,unsigned long *number) {

	if(str==NULL) return false;

	if(str[0]==0) return false;

	if((str[0]=='0')&&(str[1]=='x')) {

		if(sscanf(&(str[2]),"%lx",number)==1) {

			return true;

		}

	}

	if(sscanf(str,"%ld",number)==1) {

		return true;

	}

	return false;

}

bool ParseString(const char *str, char **fileName)
{
	if(*fileName)free(*fileName);

	*fileName = malloc(strlen(str)+1);
	strcpy(*fileName, str);

}

void DisplayUsage(void) {

	printf("usage: %s [-h] [-e adaptername] [-c command] [-a address] [-d data] [-f filename]\n",iam);

	printf("  -h displays this usage information, other options ignored.\n");

	printf("  -e specifies the adapter name (eth0,eth1...)\n");

	printf("       if not specified then %s will attempt to\n",iam);

	printf("       auto detect.\n");

	printf("  -c specifies the command code\n");

	printf("       GET_CONFIG = gets internal variables of driver\n");

	printf("       DUMP_REGS = dumps the LAN9500 memory mapped registers\n");

	printf("       DUMP_MAC = dumps the LAN9500 MAC registers\n");

	printf("       DUMP_PHY = dumps the LAN9500 PHY registers\n");

	printf("       DUMP_EEPROM = dumps  512 bytes of the EEPROM\n");

	printf("       GET_MAC = gets MAC address from ADDRH and ADDRL\n");

	printf("       SET_MAC = sets MAC address in ADDRH and ADDRL\n");

	printf("         -a specifies the value to write to ADDRH\n");

	printf("         -d specifies the value to write to ADDRL\n");

	printf("       LOAD_MAC = causes the LAN9500 to reload the MAC address\n");

	printf("           from the external EEPROM. Also displays it\n");

	printf("       SAVE_MAC = writes a MAC address to the EEPROM\n");

	printf("         -a specifies the part of the MAC address that would\n");

	printf("            appear in ADDRH\n");

	printf("         -d specifies the part of the MAC address that would\n");

	printf("            appear in ADDRL\n");

	printf("       SET_DEBUG = sets the driver's internal debug_mode value\n");

	printf("         -d specifies the debug mode\n");

	printf("             0x01, bit 0, enables trace messages\n");

	printf("             0x02, bit 1, enables warning messages\n");

	printf("             0x04, bit 2, enables GPO signals\n");

	printf("          NOTE: trace, and warning messages will only show if\n");

	printf("             they have been turned on at driver compile time.\n");

	printf("       SET_LINK = sets the driver's internal link_mode value\n");

	printf("             and also attempts to relink with the new setting\n");

	printf("         -d specifies the link mode\n");

	printf("             1 = 10HD, 2 = 10FD, 4 = 100HD, 8 = 100FD\n");

	printf("             to specify multiple link modes, add the values\n");

	printf("             of each mode you want and use the sum as the link mode\n");

	printf("       GET_LINK = gets the driver's internal link_mode value\n");

	printf("       CHECK_LINK = causes the driver to recheck its link status\n");

	printf("       SET_AMDIX = sets the Auto Mdix value\n");

	printf("         -d specifies the Auto Mdix value\n");

	printf("             0 = Straight Cable, 1 = CrossOver Cable, 2 = Enable AMDIX\n");

	printf("       GET_AMDIX = gets the Auto Mdix value\n");
	printf("     Warning!! the following read and write commands may cause\n");

	printf("     unpredictable results, including system lock up or crash.\n");

	printf("     Use with caution\n");

	printf("       READ_REG = reads a value from the LAN9500 Memory Map\n");

	printf("         -a specifies offset into LAN9500 Memory Map\n");

	printf("       WRITE_REG = writes a value to the LAN9500 Memory Map\n");

	printf("         -a specifies offset into LAN9500 Memory Map\n");

	printf("         -d specifies data to write in HEX form\n");

	printf("       READ_MAC = reads a value from the LAN9500 Mac registers\n");

	printf("         -a specifies the Mac register index\n");

	printf("       WRITE_MAC = writes a value to the LAN9500 Mac registers\n");

	printf("         -a specifies the Mac register index\n");

	printf("         -d specifies data to write in HEX form\n");

	printf("       READ_PHY = reads a value from the LAN9500 Phy registers\n");

	printf("         -a specifies the Phy register index\n");

	printf("       WRITE_PHY = writes a value to the LAN9500 Phy registers\n");

	printf("         -a specifies the Phy register index\n");

	printf("         -d specifies data to write in HEX form\n");

    printf("       READ_EEPROM = reads a value from the LAN9500 eeprom\n");

    printf("         -a specifies the eeprom offset\n");

    printf("       WRITE_EEPROM = writes a value to the LAN9500 eeprom\n");

    printf("         -a specifies the eeprom offset\n");

    printf("         -d specifies data to write in HEX form\n");

    printf("       WRITE_FILE_TO_EEPROM = writes contents from a binary file into the LAN9500 eeprom\n");

    printf("         -f specifies the binary file name\n");

    printf("       READ_EEPROM_TO_FILE = reads the whole LAN9500 eeprom and write into a binary file\n");

    printf("         -f specifies the binary file name\n");

    printf("       VERIFY_EEPROM_WITH_FILE = compares the LAN9500 eeprom with the binary file\n");

    printf("         -f specifies the binary file name\n");

    printf("       GET_ERRORS = reads Linux urb error counters\n");

#ifdef RW_MEM
	printf("       READ_BYTE = reads a byte from a location in memory\n");

	printf("         -a address\n");

	printf("       READ_WORD = reads a word from a location in memory\n");

	printf("         -a address\n");

	printf("       READ_DWORD = reads a dword from a location in memory\n");

	printf("         -a address\n");

	printf("       WRITE_BYTE = write a byte to a location in memory\n");

	printf("         -a address -d data\n");

	printf("       WRITE_WORD = write a word to a location in memory\n");

	printf("         -a address -d data\n");

	printf("       WRITE_DWORD = write a dword to a location in memory\n");

	printf("         -a address -d data\n");
#endif //RW_MEM
	printf("  -a specifies the address, index, or offset of a register\n");

	printf("  -d specifies the data to write to a register\n");

	printf("       can be decimal or hexadecimal\n");

	printf("  -f specifies the file name\n");
}



void GetMacAddress(PCOMMAND_DATA commandData)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_GET_MAC_ADDRESS;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		printf("Mac Address == %02lX %02lX %02lX %02lX %02lX %02lX \n", commandData->IoctlData.Data[1] & 0xFF,
          (commandData->IoctlData.Data[1] >> 8) & 0xFF,
          (commandData->IoctlData.Data[1] >> 16) & 0xFF,  (commandData->IoctlData.Data[1] >> 24) & 0xFF,
          commandData->IoctlData.Data[0] & 0xFF, (commandData->IoctlData.Data[0] >> 8) & 0xFF);

	} else {

		printf("Failed to Get Mac Address\n");

	}

}

void SetMacAddress(PCOMMAND_DATA commandData,unsigned long addrh,unsigned long addrl)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_SET_MAC_ADDRESS;

	commandData->IoctlData.Data[0]=addrh;

	commandData->IoctlData.Data[1]=addrl;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

		printf("Failed to Set Mac Address\n");

	}

}

void LoadMacAddress(PCOMMAND_DATA commandData)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_LOAD_MAC_ADDRESS;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

        printf("Mac Address == %02lX %02lX %02lX %02lX %02lX %02lX \n", commandData->IoctlData.Data[1] & 0xFF,
          (commandData->IoctlData.Data[1] >> 8) & 0xFF,
          (commandData->IoctlData.Data[1] >> 16) & 0xFF,  (commandData->IoctlData.Data[1] >> 24) & 0xFF,
          commandData->IoctlData.Data[0] & 0xFF, (commandData->IoctlData.Data[0] >> 8) & 0xFF);

	} else {

		printf("Failed to Load Mac Address\n");

	}

}

void SaveMacAddress(PCOMMAND_DATA commandData,unsigned long addrh,unsigned long addrl)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_SAVE_MAC_ADDRESS;

	commandData->IoctlData.Data[0]=addrh;

	commandData->IoctlData.Data[1]=addrl;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

		printf("Failed to Save Mac Address\n");

	}

}



void LanDumpRegs(PCOMMAND_DATA commandData)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_DUMP_LAN_REGS;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		printf("offset 0x00, ID_REV       = 0x%08lX\n", commandData->IoctlData.Data[LAN_REG_ID_REV]);

        printf("offset 0x04 FPGA_REV      = 0x%08lX\n", commandData->IoctlData.Data[LAN_REG_FPGA_REV]);

        printf("offset 0x08 INT_STS       = 0x%08lX\n", commandData->IoctlData.Data[LAN_REG_INT_STS]);

		printf("offset 0x0C RX_CFG        = 0x%08lX\n", commandData->IoctlData.Data[LAN_REG_RX_CFG]);

		printf("offset 0x10 TX_CFG        = 0x%08lX\n", commandData->IoctlData.Data[LAN_REG_TX_CFG]);

		printf("offset 0x14 HW_CFG        = 0x%08lX\n", commandData->IoctlData.Data[LAN_REG_HW_CFG]);

		printf("offset 0x18 RX_FIFO_INF   = 0x%08lX\n", commandData->IoctlData.Data[LAN_REG_RX_FIFO_INF]);

		printf("offset 0x1C TX_FIFO_INF   = 0x%08lX\n", commandData->IoctlData.Data[LAN_REG_TX_FIFO_INF]);

		printf("offset 0x20 PMT_CTRL      = 0x%08lX\n", commandData->IoctlData.Data[LAN_REG_PMT_CTRL]);

        printf("offset 0x24 LED_GPIO_CFG  = 0x%08lX\n", commandData->IoctlData.Data[LAN_REG_LED_GPIO_CFG]);

		printf("offset 0x28 GPIO_CFG      = 0x%08lX\n", commandData->IoctlData.Data[LAN_REG_GPIO_CFG]);

		printf("offset 0x2C AFC_CFG       = 0x%08lX\n", commandData->IoctlData.Data[LAN_REG_AFC_CFG]);

		printf("offset 0x30 E2P_CMD       = 0x%08lX\n",  commandData->IoctlData.Data[LAN_REG_E2P_CMD]);

		printf("offset 0x34 E2P_DATA      = 0x%08lX\n",  commandData->IoctlData.Data[LAN_REG_E2P_DATA]);

        printf("offset 0x38 BURST_CAP     = 0x%08lX\n",  commandData->IoctlData.Data[LAN_REG_BURST_CAP]);

        printf("offset 0x3C STRAP_DBG     = 0x%08lX\n",  commandData->IoctlData.Data[LAN_REG_STRAP_DBG]);

        printf("offset 0x40 DP_SEL        = 0x%08lX\n",  commandData->IoctlData.Data[LAN_REG_DP_SEL]);

        printf("offset 0x44 DP_CMD        = 0x%08lX\n",  commandData->IoctlData.Data[LAN_REG_DP_CMD]);

        printf("offset 0x48 DP_ADDR       = 0x%08lX\n",  commandData->IoctlData.Data[LAN_REG_DP_ADDR]);

        printf("offset 0x4C DP_DATA0      = 0x%08lX\n",  commandData->IoctlData.Data[LAN_REG_DP_DATA0]);

        printf("offset 0x50 DP_DATA1      = 0x%08lX\n",  commandData->IoctlData.Data[LAN_REG_DP_DATA1]);

        printf("offset 0x64 GPIO_WAKE     = 0x%08lX\n",  commandData->IoctlData.Data[LAN_REG_GPIO_WAKE]);

        printf("offset 0x68 INT_EP_CTL    = 0x%08lX\n",  commandData->IoctlData.Data[LAN_REG_INT_EP_CTL]);

        printf("offset 0x6C BULK_IN_DLY   = 0x%08lX\n",  commandData->IoctlData.Data[LAN_REG_BULK_IN_DLY]);

	} else {

		printf("Failed to DUMP registers\n");

	}

}



void MacDumpRegs(PCOMMAND_DATA commandData)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_DUMP_MAC_REGS;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		printf("offset 0x100, MAC_CR   = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_MAC_CR]);

		printf("offset 0x104, ADDRH    = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_ADDRH]);

		printf("offset 0x108, ADDRL    = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_ADDRL]);

		printf("offset 0x10C, HASHH    = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_HASHH]);

		printf("offset 0x110, HASHL    = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_HASHL]);

		printf("offset 0x114, MII_ACC  = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_MII_ADDR]);

		printf("offset 0x118, MII_DATA = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_MII_DATA]);

		printf("offset 0x11C, FLOW     = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_FLOW]);

		printf("offset 0x120, VLAN1    = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_VLAN1]);

		printf("offset 0x124, VLAN2    = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_VLAN2]);

		printf("offset 0x128, WUFF     = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_WUFF]);

		printf("offset 0x12C, WUCSR    = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_WUCSR]);

        printf("offset 0x130, COE_CR    = 0x%08lX\n",commandData->IoctlData.Data[MAC_REG_COE_CR]);

	} else {

		printf("Failed to Dump Mac Registers\n");

	}

}



void DumpEEPROM(PCOMMAND_DATA commandData)

{
    int i,j;
    unsigned char* value;
    int eepromSize;
	if(commandData==NULL) return;

	eepromSize = GetEepromSize(commandData) * 128;
	if(eepromSize == 0){
		printf("EEPROM doesn't exist!\n");
		return;
	}

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_DUMP_EEPROM;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

        value = (unsigned char*)&commandData->IoctlData.Data[0];
        printf("Offset      Values\n");
        printf("------      ------\n");
        for(i=0; i<eepromSize/16; i++){
            printf("0x%04lX     ", (long unsigned int)i*16);
            for(j=0; j<16; j++){
                printf("%02lX ", (long unsigned int)value[i*16+j]);
            }
            printf("\n");
        }

	} else {

		printf("Failed to Dump EEPROM\n");

	}

}



void DumpTemp(PCOMMAND_DATA commandData)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_DUMP_TEMP;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		unsigned long c=0;

		for(c=0;c<0x10;c++) {

			printf("temp[0x%02lX]=0x%08lX, ",c,commandData->IoctlData.Data[c]);

			printf("temp[0x%02lX]=0x%08lX, ",c+0x10,commandData->IoctlData.Data[c+0x10]);

			printf("temp[0x%02lX]=0x%08lX, ",c+0x20,commandData->IoctlData.Data[c+0x20]);

			printf("temp[0x%02lX]=0x%08lX\n",c+0x30,commandData->IoctlData.Data[c+0x30]);

		}

	} else {

		printf("Failed to dump temp data.\n");

	}

}



void PhyDumpRegs(PCOMMAND_DATA commandData)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_DUMP_PHY_REGS;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		printf("index 0, Basic Control Reg = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_BCR]);

		printf("index 1, Basic Status Reg  = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_BSR]);

		printf("index 2, PHY identifier 1  = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_ID1]);

		printf("index 3, PHY identifier 2  = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_ID2]);

		printf("index 4, Auto Negotiation Advertisement Reg = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_ANEG_ADV]);

		printf("index 5, Auto Negotiation Link Partner Ability Reg = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_ANEG_LPA]);

		printf("index 6, Auto Negotiation Expansion Register = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_ANEG_ER]);

		printf("index 16, Silicon Revision Reg = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_SILICON_REV]);

		printf("index 17, Mode Control/Status Reg = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_MODE_CTRL_STS]);

		printf("index 18, Special Modes = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_SPECIAL_MODES]);

		printf("index 20, TSTCNTL = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_TSTCNTL]);

		printf("index 21, TSTREAD1 = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_TSTREAD1]);

		printf("index 22, TSTREAD2 = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_TSTREAD2]);

		printf("index 23, TSTWRITE = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_TSTWRITE]);

		printf("index 27, Control/Status Indication = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_SPECIAL_CTRL_STS]);

		printf("index 28, Special internal testability = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_SITC]);

		printf("index 29, Interrupt Source Register = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_INT_SRC]);

		printf("index 30, Interrupt Mask Register = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_INT_MASK]);

		printf("index 31, PHY Special Control/Status Register = 0x%04lX\n",commandData->IoctlData.Data[PHY_REG_SPECIAL]);

	} else {

		printf("Failed to DUMP Phy Registers\n");

	}

}



void SetDebugMode(PCOMMAND_DATA commandData,

				  unsigned long debug_mode)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_SET_DEBUG_MODE;

	commandData->IoctlData.Data[0]=debug_mode;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

		printf("Failed to set debug mode.\n");

	}

}



void SetLinkMode(PCOMMAND_DATA commandData,

				 unsigned long link_mode)

{

	if(link_mode<=0x7F) {

		if(commandData==NULL) return;

		commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

		commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

		commandData->IoctlData.dwCommand=COMMAND_SET_LINK_MODE;

		commandData->IoctlData.Data[0]=link_mode;

		ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

		if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

			printf("Failed to set link mode.\n");

		}

	} else {

		printf("Invalid Link Mode, %ld\n",link_mode);

	}

}


void SetAutoMdixSts(PCOMMAND_DATA commandData, unsigned int AutoMdix)

{

//	if(AutoMdix<=2) {

		if(commandData==NULL) return;

		commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

		commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

		commandData->IoctlData.dwCommand=COMMAND_SET_AMDIX_STS;

		commandData->IoctlData.Data[0]=AutoMdix;

		ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

		if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

			printf("Failed to set AMDIX State.\n");

		}

//	} else {


//		printf("Invalid AutoMdix value, %d\n",AutoMdix);

//	}

}

void GetLinkMode(PCOMMAND_DATA commandData)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_GET_LINK_MODE;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		unsigned long link_mode=commandData->IoctlData.Data[0];

		printf("link_mode == 0x%02lX == %s,%s,%s,%s,%s,%s,%s\n",

			link_mode,

			(link_mode&0x40)?"ANEG":"",

			(link_mode&0x20)?"SYMP":"",

			(link_mode&0x10)?"ASYMP":"",

			(link_mode&0x08)?"100FD":"",

			(link_mode&0x04)?"100HD":"",

			(link_mode&0x02)?"10FD":"",

			(link_mode&0x01)?"10HD":"");

	} else {

		printf("Failed to get link mode\n");

	}

}

void GetAutoMdixSts(PCOMMAND_DATA commandData)
{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_GET_AMDIX_STS;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		printf("AutoMdix value = 0x%x\n", (unsigned int)commandData->IoctlData.Data[0]);

	} else {

		printf("Failed to get AutoMdix value.\n");

	}


}

void CheckLink(PCOMMAND_DATA commandData)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_CHECK_LINK;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		printf("Checked link successfully\n");

	} else {

		printf("Failed to check link\n");

	}

}


void GetFlowParams(PCOMMAND_DATA commandData)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_GET_FLOW_PARAMS;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		const unsigned long * data=commandData->IoctlData.Data;

		printf("Flow Control Parameters\n");

		printf("  RxFlowMeasuredMaxThroughput     = 0x%08lX\n",data[0]);

		printf("  RxFlowMeasuredMaxPacketCount    = 0x%08lX\n",data[1]);

		printf("  RxFlowParameters.MaxThroughput  = 0x%08lX\n",data[2]);



		printf("  RxFlowParameters.MaxPacketCount = 0x%08lX\n",data[3]);

		printf("  RxFlowParameters.PacketCost     = 0x%08lX\n",data[4]);

		printf("  RxFlowParameters.BurstPeriod    = 0x%08lX\n",data[5]);

		printf("  RxFlowMaxWorkLoad               = 0x%08lX\n",data[6]);

		printf("  INT_CFG.INT_DEAS                = 0x%08lX\n",data[7]);

	} else {

		printf("Failed to get flow control parameters\n");

	}

}



void GetConfiguration(PCOMMAND_DATA commandData)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_GET_CONFIGURATION;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		const unsigned long * data=commandData->IoctlData.Data;

		printf("Compiled: %s\n",commandData->IoctlData.Strng1);

		printf("Driver Version = %lX.%02lX.%02lX\n",

			data[0]>>16&0xFF, data[0]>>8&0xFF, data[0]&0xFFUL);

		printf("Driver Parameters\n");

		printf("  link_mode        = 0x%08lX\n",data[1]);

		printf("  mac_addr_hi16    = 0x%08lX\n",data[2]);

		printf("  mac_addr_lo32    = 0x%08lX\n",data[3]);

		printf("  debug_mode       = 0x%08lX\n",data[4]);

		printf("privateData\n");

		printf("  dwIdRev          = 0x%08lX\n",data[5]);

		printf("  dwFpgaRev        = 0x%08lX\n",data[6]);

		printf("  bPhyAddress      = 0x%08lX\n",data[7]);

		printf("  dwPhyId          = 0x%08lX\n",data[8]);

		printf("  bPhyModel        = 0x%08lX\n",data[9]);

		printf("  bPhyRev          = 0x%08lX\n",data[10]);

		printf("  dwLinkSpeed      = 0x%08lX\n",data[11]);

		printf("  eepromSize       = %d\n",	(unsigned int)data[12] * 128);

	} else {

		printf("Failed to get driver configuration\n");

	}

}

void GetErrors(PCOMMAND_DATA commandData)

{

    if(commandData==NULL) return;

    commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

    commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

    commandData->IoctlData.dwCommand=COMMAND_GET_ERRORS;

    ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

    if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

        const unsigned long * data=commandData->IoctlData.Data;

        printf("  TX EPIPE         = 0x%08lX\n",data[0]);
        printf("  TX EPROTO        = 0x%08lX\n",data[1]);
        printf("  TX ETIMWOUT      = 0x%08lX\n",data[2]);
        printf("  TX EILSEQ        = 0x%08lX\n",data[3]);

        printf("  RX EPIPE         = 0x%08lX\n",data[4]);
        printf("  RX EPROTO        = 0x%08lX\n",data[5]);
        printf("  RX ETIMWOUT      = 0x%08lX\n",data[6]);
        printf("  RX EILSEQ        = 0x%08lX\n",data[7]);
        printf("  RX EOVERFLOW     = 0x%08lX\n",data[8]);

    } else {

        printf("Failed to get driver configuration\n");

    }

}

#ifdef RW_MEM
void ReadByte(PCOMMAND_DATA commandData,unsigned long address)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_READ_BYTE;

	commandData->IoctlData.Data[0]=address;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		printf("Memory Address == 0x%08lX, Read Value == 0x%02lX\n",

			commandData->IoctlData.Data[0],

			commandData->IoctlData.Data[1]&0xFFUL);

	} else {

		printf("Failed to Read Memory\n");

	}

}

void ReadWord(PCOMMAND_DATA commandData, unsigned long address)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_READ_WORD;

	commandData->IoctlData.Data[0]=address;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		printf("Memory Address == 0x%08lX, Read Value == 0x%04lX\n",

			commandData->IoctlData.Data[0],

			commandData->IoctlData.Data[1]&0xFFFFUL);

	} else {

		printf("Failed to Read Memory\n");

	}



}

void ReadDWord(PCOMMAND_DATA commandData,unsigned long address)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_READ_DWORD;

	commandData->IoctlData.Data[0]=address;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		printf("Memory Address == 0x%08lX, Read Value == 0x%08lX\n",

			commandData->IoctlData.Data[0],

			commandData->IoctlData.Data[1]);

	} else {

		printf("Failed to Read Memory\n");

	}

}

void WriteByte(PCOMMAND_DATA commandData,unsigned long address, unsigned long data)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_WRITE_BYTE;

	commandData->IoctlData.Data[0]=address;

	commandData->IoctlData.Data[1]=data&0xFFUL;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

		printf("Failed to Write Memory\n");

	}

}

void WriteWord(PCOMMAND_DATA commandData,unsigned long address, unsigned long data)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_WRITE_WORD;

	commandData->IoctlData.Data[0]=address;

	commandData->IoctlData.Data[1]=data&0xFFFFUL;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

		printf("Failed to Write Memory\n");

	}

}

void WriteDWord(PCOMMAND_DATA commandData,unsigned long address, unsigned long data)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_WRITE_DWORD;

	commandData->IoctlData.Data[0]=address;

	commandData->IoctlData.Data[1]=data;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

		printf("Failed to Write Memory\n");

	}

}
#endif //RW_MEM


void LanGetReg(PCOMMAND_DATA commandData,unsigned long address)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_LAN_GET_REG;

	commandData->IoctlData.Data[0]=address;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		printf("Mem Map Offset == 0x%08lX, Read Value == 0x%08lX\n",

			commandData->IoctlData.Data[0],

			commandData->IoctlData.Data[1]);

	} else {

		printf("Failed to Read Register\n");

	}

}

void LanSetReg(PCOMMAND_DATA commandData, unsigned long address, unsigned long data)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_LAN_SET_REG;

	commandData->IoctlData.Data[0]=address;

	commandData->IoctlData.Data[1]=data;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

		printf("Failed to Write Register\n");

	}

}

void MacGetReg(PCOMMAND_DATA commandData, unsigned long address)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_MAC_GET_REG;

	commandData->IoctlData.Data[0]=address;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		printf("Mac Index == 0x%08lX, Read Value == 0x%08lX\n",

			commandData->IoctlData.Data[0],

			commandData->IoctlData.Data[1]);

	} else {

		printf("Failed to read Mac Register\n");

	}

}

void MacSetReg(

	PCOMMAND_DATA commandData,

	unsigned long address, unsigned long data)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_MAC_SET_REG;

	commandData->IoctlData.Data[0]=address;

	commandData->IoctlData.Data[1]=data;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

		printf("Failed to Write Mac Register\n");

	}

}

void PhyGetReg(PCOMMAND_DATA commandData, unsigned long address)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_PHY_GET_REG;

	commandData->IoctlData.Data[0]=address;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		printf("Phy Index == 0x%08lX, Read Value == 0x%08lX\n",

			commandData->IoctlData.Data[0],

			commandData->IoctlData.Data[1]);

	} else {

		printf("Failed to Read Phy Register\n");

	}

}

void PhySetReg(

	PCOMMAND_DATA commandData,

	unsigned long address, unsigned long data)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_PHY_SET_REG;

	commandData->IoctlData.Data[0]=address;

	commandData->IoctlData.Data[1]=data;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

		printf("Failed to Write Phy Register\n");

	}

}

void GetEeprom(PCOMMAND_DATA commandData, unsigned long address, bool msgOn)

{

    if(commandData==NULL) return;

    commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

    commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

    commandData->IoctlData.dwCommand=COMMAND_GET_EEPROM;

    commandData->IoctlData.Data[0]=address;

    ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

    if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {
    	if(msgOn){
    		printf("Index == 0x%04lX, Read Value == 0x%02X\n",
    				commandData->IoctlData.Data[0],
    				(unsigned int)commandData->IoctlData.Data[1]);
    	}
    } else {

        printf("Failed to Read EEPROM\n");

    }

}

void SetEeprom(

    PCOMMAND_DATA commandData,

    unsigned long address, unsigned long data)

{

    if(commandData==NULL) return;

    commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

    commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

    commandData->IoctlData.dwCommand=COMMAND_SET_EEPROM;

    commandData->IoctlData.Data[0]=address;

    commandData->IoctlData.Data[1]=data;

    ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

    if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

        printf("Failed to Write EEPROM\n");

    }

}

void SetEepromBuffer(

    PCOMMAND_DATA commandData,

    unsigned long address, unsigned int len, char* buf)

{
    if(commandData==NULL) return;

    commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

    commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

    commandData->IoctlData.dwCommand=COMMAND_SET_EEPROM_BUFFER;

    commandData->IoctlData.Data[0]=address;

    commandData->IoctlData.Data[1]=len;

    memcpy(&commandData->IoctlData.Data[2], buf, len); 

    ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

    if(commandData->IoctlData.dwSignature!=SMSC9500_DRIVER_SIGNATURE) {

        printf("Failed to Write EEPROM\n");
    }
}


int GetEepromSize(PCOMMAND_DATA commandData)

{

	if(commandData==NULL) return;

	commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

	commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

	commandData->IoctlData.dwCommand=COMMAND_GET_CONFIGURATION;

	ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

	if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

		const unsigned long * data=commandData->IoctlData.Data;

		return(data[12]);

	} else {
		return 0;
	}
}

void WriteEepromToFile(PCOMMAND_DATA commandData, char* fileName)
{
	FILE * filePtr=NULL;
	int eepromSize, i;
	unsigned long addr, data;
	char buf;

	eepromSize = GetEepromSize(commandData) * 128;
	printf("Eeprom size is %dB\n", eepromSize);
	if(eepromSize == 0){
		printf("EEPROM doesn't exist\n");
		return;
	}

	filePtr = fopen(fileName,"w+b");

	if(filePtr != NULL){

		for(i=0; i<eepromSize; i++){
			GetEeprom(commandData, i, false);
			if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {
					buf = commandData->IoctlData.Data[1];
					fwrite(&buf, 1, 1, filePtr);
			} else {
				printf("Failed to Read EEPROM\n");
				break;
			}

		}
		if(i == eepromSize){
			printf("Wrote EEPROM contents into file %s successfully\n", fileName);
		}
	}

	fclose(filePtr);
}

void VerifyEepromWithFile(PCOMMAND_DATA commandData, char* fileName)
{
	FILE * filePtr=NULL;
	int eepromSize, fileSize, i;
	unsigned long addr, data;
	unsigned char buf;

	eepromSize = GetEepromSize(commandData) * 128;
	printf("Eeprom size is %dB\n", eepromSize);
	if(eepromSize == 0){
		printf("EEPROM doesn't exist\n");
		return;
	}

	filePtr = fopen(fileName,"rb");

	if(filePtr != NULL){
		fseek(filePtr, 0, SEEK_END);
		fileSize = ftell(filePtr); //Get file size
		fseek(filePtr, 0, SEEK_SET);
		if(fileSize < eepromSize){
			printf("EEPROM size is larger than file size\n");
			fclose(filePtr);
			return;
		}

		for(i=0; i<eepromSize; i++){
			GetEeprom(commandData, i, false);
			if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {
					if(fread(&buf, 1, 1, filePtr) != 0){
						if(buf != (unsigned char)commandData->IoctlData.Data[1]){
							printf("EEPROM contents are DIFFERENT from the file contents at offset %d\n", i);
							break;
						}
					}
			} else {
				printf("Failed to Read EEPROM\n");
				break;
			}
		}
		if(i == eepromSize){
			printf("EEPROM contents are as SAME as the file contents\n");
		}
	}

	fclose(filePtr);
}

void UpdateEepromFromFile(PCOMMAND_DATA commandData, char* fileName)
{
	FILE * filePtr=NULL;
	int eepromSize, fileSize, i;
	unsigned long addr, data;
	char* buf;

	eepromSize = GetEepromSize(commandData) * 128;
	printf("Eeprom size is %dB\n", eepromSize);
	if(eepromSize == 0){
		printf("EEPROM doesn't exist\n");
		return;
	}

	filePtr = fopen(fileName,"rb");
	if(filePtr != NULL){
		fseek(filePtr, 0, SEEK_END);
		fileSize = ftell(filePtr); //Get file size
		fseek(filePtr, 0, SEEK_SET);

		if(fileSize > eepromSize){
			printf("The binary file size is larger than %d\n", eepromSize);
			fclose(filePtr);
			return;
		}
		if(fileSize > MAX_EEPROM_SIZE){
			printf("The binary file size is larger than supported EEPROM size (%d)\n", MAX_EEPROM_SIZE);
			fclose(filePtr);
			return;
		}
		printf("File size is %dB\n", fileSize);
		buf = malloc(fileSize);
		if(!buf){
			printf("No memory available\n");
			fclose(filePtr);
			return;
		}

		if(fread(buf, 1, fileSize, filePtr) != 0){
			SetEepromBuffer(commandData, 0, fileSize, buf);
			if(commandData->IoctlData.dwSignature != SMSC9500_DRIVER_SIGNATURE) {
				printf("Failed to update EEPROM\n");
			} else {
				printf("Updated EEPROM contents successfully\n");
			}
		} else {
			printf("Unable to read binary file\n");
		}

		free(buf);
	}

	fclose(filePtr);
}

bool Initialize(PCOMMAND_DATA commandData,const char *ethName) {



	if(commandData==NULL) return false;

	if(ethName==NULL) return false;

	commandData->hSockFD=socket(AF_INET,SOCK_DGRAM,0);

	if((commandData->hSockFD) < 0) {

		perror("\r\nFailed to create socket !! ->");

		return false;

   	}

	commandData->IfReq.ifr_data= (void *)&(commandData->IoctlData);

	memset(&(commandData->IoctlData),0,sizeof(SMSC9500_IOCTL_DATA));

	if(ethName[0]!=0) {

		strncpy(commandData->IfReq.ifr_name,ethName,IFNAMSIZ);

		commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

		commandData->IoctlData.dwCommand=COMMAND_GET_SIGNATURE;

		ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

		if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

			return true;

		}

		printf("Failed to find 9500 driver on %s\n",commandData->IfReq.ifr_name);

	} else {

		int ifNumber;

		for(ifNumber=0;ifNumber<8;ifNumber++) {

			sprintf(commandData->IfReq.ifr_name,"eth%d",ifNumber);

			commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

			commandData->IoctlData.dwCommand=COMMAND_GET_SIGNATURE;

			commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

			ioctl(commandData->hSockFD,SMSC9500_IOCTL,&(commandData->IfReq));

			if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

				//printf("found 9500 on %s\n",commandData->IfReq.ifr_name);

				return true;

			}

		}

		printf("Failed to find 9500 driver on eth0 .. eth7\n");

	}

	printf("Either the driver has not been installed or there is\n");

	printf("a possible version mismatch between smsc9500.o and cmd9500\n");

	return false;

}



bool ReceiveULong(SOCKET sock,unsigned long * pDWord)

{

	bool result=false;

	unsigned long data=0;

	unsigned char ch=0;

	if(recv(sock,&ch,1,0)>0) {

		data=(unsigned long)ch;

		if(recv(sock,&ch,1,0)>0) {

			data|=(((unsigned long)ch)<<8);

			if(recv(sock,&ch,1,0)>0) {

				data|=(((unsigned long)ch)<<16);

				if(recv(sock,&ch,1,0)>0) {

					data|=(((unsigned long)ch)<<24);

					(*pDWord)=data;

					result=true;

				}

			}

		}

	}

	return result;

}



bool SendULong(SOCKET sock,unsigned long data)

{

	bool result=false;

	unsigned char ch=(unsigned char)(data&0x000000FFUL);

	if(send(sock,&ch,1,0)==1) {

		ch=(unsigned char)((data>>8)&0x000000FFUL);

		if(send(sock,&ch,1,0)==1) {

			ch=(unsigned char)((data>>16)&0x000000FFUL);

			if(send(sock,&ch,1,0)==1) {

				ch=(unsigned char)((data>>24)&0x000000FFUL);

				if(send(sock,&ch,1,0)==1) {

					result=true;

				}

			}

		}

	}

	return result;

}



void process_requests(PCOMMAND_DATA commandData)

{

	unsigned long requestCode=0;

	while(ReceiveULong(server_sock,&requestCode)) {

		switch(requestCode) {

		case COMMAND_GET_FLOW_PARAMS:

			commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

			commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

			commandData->IoctlData.dwCommand=COMMAND_GET_FLOW_PARAMS;

			ioctl(commandData->hSockFD,

				SMSC9500_IOCTL,&(commandData->IfReq));

			if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

				SendULong(server_sock,1);//1==success

				SendULong(server_sock,commandData->IoctlData.Data[0]);//RxFlowMeasuredMaxThroughput

				SendULong(server_sock,commandData->IoctlData.Data[1]);//RxFlowMeasuredMaxPacketCount

				SendULong(server_sock,commandData->IoctlData.Data[2]);//RxFlowParameters.MaxThroughput

				SendULong(server_sock,commandData->IoctlData.Data[3]);//RxFlowParameters.MaxPacketCount

				SendULong(server_sock,commandData->IoctlData.Data[4]);//RxFlowParameters.PacketCost

				SendULong(server_sock,commandData->IoctlData.Data[5]);//RxFlowParameters.BurstPeriod

				SendULong(server_sock,commandData->IoctlData.Data[6]);//RxFlowMaxWorkLoad

				SendULong(server_sock,commandData->IoctlData.Data[7]);//INT_CFG.INT_DEAS

			} else {

				SendULong(server_sock,0);//0==failed

			}

			break;

		case COMMAND_SET_FLOW_PARAMS:

			{

				unsigned long data=0;

				commandData->IfReq.ifr_data=(void *)&(commandData->IoctlData);

				commandData->IoctlData.dwSignature=SMSC9500_APP_SIGNATURE;

				commandData->IoctlData.dwCommand=COMMAND_SET_FLOW_PARAMS;

				if(!ReceiveULong(server_sock,&data)) break;

				commandData->IoctlData.Data[0]=data;//RxFlowMeasuredMaxThroughput

				if(!ReceiveULong(server_sock,&data)) break;

				commandData->IoctlData.Data[1]=data;//RxFlowMeasuredMaxPacketCount

				if(!ReceiveULong(server_sock,&data)) break;

				commandData->IoctlData.Data[2]=data;//RxFlowParameters.MaxThroughput

				if(!ReceiveULong(server_sock,&data)) break;

				commandData->IoctlData.Data[3]=data;//RxFlowParameters.MaxPacketCount

				if(!ReceiveULong(server_sock,&data)) break;

				commandData->IoctlData.Data[4]=data;//RxFlowParameters.PacketCost

				if(!ReceiveULong(server_sock,&data)) break;

				commandData->IoctlData.Data[5]=data;//RxFlowParameters.BurstPeriod

				if(!ReceiveULong(server_sock,&data)) break;

				commandData->IoctlData.Data[6]=data;//RxFlowMaxWorkLoad

				if(!ReceiveULong(server_sock,&data)) break;

				commandData->IoctlData.Data[7]=data;//INT_CFG.INT_DEAS

				ioctl(commandData->hSockFD,

					SMSC9500_IOCTL,&(commandData->IfReq));

				if(commandData->IoctlData.dwSignature==SMSC9500_DRIVER_SIGNATURE) {

					SendULong(server_sock,1);//1==success

				} else {

					SendULong(server_sock,0);//0==failed

				}

			}

			break;

		default:

			printf("WARNING, unknown requestCode=0x%08lX\n",requestCode);

			break;

		}

	}

}

unsigned long ReadThroughput(char * fileName)

{

	unsigned long result=0;

	bool clearFlag=true;

	FILE * filePtr=NULL;

	filePtr=fopen(fileName,"r");

	if(filePtr!=NULL) {

		char ch=0;

		while(fread(&ch,1,1,filePtr)!=0) {

			switch(ch) {

			case '0':case '1':case '2':case '3':case '4':



			case '5':case '6':case '7':case '8':case '9':

				if(clearFlag) {

					result=0;

					clearFlag=false;

				}

				result*=10;

				result+=(unsigned long)(ch-'0');

				break;

			case '.':

				break;

			default:

				clearFlag=true;

				break;

			}

		}

		fclose(filePtr);

	} else {

		printf("ReadThroughput: unable to open file\n");

	}

	return result;

}


int main(ac,av)

int ac;

char * av[];

{

	int oc=0;

	bool eSet=false;

	char ethName[IFNAMSIZ];

	COMMAND_DATA commandData;

	bool cSet=false;

	bool aSet=false;
	bool fSet = false;
	unsigned long address=0;

	bool dSet=false;

	unsigned long data=0;

	unsigned long commandCode=0;

	char hostName[128];

	bool hSet=false;

	unsigned long portNum=0;
	char *fileName = NULL;
	bool pSet=false;





	iam=av[0];

	ethName[0]=0;

	hostName[0]=0;



	while((oc=getopt(ac,av,"hH:p:e:c:a:d:f:"))!=-1) {

		switch(oc) {

		case 'h'://help

			goto BAD_USAGE;

		case 'H'://Host address

			if(hSet) goto BAD_USAGE;

			strcpy(hostName,optarg);

			hSet=true;

			break;

		case 'p':

			if(pSet) goto BAD_USAGE;

			if(!ParseNumber(optarg,&portNum)) {

				goto BAD_USAGE;

			}

			if(portNum>0xFFFFUL) goto BAD_USAGE;

			pSet=true;

			break;

		case 'e':

			if(eSet) goto BAD_USAGE;

			eSet=true;

			strncpy(ethName,optarg,IFNAMSIZ);

			ethName[IFNAMSIZ-1]=0;

			break;

		case 'c':

			if(cSet) goto BAD_USAGE;

			if(strcmp(optarg,"GET_CONFIG")==0) {

				commandCode=COMMAND_GET_CONFIGURATION;

			} else if(strcmp(optarg,"DUMP_REGS")==0) {

				commandCode=COMMAND_DUMP_LAN_REGS;

			} else if(strcmp(optarg,"DUMP_MAC")==0) {

				commandCode=COMMAND_DUMP_MAC_REGS;

			} else if(strcmp(optarg,"DUMP_PHY")==0) {

				commandCode=COMMAND_DUMP_PHY_REGS;

			} else if(strcmp(optarg,"DUMP_EEPROM")==0) {

				commandCode=COMMAND_DUMP_EEPROM;

			} else if(strcmp(optarg,"DUMP_TEMP")==0) {

				commandCode=COMMAND_DUMP_TEMP;

			} else if(strcmp(optarg,"GET_MAC")==0) {

				commandCode=COMMAND_GET_MAC_ADDRESS;

			} else if(strcmp(optarg,"SET_MAC")==0) {

				commandCode=COMMAND_SET_MAC_ADDRESS;

			} else if(strcmp(optarg,"LOAD_MAC")==0) {

				commandCode=COMMAND_LOAD_MAC_ADDRESS;

			} else if(strcmp(optarg,"SAVE_MAC")==0) {

				commandCode=COMMAND_SAVE_MAC_ADDRESS;

			} else if(strcmp(optarg,"SET_DEBUG")==0) {

				commandCode=COMMAND_SET_DEBUG_MODE;

			} else if(strcmp(optarg,"SET_LINK")==0) {

				commandCode=COMMAND_SET_LINK_MODE;

			} else if(strcmp(optarg,"GET_LINK")==0) {

				commandCode=COMMAND_GET_LINK_MODE;

			} else if(strcmp(optarg,"SET_AMDIX")==0) {

				commandCode=COMMAND_SET_AMDIX_STS;

			} else if(strcmp(optarg,"GET_AMDIX")==0) {

				commandCode=COMMAND_GET_AMDIX_STS;

			} else if(strcmp(optarg,"CHECK_LINK")==0) {

				commandCode=COMMAND_CHECK_LINK;

			} else if(strcmp(optarg,"READ_REG")==0) {

				commandCode=COMMAND_LAN_GET_REG;

			} else if(strcmp(optarg,"WRITE_REG")==0) {

				commandCode=COMMAND_LAN_SET_REG;

			} else if(strcmp(optarg,"READ_MAC")==0) {

				commandCode=COMMAND_MAC_GET_REG;

			} else if(strcmp(optarg,"WRITE_MAC")==0) {

				commandCode=COMMAND_MAC_SET_REG;

			} else if(strcmp(optarg,"READ_PHY")==0) {

				commandCode=COMMAND_PHY_GET_REG;

			} else if(strcmp(optarg,"WRITE_PHY")==0) {

				commandCode=COMMAND_PHY_SET_REG;

			} else if(strcmp(optarg,"READ_EEPROM")==0) {

                commandCode=COMMAND_GET_EEPROM;

            } else if(strcmp(optarg,"WRITE_EEPROM")==0) {

                commandCode=COMMAND_SET_EEPROM;

            } else if(strcmp(optarg,"WRITE_FILE_TO_EEPROM")==0) {

                commandCode=COMMAND_WRITE_EEPROM_FROM_FILE;
            } else if(strcmp(optarg,"READ_EEPROM_TO_FILE")==0) {

                commandCode=COMMAND_WRITE_EEPROM_TO_FILE;

            } else if(strcmp(optarg,"VERIFY_EEPROM_WITH_FILE")==0) {

                commandCode=COMMAND_VERIFY_EEPROM_WITH_FILE;

            } else if(strcmp(optarg,"GET_ERRORS")==0) {

                commandCode=COMMAND_GET_ERRORS;
            }

#ifdef RW_MEM
            else if(strcmp(optarg,"READ_BYTE")==0) {

				commandCode=COMMAND_READ_BYTE;

			} else if(strcmp(optarg,"READ_WORD")==0) {

				commandCode=COMMAND_READ_WORD;

			} else if(strcmp(optarg,"READ_DWORD")==0) {

				commandCode=COMMAND_READ_DWORD;

			} else if(strcmp(optarg,"WRITE_BYTE")==0) {

				commandCode=COMMAND_WRITE_BYTE;

			} else if(strcmp(optarg,"WRITE_WORD")==0) {

				commandCode=COMMAND_WRITE_WORD;

			} else if(strcmp(optarg,"WRITE_DWORD")==0) {

				commandCode=COMMAND_WRITE_DWORD;

			}
#endif //RW_MEM
            else {

				goto BAD_USAGE;

			}

			cSet=true;

			break;

		case 'a':

			if(aSet) goto BAD_USAGE;

			if(!ParseNumber(optarg,&address)) {

				goto BAD_USAGE;

			}

			aSet=true;

			break;

		case 'd':

			if(dSet) goto BAD_USAGE;

			if(!ParseNumber(optarg,&data)) {

				goto BAD_USAGE;

			}

			dSet=true;

			break;

		case 'f':

			if(dSet) goto BAD_USAGE;

			if(!ParseString(optarg, &fileName)) {

				goto BAD_USAGE;

			}

			fSet=true;

			break;

		default:

			goto BAD_USAGE;

		}

	}

	if(!Initialize(&commandData,ethName)) {

		return 1;

	}

	switch(commandCode) {

	case COMMAND_GET_CONFIGURATION:

		GetConfiguration(&commandData);

		break;

	case COMMAND_DUMP_LAN_REGS:

		LanDumpRegs(&commandData);

		break;

	case COMMAND_DUMP_MAC_REGS:

		MacDumpRegs(&commandData);

		break;

	case COMMAND_DUMP_PHY_REGS:

		PhyDumpRegs(&commandData);

		break;

	case COMMAND_DUMP_EEPROM:

		DumpEEPROM(&commandData);

		break;

	case COMMAND_GET_MAC_ADDRESS:

		GetMacAddress(&commandData);

		break;

	case COMMAND_SET_MAC_ADDRESS:

		if(!aSet) goto BAD_USAGE;

		if(!dSet) goto BAD_USAGE;

		SetMacAddress(&commandData,address,data);

		break;

	case COMMAND_LOAD_MAC_ADDRESS:

		LoadMacAddress(&commandData);

		break;

	case COMMAND_SAVE_MAC_ADDRESS:

		if(!aSet) goto BAD_USAGE;

		if(!dSet) goto BAD_USAGE;

		SaveMacAddress(&commandData,address,data);

		break;

	case COMMAND_SET_DEBUG_MODE:

		if(!dSet) goto BAD_USAGE;

		SetDebugMode(&commandData,data);

		break;

	case COMMAND_SET_LINK_MODE:

		if(!dSet) goto BAD_USAGE;

		SetLinkMode(&commandData,data);

		break;

	case COMMAND_GET_LINK_MODE:

		GetLinkMode(&commandData);

		break;
	case COMMAND_SET_AMDIX_STS:

		if(!dSet) goto BAD_USAGE;

		SetAutoMdixSts(&commandData,data);

		break;

	case COMMAND_GET_AMDIX_STS:

		GetAutoMdixSts(&commandData);

		break;

	case COMMAND_CHECK_LINK:

		CheckLink(&commandData);

		break;

	case COMMAND_LAN_GET_REG:

		if(!aSet) goto BAD_USAGE;

		LanGetReg(&commandData,address);

		break;

	case COMMAND_LAN_SET_REG:

		if(!aSet) goto BAD_USAGE;

		if(!dSet) goto BAD_USAGE;

		LanSetReg(&commandData,address,data);

		break;

	case COMMAND_MAC_GET_REG:

		if(!aSet) goto BAD_USAGE;

		MacGetReg(&commandData,address);

		break;

	case COMMAND_MAC_SET_REG:

		if(!aSet) goto BAD_USAGE;

		if(!dSet) goto BAD_USAGE;

		MacSetReg(&commandData,address,data);

		break;

	case COMMAND_PHY_GET_REG:

		if(!aSet) goto BAD_USAGE;

		PhyGetReg(&commandData,address);

		break;

	case COMMAND_PHY_SET_REG:

		if(!aSet) goto BAD_USAGE;

		if(!dSet) goto BAD_USAGE;

		PhySetReg(&commandData,address,data);

		break;
    case COMMAND_GET_EEPROM:

        if(!aSet) goto BAD_USAGE;

        GetEeprom(&commandData,address, true);

        break;

    case COMMAND_SET_EEPROM:

        if(!aSet) goto BAD_USAGE;
        if(!dSet) goto BAD_USAGE;

        SetEeprom(&commandData,address,data);

        break;
    case COMMAND_WRITE_EEPROM_FROM_FILE:
    	if(!fSet) goto BAD_USAGE;
        UpdateEepromFromFile(&commandData, fileName);
        break;

    case COMMAND_WRITE_EEPROM_TO_FILE:
    	if(!fSet) goto BAD_USAGE;

        WriteEepromToFile(&commandData, fileName);
        break;

    case COMMAND_VERIFY_EEPROM_WITH_FILE:
    	if(!fSet) goto BAD_USAGE;
        VerifyEepromWithFile(&commandData, fileName);
        break;

    case COMMAND_GET_ERRORS:
        GetErrors(&commandData);
        break;

#ifdef RW_MEM

	case COMMAND_READ_BYTE:

		if(!aSet) goto BAD_USAGE;

		ReadByte(&commandData,address);

		break;

	case COMMAND_READ_WORD:

		if(!aSet) goto BAD_USAGE;

		ReadWord(&commandData,address);

		break;

	case COMMAND_READ_DWORD:

		if(!aSet) goto BAD_USAGE;

		ReadDWord(&commandData,address);

		break;

	case COMMAND_WRITE_BYTE:

		if(!aSet) goto BAD_USAGE;

		if(!dSet) goto BAD_USAGE;

		WriteByte(&commandData,address,data);

		break;

	case COMMAND_WRITE_WORD:

		if(!aSet) goto BAD_USAGE;

		if(!dSet) goto BAD_USAGE;

		WriteWord(&commandData,address,data);

		break;

	case COMMAND_WRITE_DWORD:

		if(!aSet) goto BAD_USAGE;

		if(!dSet) goto BAD_USAGE;

		WriteDWord(&commandData,address,data);

		break;
#endif //RW_MEM
	default:

		goto BAD_USAGE;

	}

	return 1;

BAD_USAGE:

	DisplayUsage();

	return 1;

}

