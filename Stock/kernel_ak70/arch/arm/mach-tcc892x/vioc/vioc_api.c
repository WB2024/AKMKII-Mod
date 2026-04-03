/******************************************************************************
*
*  (C)Copyright All Rights Reserved by Telechips Inc.
*
*  This material is confidential and shall remain as such.
*  Any unauthorized use, distribution, reproduction is strictly prohibited.
*
*******************************************************************************
*
*  File Name   : Vioc_api.c
*
*  Description : VIOC user application interface Module
*
*******************************************************************************
*
*  yyyy/mm/dd     ver            descriptions                Author
*	---------	--------   ---------------       -----------------
*    2011/08/08     0.1            created                      hskim
*******************************************************************************/

#include <linux/kernel.h>
#include <mach/bsp.h>
#include <mach/io.h>
#include <linux/string.h>

#include <mach/vioc_api.h>

#include <mach/vioc_scaler.h>
#include <mach/vioc_rdma.h>
#include <mach/vioc_wdma.h>
#include <mach/vioc_wmix.h>
#include <mach/vioc_disp.h>
#include <mach/vioc_ireq.h>
#include <mach/vioc_config.h>
#include <mach/vioc_plugin_tcc892x.h>
#include <mach/vioc_outcfg.h>


/* Scaler Set */
/*
	API : VIOC_API_SCALER_Set/GetXXX()
*/
void VIOC_API_SCALER_SetConfig(unsigned int scaler, VIOC_SCALER_INFO_Type * Info)
{
	volatile PVIOC_SC pScaler;
	pScaler = (volatile PVIOC_SC)tcc_p2v((unsigned int)HwVIOC_SC0 + scaler*0x100);

	if(Info->BYPASS == TRUE) 	VIOC_SC_SetBypass(pScaler, TRUE);
 	else 						VIOC_SC_SetBypass(pScaler, FALSE);

	//VIOC_SC_SetSrcSize((VIOC_SC *)baseAddr, Info->SRC_WIDTH, Info->SRC_HEIGHT);
	VIOC_SC_SetDstSize(pScaler, Info->DST_WIDTH, Info->DST_HEIGHT);
	VIOC_SC_SetOutSize(pScaler, Info->OUTPUT_WIDTH, Info->OUTPUT_HEIGHT);
	VIOC_SC_SetOutPosition(pScaler, Info->OUTPUT_POS_X, Info->OUTPUT_POS_Y);
}

void VIOC_API_SCALER_SetUpdate(unsigned int scaler)
{
	volatile PVIOC_SC pScaler;
	pScaler = (volatile PVIOC_SC)tcc_p2v((unsigned int)HwVIOC_SC0 + scaler*0x100);

	VIOC_SC_SetUpdate(pScaler);
}

int VIOC_API_SCALER_SetInterruptEnable(unsigned int scaler, unsigned int interrupt)
{
	int ret = 0; // VIOC_DRIVER_NOERR;
	volatile PVIOC_SC pScaler;
	pScaler = (volatile PVIOC_SC)tcc_p2v((unsigned int)HwVIOC_SC0 + scaler*0x100);

	if(scaler >= VIOC_SCALER_MAX)
	{
		ret = -3; // VIOC_DRIVER_ERR_INVALID;
	}
	else
	{
		VIOC_SC_SetInterruptMask(pScaler, interrupt, FALSE);
	}

	return ret;
}

int VIOC_API_SCALER_SetInterruptDiable(unsigned int scaler, unsigned int interrupt)
{
	int ret = 0; // VIOC_DRIVER_NOERR;
	volatile PVIOC_SC pScaler;
	pScaler = (volatile PVIOC_SC)tcc_p2v((unsigned int)HwVIOC_SC0 + scaler*0x100);

	if(scaler >= VIOC_SCALER_MAX)
	{
		ret = -3; // VIOC_DRIVER_ERR_INVALID;
	}
	else
	{
		VIOC_SC_SetInterruptMask(pScaler, interrupt, TRUE);
	}

	return ret;
}

int VIOC_API_SCALER_SetPlugIn(unsigned int scaler, unsigned int path)
{
	int ret;

	ret = VIOC_CONFIG_PlugIn(scaler, path);
	if(ret == VIOC_DEVICE_CONNECTED)
		ret = VIOC_DRIVER_NOERR;
	else
		ret = VIOC_DRIVER_ERR;

	return ret;
}

int VIOC_API_SCALER_SetPlugOut(unsigned int scaler)
{
	int ret;

	ret = VIOC_CONFIG_PlugOut(scaler);
	if(ret == VIOC_PATH_DISCONNECTED)
		ret = VIOC_DRIVER_NOERR;
	else
		ret = VIOC_DRIVER_ERR;

	return ret;
}

void VIOC_API_IREQ_Init(void)
{
	VIOC_IREQ_RegisterHandle();
}


void vioc_display_device_reset(unsigned int device_num)
{
	#define DD0_DMA_CONNECT_CHECK(PlugInOut) 	((PlugInOut.connect_device == VIOC_SC_RDMA_00)|| (PlugInOut.connect_device == VIOC_SC_RDMA_01) \
													||(PlugInOut.connect_device == VIOC_SC_RDMA_02) || (PlugInOut.connect_device == VIOC_SC_RDMA_03) \
													|| (PlugInOut.connect_device == VIOC_SC_WDMA_00))
													
	#define DD1_DMA_CONNECT_CHECK(PlugInOut) 	((PlugInOut.connect_device == VIOC_SC_RDMA_04)|| (PlugInOut.connect_device == VIOC_SC_RDMA_05) \
													||(PlugInOut.connect_device == VIOC_SC_RDMA_06) || (PlugInOut.connect_device == VIOC_SC_RDMA_07)  \
													|| (PlugInOut.connect_device == VIOC_SC_WDMA_01))
	volatile PVIOC_IREQ_CONFIG pIREQConfig;

	volatile VIOC_DISP DISPBackup , *pDISPBackup;
	volatile VIOC_WMIX WMIXBackup, *pWMIXBackup;
	volatile VIOC_RDMA RDMA0Backup, RDMA1Backup, RDMA2Backup, RDMA3Backup;
	volatile VIOC_RDMA *pRDMA0Backup, *pRDMA1Backup, *pRDMA2Backup, *pRDMA3Backup;
	
	volatile unsigned int WMIXER_N, RDMA_0, RDMA_1, RDMA_2, RDMA_3;

	//RDMA check PlugInOut of VIOC plug in/out block 
	volatile VIOC_PlugInOutCheck VIOC_PlugIn[VIOC_VIQE + 1];
	volatile unsigned int VIOC_PlugIn_reset[VIOC_VIQE + 1];
	volatile VIOC_SC SC0_Backup, SC1_Backup, SC2_Backup, SC3_Backup;
	volatile VIQE ViqeBackup;
	volatile int i;
	
	pIREQConfig = (volatile PVIOC_IREQ_CONFIG)tcc_p2v((unsigned int)HwVIOC_IREQ);

	for(i = 0; i <= VIOC_VIQE; i++)
	{
		VIOC_PlugIn_reset[i] = false;		

		VIOC_CONFIG_Device_PlugState(i, &VIOC_PlugIn[i]);

		if(VIOC_PlugIn[i].enable)
		{
			if(device_num) {
				if(DD1_DMA_CONNECT_CHECK(VIOC_PlugIn[i]))
					VIOC_PlugIn_reset[i] = true;
			}
			else	{
				if(DD0_DMA_CONNECT_CHECK(VIOC_PlugIn[i]))
					VIOC_PlugIn_reset[i] = true;
			}
		}
	}
	
	if(device_num)	{
		pDISPBackup = (VIOC_DISP *)tcc_p2v(HwVIOC_DISP1);
		pWMIXBackup = (VIOC_WMIX *)tcc_p2v(HwVIOC_WMIX1);

		pRDMA0Backup = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA04);
		pRDMA1Backup = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA05);
		pRDMA2Backup = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA06);
		pRDMA3Backup = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA07);

		WMIXER_N = VIOC_WMIX1;

		RDMA_0 = VIOC_WMIX_RDMA_04;
		RDMA_1 = VIOC_WMIX_RDMA_05;
		RDMA_2 = VIOC_WMIX_RDMA_06;
		RDMA_3 = VIOC_WMIX_RDMA_07;
	}
	else 	{
		pDISPBackup = (VIOC_DISP *)tcc_p2v(HwVIOC_DISP0);
		pWMIXBackup = (VIOC_WMIX *)tcc_p2v(HwVIOC_WMIX0);
		
		pRDMA0Backup = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA00);
		pRDMA1Backup = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA01);
		pRDMA2Backup = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA02);
		pRDMA3Backup = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA03);

		WMIXER_N = VIOC_WMIX0;
		
		RDMA_0 = VIOC_WMIX_RDMA_00;
		RDMA_1 = VIOC_WMIX_RDMA_01;
		RDMA_2 = VIOC_WMIX_RDMA_02;
		RDMA_3 = VIOC_WMIX_RDMA_03;
	}

	// backup RDMA, WMIXER, DisplayDevice register
	RDMA0Backup = *pRDMA0Backup;
	RDMA1Backup = *pRDMA1Backup;
	RDMA2Backup = *pRDMA2Backup;
	RDMA3Backup = *pRDMA3Backup;

	DISPBackup = *pDISPBackup;
	
	WMIXBackup = *pWMIXBackup;

	if(VIOC_PlugIn_reset[VIOC_SC0])
		SC0_Backup = *(VIOC_SC *)tcc_p2v(HwVIOC_SC0);

	if(VIOC_PlugIn_reset[VIOC_SC1])
		SC1_Backup = *(VIOC_SC *)tcc_p2v(HwVIOC_SC1);

	if(VIOC_PlugIn_reset[VIOC_SC2])
		SC2_Backup = *(VIOC_SC *)tcc_p2v(HwVIOC_SC2);

	if(VIOC_PlugIn_reset[VIOC_SC3])
		SC3_Backup = *(VIOC_SC *)tcc_p2v(HwVIOC_SC3);

	if(VIOC_PlugIn_reset[VIOC_VIQE])
		ViqeBackup = *(VIQE *)tcc_p2v(HwVIOC_VIQE0_BASE);	
	
	// h/w block reset
	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(20+device_num)), (0x1<<(20+device_num))); // disp reset

	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(0+device_num)), (0x1<<(0+device_num))); // wdma reset

	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(9+WMIXER_N)), (0x1<<(9+WMIXER_N))); // wmix reset

	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<RDMA_0), (0x1<<RDMA_0)); // rdma reset
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<RDMA_1), (0x1<<RDMA_1)); // rdma reset	
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<RDMA_2), (0x1<<RDMA_2)); // rdma reset
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<RDMA_3), (0x1<<RDMA_3)); // rdma reset


	if(VIOC_PlugIn_reset[VIOC_SC0])
		BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<(28+VIOC_SC0)), (0x1<<(28+VIOC_SC0))); // scaler 0 reset

	if(VIOC_PlugIn_reset[VIOC_SC1])
		BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<(28+VIOC_SC1)), (0x1<<(28+VIOC_SC1))); // scaler 1 reset

	if(VIOC_PlugIn_reset[VIOC_SC2])
		BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<(28+VIOC_SC2)), (0x1<<(28+VIOC_SC2))); // scaler 2 reset

	if(VIOC_PlugIn_reset[VIOC_SC3])
		BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<(28+VIOC_SC3)), (0x1<<(28+VIOC_SC3))); // scaler 3 reset

	if(VIOC_PlugIn_reset[VIOC_VIQE])
		BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(16)), (0x1<<(16))); // VIQE reset

		
	// h/w block release reset 
	if(VIOC_PlugIn_reset[VIOC_VIQE])
		BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(16)), (0x0<<(16))); // VIQE reset

	if(VIOC_PlugIn_reset[VIOC_SC3])
		BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<(28+VIOC_SC3)), (0x0<<(28+VIOC_SC3))); // scaler 3 reset

	if(VIOC_PlugIn_reset[VIOC_SC2])
		BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<(28+VIOC_SC2)), (0x0<<(28+VIOC_SC2))); // scaler 2 reset

	if(VIOC_PlugIn_reset[VIOC_SC1])
		BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<(28+VIOC_SC1)), (0x0<<(28+VIOC_SC1))); // scaler 1 reset

	if(VIOC_PlugIn_reset[VIOC_SC0])
		BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<(28+VIOC_SC0)), (0x0<<(28+VIOC_SC0))); // scaler 0 reset
	
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<RDMA_0), (0x0<<RDMA_0)); // rdma reset release
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<RDMA_1), (0x0<<RDMA_1)); // rdma reset	
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<RDMA_2), (0x0<<RDMA_2)); // rdma reset
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<RDMA_3), (0x0<<RDMA_3)); // rdma reset 
	
	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(9+WMIXER_N)), (0x0<<(9+WMIXER_N))); // wmix reset release

	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(0+device_num)), (0x0<<(0+device_num))); // wdma reset release

	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(0+device_num)), (0x0<<(0+device_num))); // wdma reset release

	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(20+device_num)), (0x0<<(20+device_num))); // disp reset release


	// restore VIQE, SCALER , RDMA, WMIXER, DisplayDevice register
	if(VIOC_PlugIn_reset[VIOC_VIQE]) {
		*(VIQE *)tcc_p2v(HwVIOC_VIQE0_BASE) = ViqeBackup;
	}

	if(VIOC_PlugIn_reset[VIOC_SC3]) {
		*(VIOC_SC *)tcc_p2v(HwVIOC_SC3) = SC3_Backup;
		VIOC_API_SCALER_SetUpdate(VIOC_SC3); // scaler 3 update
	}

	if(VIOC_PlugIn_reset[VIOC_SC2]) {
		*(VIOC_SC *)tcc_p2v(HwVIOC_SC2) = SC2_Backup;
		VIOC_API_SCALER_SetUpdate(VIOC_SC2); // scaler 2 update		
	}
	
	if(VIOC_PlugIn_reset[VIOC_SC1]) {
		*(VIOC_SC *)tcc_p2v(HwVIOC_SC1) = SC1_Backup;
		VIOC_API_SCALER_SetUpdate(VIOC_SC1); // scaler 1 update
	}

	if(VIOC_PlugIn_reset[VIOC_SC0]) {
		*(VIOC_SC *)tcc_p2v(HwVIOC_SC0) = SC0_Backup;
		VIOC_API_SCALER_SetUpdate(VIOC_SC0); // scaler 0 update
	}
		
	*pRDMA0Backup = RDMA0Backup;
	*pRDMA1Backup = RDMA1Backup;
	*pRDMA2Backup = RDMA2Backup;
	*pRDMA3Backup = RDMA3Backup;

	*pWMIXBackup = WMIXBackup;

	VIOC_RDMA_SetImageUpdate(pRDMA0Backup);
	VIOC_RDMA_SetImageUpdate(pRDMA1Backup);
	VIOC_RDMA_SetImageUpdate(pRDMA2Backup);
	VIOC_RDMA_SetImageUpdate(pRDMA3Backup);

	VIOC_WMIX_SetUpdate(pWMIXBackup);

	*pDISPBackup = DISPBackup;
	printk("VIOC_sc0:%d VIOC_sc1:%d VIOC_sc2:%d VIOC_sc3:%d VIOC_VIQE:%d  \n", VIOC_PlugIn_reset[0], VIOC_PlugIn_reset[1], VIOC_PlugIn_reset[2], VIOC_PlugIn_reset[3], VIOC_PlugIn_reset[4]);
}


void vioc_disp_port_change(unsigned int lcd_org_num, unsigned int lcd_dst_num)
{

	volatile PVIOC_IREQ_CONFIG pIREQConfig;
	pIREQConfig = (volatile PVIOC_IREQ_CONFIG)tcc_p2v((unsigned int)HwVIOC_IREQ);

	volatile VIOC_DISP DISPBackup , *pDISPBackup, *pDISPBase;
	volatile VIOC_WMIX WMIXBackup, *pWMIXBackup,*pWMIXBase,*pWMIXBase_HDMI;
	volatile VIOC_RDMA RDMA0Backup, RDMA1Backup, RDMA2Backup, RDMA3Backup;
	volatile VIOC_RDMA *pRDMA0Backup, *pRDMA1Backup, *pRDMA2Backup, *pRDMA3Backup;
	volatile VIOC_RDMA *pRDMA0Base,*pRDMA1Base,*pRDMA2Base,*pRDMA3Base;
	volatile unsigned int WMIXER_N, RDMA_0, RDMA_1, RDMA_2, RDMA_3,DD_N;
	volatile unsigned int WDMA_N;

	PCKC 	pCKC;
	pCKC = (CKC*)tcc_p2v(HwCKC_BASE);

	CKC CKC_BackUp;
	
	memcpy(&CKC_BackUp, (PCKC)pCKC, sizeof(CKC));

	if(lcd_org_num)
	{
		pRDMA0Backup = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA04);
		pRDMA3Backup = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA07);
		pWMIXBackup = (VIOC_WMIX *)tcc_p2v(HwVIOC_WMIX1);
		pDISPBackup = (VIOC_DISP *)tcc_p2v(HwVIOC_DISP1);

		RDMA_0 = VIOC_WMIX_RDMA_04;
		RDMA_1 = VIOC_WMIX_RDMA_05;
		RDMA_2 = VIOC_WMIX_RDMA_06;
		RDMA_3 = VIOC_WMIX_RDMA_07;

		WMIXER_N = VIOC_WMIX1;
		WDMA_N = 1;
	}
	else
	{
		pRDMA0Backup = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA00);
		pRDMA3Backup = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA03);
		pWMIXBackup = (VIOC_WMIX *)tcc_p2v(HwVIOC_WMIX0);
		pDISPBackup = (VIOC_DISP *)tcc_p2v(HwVIOC_DISP0);
			
		RDMA_0 = VIOC_WMIX_RDMA_00;
		RDMA_1 = VIOC_WMIX_RDMA_01;
		RDMA_2 = VIOC_WMIX_RDMA_02;
		RDMA_3 = VIOC_WMIX_RDMA_03;

		WMIXER_N = VIOC_WMIX0;
		WDMA_N = 0;
	}

	if(lcd_dst_num)
	{
		pCKC->PCLKCTRL05.nREG = CKC_BackUp.PCLKCTRL03.nREG; 	//LCDC0 CLK -> LCDC1 CLK

		pRDMA0Base = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA04);
		pRDMA3Base = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA07);
		
		pWMIXBase = (VIOC_WMIX*)tcc_p2v(HwVIOC_WMIX1);
		pWMIXBase_HDMI = (VIOC_WMIX*)tcc_p2v(HwVIOC_WMIX0);
		pDISPBase = (VIOC_DISP *)tcc_p2v(HwVIOC_DISP1);
	}
	else
	{		
		pCKC->PCLKCTRL03.nREG = CKC_BackUp.PCLKCTRL05.nREG;	//LCDC1 CLK -> LCDC0 CLK
		
		pRDMA0Base = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA00);
		pRDMA3Base = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA03);

		pWMIXBase = (VIOC_WMIX*)tcc_p2v(HwVIOC_WMIX0);		
		pWMIXBase_HDMI = (VIOC_WMIX*)tcc_p2v(HwVIOC_WMIX1);
		pDISPBase = (VIOC_DISP *)tcc_p2v(HwVIOC_DISP0);
	}

	VIOC_DISP_TurnOff(pDISPBackup);

	VIOC_RDMA_SetImageDisable(pRDMA3Backup);	//disable video channel
	
	// backup RDMA, WMIXER, DisplayDevice register
	
	RDMA0Backup = *pRDMA0Backup;
	//RDMA1Backup = *pRDMA1Backup;
	//RDMA2Backup = *pRDMA2Backup;	
	
	WMIXBackup = *pWMIXBackup;
	DISPBackup = *pDISPBackup;
	
	// h/w block reset
	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(20+lcd_org_num)), (0x1<<(20+lcd_org_num))); // disp reset

	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<WDMA_N), (0x1<<WDMA_N)); // wdma reset

	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(9+WMIXER_N)), (0x1<<(9+WMIXER_N))); // wmix reset
	
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<RDMA_0), (0x1<<RDMA_0)); // rdma reset	
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<RDMA_3), (0x1<<RDMA_3)); // rdma reset 
		
	// h/w block normal
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<RDMA_0), (0x0<<RDMA_0)); // rdma reset release
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<RDMA_3), (0x0<<RDMA_3)); // rdma reset 
	
	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(9+WMIXER_N)), (0x0<<(9+WMIXER_N))); // wmix reset release
	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<WDMA_N), (0x0<<WDMA_N)); // wdma reset
	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(20+lcd_org_num)), (0x0<<(20+lcd_org_num))); // disp reset release

	if(lcd_org_num)
	{
		BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(20+lcd_dst_num)), (0x1<<(20+lcd_dst_num))); // disp reset
		BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<0), (0x1<<0)); // wdma reset
		BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(9+VIOC_WMIX0)), (0x1<<(9+VIOC_WMIX0))); // wmix reset
		BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<VIOC_WMIX_RDMA_00), (0x1<<VIOC_WMIX_RDMA_00)); // rdma reset
		
		BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (0x1<<VIOC_WMIX_RDMA_00), (0x0<<VIOC_WMIX_RDMA_00)); // rdma normal
		BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(9+VIOC_WMIX0)), (0x0<<(9+VIOC_WMIX0))); // wmix reset release
		BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<0), (0x0<<0)); // wdma reset
		BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (0x1<<(20+lcd_dst_num)), (0x0<<(20+lcd_dst_num))); // disp reset release		
	}
	
	// restore RDMA, WMIXER, DisplayDevice register
	*pRDMA0Base = RDMA0Backup;
	//*pRDMA3Base = RDMA3Backup;
	//*pWMIXBase = WMIXBackup;	
	
	VIOC_RDMA_SetImageUpdate(pRDMA0Base);
	VIOC_RDMA_SetImageUpdate(pRDMA3Base);

	if(!lcd_dst_num)
		VIOC_WMIX_SetOverlayPriority(pWMIXBase,24);		//lcd  priority
	else	
		VIOC_WMIX_SetOverlayPriority(pWMIXBase_HDMI,24);	//hdmi priority

	VIOC_WMIX_SetUpdate(pWMIXBase);

	*pDISPBase = DISPBackup;
	VIOC_DISP_TurnOn(pDISPBase);
	
	printk(" %s LCDC:%d  pRDMABase:%p  pWMIXBase:%p pDISPBase:%p end \n", __func__, lcd_dst_num, pRDMA0Base, pWMIXBase, pDISPBase);

}
