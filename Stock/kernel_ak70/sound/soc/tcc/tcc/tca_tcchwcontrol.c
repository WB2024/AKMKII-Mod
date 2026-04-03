
/****************************************************************************
 *   FileName    : tca_tcchwcontrol.c
 *   Description : 
 ****************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips, Inc.
 *   ALL RIGHTS RESERVED
 *
 ****************************************************************************/
/*****************************************************************************
*
* includes
*
******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <mach/bsp.h>
#include <mach/io.h>
#include <mach/gpio.h>
#include <asm/mach-types.h>
#include "tca_tcchwcontrol.h"
#include <mach/board_astell_kern.h>

/* System revision Info */
extern unsigned int system_rev; 

/*****************************************************************************
* Function Name : tca_dma_clrstatus()
******************************************************************************
* Desription    :
* Parameter     : 
* Return        : success(SUCCESS) 
******************************************************************************/
unsigned int tca_dma_clrstatus(void *pADMABaseAddr, unsigned int nDmanum)
{
	ADMA *pStrADMABaseReg = (ADMA *)pADMABaseAddr;
	if (nDmanum) {
		//If you use this register to write, set interrupt status is cleared.
        pStrADMABaseReg->IntStatus |= (Hw6 | Hw2);
    } 
	else 
	{
		//If you use this register to write, set interrupt status is cleared.
        pStrADMABaseReg->IntStatus |= (Hw4 | Hw0);
    }

	return 0/*SUCCESS*/;
}

/*****************************************************************************
* Function Name : tca_irq_getnumber()
******************************************************************************
* Desription    :
* Parameter     : 
* Return        : DMA number 
******************************************************************************/
unsigned int tca_irq_getnumber(void)
{
#if defined(CONFIG_ARCH_TCC892X) || defined(CONFIG_ARCH_TCC893X)
    return INT_ADMA0;
#else
    return IRQ_ADMA;
#endif
}

/*****************************************************************************
* Function Name : tca_dma_getstatus()
******************************************************************************
* Desription    : dma status check
* Parameter     : 
* Return        :  
******************************************************************************/
unsigned int tca_dma_getstatus(void *pADMABaseAddr, unsigned int nDmanum)
{
	ADMA *pStrADMABaseReg = (ADMA *)pADMABaseAddr;
    return nDmanum ? (pStrADMABaseReg->IntStatus & (Hw6 | Hw2)) : (pStrADMABaseReg->IntStatus & (Hw4 | Hw0));
}

/*****************************************************************************
* Function Name : tca_tcc_initport()
******************************************************************************
* Desription    : GPIO port init
* Parameter     : 
* Return        :  
******************************************************************************/
#if defined(CONFIG_ARCH_TCC892X) || defined(CONFIG_ARCH_TCC893X)
unsigned int tca_tcc_initport(void)
{
	
	/* SPDIF Tx */
	if(machine_is_tcc8920st() || machine_is_tcc8930st())
	{
    	tcc_gpio_config(TCC_GPG(5), GPIO_FN1);
	}
	else
	{
#if defined(CONFIG_SOC_TCC8925S)	// Planet 20121025 SPDIF exists only on channel01
		tcc_gpio_config(TCC_GPG(18), GPIO_FN1);
#else
    //if (system_rev == 0x1006) {
    //2013.11.19 iriver.ysyim: TCC_GPG(18) is GPIO_CHG_DONE
  //      tcc_gpio_config(TCC_GPG(18), GPIO_FN1);		// SPDIF Planet 20121107
    //}
    //else {
    //    tcc_gpio_config(TCC_GPG(5), GPIO_FN1);
    //}
#endif
	}

	
	// AKJR2 evm.  
	
	//FUNC1
	//MDAI_MCLK 
	//MDAI_BCLK 
	//MDAI_LRCK 
	//MDAI_DO[00] 
	//MDAI_DI[00] 

    tcc_gpio_config(TCC_GPG(6), GPIO_FN1);
    tcc_gpio_config(TCC_GPG(7), GPIO_FN1);
    tcc_gpio_config(TCC_GPG(8), GPIO_FN1);
    tcc_gpio_config(TCC_GPG(9), GPIO_FN1);
    tcc_gpio_config(TCC_GPG(10), GPIO_FN1);

    return 0/*SUCCESS*/;
}
#else
unsigned int tca_tcc_initport(void)
{
	GPIO *pStrGPIOBaseReg = (GPIO *)tcc_p2v(HwGPIO_BASE);
    /* Clear GPIO function bit for DAI & SPDIF */
    if(machine_is_m57te() || machine_is_m801()) {
        pStrGPIOBaseReg->GPDFN0 &= ~(Hw20 - Hw0);
    }
    else 
    {
        pStrGPIOBaseReg->GPDFN0 &= ~(Hw20 - Hw0);

	//20110224 koo : CONFIG_MACH_TCC9300ST에서 GPIO_D[9:10]을 TSD[7:6]으로 사용하고 있는데 아래 부분에서 임의로 clear 시켜 해당 부분 제외.
	//pStrGPIOBaseReg->GPDFN1 &= ~(Hw16 - Hw0);
	 if(machine_is_tcc9300st()) {
	 	pStrGPIOBaseReg->GPDFN1 &= ~((Hw16-Hw12) | (Hw4-Hw0));     /* Clear GPIO funtion bit for DAI and SPDIF */
	 } else {
	 	pStrGPIOBaseReg->GPDFN1 &= ~(Hw16 - Hw0);
	 }
    }

    /* Set GPIO function register for DAI & SPDIF */
    if(machine_is_tcc9200s() || machine_is_m57te() || machine_is_m801()) {
        pStrGPIOBaseReg->GPDFN0 |= 0
            | Hw0   //BCLK(1)
            | Hw4   //LRCLK(1)
            | Hw8   //MCLK(1)
            | Hw12  //DAO0(1)
            | Hw16  //DAI0(1)
            ;
    }
    else if(machine_is_tcc9200() || machine_is_tcc9201() || machine_is_tcc8900()) {
        pStrGPIOBaseReg->GPDFN0 |= 0
            | Hw0   //BCLK(1)
            | Hw4   //LRCLK(1)
            | Hw8   //MCLK(1)
            | Hw12  //DAO0(1)
            | Hw16  //DAI0(1)
            | Hw20  //DAO1(1)
            | Hw24  //DAI1(1)
            | Hw28  //DAO2(1)
            ;
        pStrGPIOBaseReg->GPDFN1 |= 0
            | Hw0   //DAI2(1)
            | Hw4   //DAO3(1)
            | Hw8   //DAI3(1)
            | Hw12  //SPDIF_Tx
            ;
    }

    if(machine_is_tcc9300() || machine_is_tcc9300cm() || machine_is_tcc9300st() || machine_is_tcc8800st() || machine_is_tcc8920st() || machine_is_tcc8930st()) {
        // Set GPIO for DAI
        pStrGPIOBaseReg->GPDFN0 |= 0
            | Hw0   //BCLK(1)
            | Hw4   //LRCLK(1)
            | Hw8   //MCLK(1)
            | Hw12  //DAO0(1)
            | Hw16  //DAI0(1)
            ;
        pStrGPIOBaseReg->GPDFN1 |= 0
            | Hw12  //SPDIF_Tx
            ;
    }

    if(machine_is_tcc8800()) {
        //Set GPIO for Audio
        pStrGPIOBaseReg->GPDFN0 |= 0
            | Hw0   //BCLK(1)
            | Hw4   //LRCLK(1)
            | Hw8   //MCLK(1)
            | Hw12  //DAO0(1)
            | Hw16  //DAI0(1)
            ;

        pStrGPIOBaseReg->GPDFN1 |= 0
            | Hw12  //SPDIF_Tx
            ;

        pStrGPIOBaseReg->GPDCD0 &= 0xFF000000;
        if(system_rev >= 0x0600) {
            pStrGPIOBaseReg->GPDCD0 |= 0x00000010;
        }

    }
    else if(machine_is_m801_88() || machine_is_m803()) {
        //Set GPIO for Audio
        pStrGPIOBaseReg->GPDFN0 |= 0
            | Hw0   //BCLK(1)
            | Hw4   //LRCLK(1)
            | Hw8   //MCLK(1)
            | Hw12  //DAO0(1)
            | Hw16  //DAI0(1)
            ;

        pStrGPIOBaseReg->GPDCD0 &= 0xFF000000;
        pStrGPIOBaseReg->GPDCD0 |= 0x00000000;
    }

    return 0/*SUCCESS*/;
}
#endif

/*****************************************************************************
* Function Name : tca_dma_setsrcaddr()
******************************************************************************
* Desription    : dma set source address
* Parameter     : dma number
* Return        : address
******************************************************************************/
unsigned int tca_dma_setsrcaddr(void *pADMABaseAddr, unsigned int DADONum, unsigned int nDmanum, unsigned int nAddr)	
{
	ADMA *pStrADMABaseReg = (ADMA *)pADMABaseAddr;

	switch(DADONum)
	{
    	case 0:
			pStrADMABaseReg->TxDaSar = nAddr;
			break;
		case 1:
			pStrADMABaseReg->TxDaSar1 = nAddr;
			break;
		case 2:
			pStrADMABaseReg->TxDaSar2 = nAddr;
			break;
		case 3:
			pStrADMABaseReg->TxDaSar3 = nAddr;
			break;
		default:
			break;
	}
	
	return 0/*SUCCESS*/;
}

/*****************************************************************************
* Function Name : tca_dma_setdestaddr()
******************************************************************************
* Desription	: set dest address
* Parameter 	: dma number, dest address
* Return		: SUCCESS 
******************************************************************************/
unsigned int tca_dma_setdestaddr(void *pADMABaseAddr, unsigned int DADINum, unsigned int nDmanum, unsigned int nAddr)	
{
	ADMA *pStrADMABaseReg = (ADMA *)pADMABaseAddr;	
	
	switch(DADINum)
	{
		case 0:
			pStrADMABaseReg->RxDaDar = nAddr;
			break;
		case 1:
			pStrADMABaseReg->RxDaDar1 = nAddr;
			break;
		case 2:
			pStrADMABaseReg->RxDaDar2 = nAddr;
			break;
		case 3:
			pStrADMABaseReg->RxDaDar3 = nAddr;
			break;
		default:
			break;
	}
	return 0/*SUCCESS*/;
}

/*****************************************************************************
* Function Name : tca_dma_control()
******************************************************************************
* Desription    : dma input control
* Parameter     : mode 1: on, 0:off
* Return        : success(SUCCESS) 
******************************************************************************/
unsigned int tca_dma_control(void *pADMABaseAddr, void *pADMADAIBaseAddr, unsigned int nMode, unsigned int nDmanum, unsigned int nInMode)				
{  
	ADMA *pStrADMABaseReg = (ADMA *)pADMABaseAddr;
	ADMADAI *pStrADMADAIBaseReg = (ADMADAI *)pADMADAIBaseAddr; 
	
    if(nMode) 
	{ //run
        tca_i2s_start(pADMADAIBaseAddr, nInMode);
#if defined(_WM8581_INCLUDE_)		
		pStrADMABaseReg->RptCtrl = Hw26|Hw25|Hw24;//DAI Buffer Threshod is set Multi-Port
#else
		pStrADMABaseReg->RptCtrl = 0;//Initialize repeat control register
#endif
		if(nInMode)//input
		{
			pStrADMABaseReg->ChCtrl |= 0
				| Hw30 //DMA Channel enable of DAI Rx
				| Hw14 //Width of Audio Data of DAI Rx 16bites
				| Hw2; //Interrupt enable of DAI Rx
		}
		else//output
		{
			pStrADMABaseReg->ChCtrl |= 0
				| Hw28 //DMA Channel enable of DAI Tx
#if defined(_WM8581_INCLUDE_)
				| Hw21 | Hw20//DMA Multi channel select : DAI0, DAI1, DAI2, DAI3
#endif
				| Hw12	//Width of Audio Data of DAI Tx 16bites
				| Hw0; //Interrupt enable of DAI Tx
		}
    }
	else 
	{
        tca_i2s_stop(pADMADAIBaseAddr, nInMode);
		if(nInMode)//input
		{
			pStrADMABaseReg->ChCtrl &= ~Hw30; //DMA Channel disable of DAI Rx
		}
		else//output
		{
			pStrADMABaseReg->ChCtrl &= ~Hw28; //DMA Channel disable of DAI Tx
		}
    }
	return 0/*SUCCESS*/;
}

/*****************************************************************************
* Function Name : tca_i2s_setregister()
******************************************************************************
* Desription    : set dai register
* Parameter     : 
* Return        : success(SUCCESS) 
******************************************************************************/
unsigned int tca_i2s_setregister(void *pADMADAIBaseAddr, unsigned int nRegval)
{
	ADMADAI*pStrADMADAIBaseReg = (ADMADAI *)pADMADAIBaseAddr;
	pStrADMADAIBaseReg->DAMR = nRegval;
	return 0;
}

/*****************************************************************************
* Function Name : tca_i2s_getregister()
******************************************************************************
* Desription    : get dai register
* Parameter     : 
* Return        : success(SUCCESS) 
******************************************************************************/
unsigned int tca_i2s_getregister(void *pADMADAIBaseAddr)
{
	ADMADAI *pStrADMADAIBaseReg = (ADMADAI *)pADMADAIBaseAddr;
	return (pStrADMADAIBaseReg->DAMR);
}

/*****************************************************************************
* Function Name : tca_i2s_dai_init()
******************************************************************************
* Desription    : i2s init
* Parameter     : 
* Return        : success(SUCCESS) 
******************************************************************************/
unsigned int tca_i2s_dai_init(void *pADMADAIBaseAddr)
{
	ADMADAI *pStrADMADAIBaseReg = (ADMADAI *)pADMADAIBaseAddr;
	
 	//Not used volume control
	pStrADMADAIBaseReg->DAVC = 0;
	#ifdef 	AK_WM8804_MASTER
	DBG_PRINT("TCA","%x\n",pStrADMADAIBaseReg);
	pStrADMADAIBaseReg->DAMR = 0
		| Hw31	//BCLK direct at master mode
		| Hw30	//LRCLK direct at master mode	
#if defined(_MULTIL_DAO_INCLUDE_)
		| Hw29	//DAI Buffer Threshold Enable
		| Hw28	//Multi-Port Enable
		//| Hw17	//Loop back test
#endif
		| Hw21| Hw20 //DAI Rx shift Bit-pack LSB and 16bit mode
		| Hw19| Hw18 //DAI Tx shift Bit-pack LSB and 16bit mode
		| Hw15	//DAI Master enable
		// tonny for slave(Hw11 : 0)
		| Hw11	//DAI System Clock master select
		| Hw10	//DAI Bit Clock master select
		| Hw9	//DAI Frame clock(LRCLK) master selcet 
		| Hw5	//DAI LRCLK(64fs->fs)
		;

	pStrADMADAIBaseReg->DAMR &= 
		~(
		 Hw11 	//DAI System Clock master select
		| Hw10 	//DAI Bit Clock master select
		| Hw9	//DAI Frame clock(LRCLK) master selcet 
		);

	#else
	pStrADMADAIBaseReg->DAMR = 0
		| Hw31 	//BCLK direct at master mode
		| Hw30 	//LRCLK direct at master mode	
#if defined(_WM8581_INCLUDE_)
		| Hw29	//DAI Buffer Threshold Enable
		| Hw28	//Multi-Port Enable
#endif
		| Hw21| Hw20 //DAI Rx shift Bit-pack LSB and 16bit mode
		| Hw19| Hw18 //DAI Tx shift Bit-pack LSB and 16bit mode
		| Hw15	//DAI Master enable
		| Hw11 	//DAI System Clock master select
		| Hw10 	//DAI Bit Clock master select
		| Hw9	//DAI Frame clock(LRCLK) master selcet 
		| Hw5	//DAI LRCLK(64fs->fs)
		;
	#endif



	
    return(0);
}

/*****************************************************************************
* Function Name : tca_i2s_initoutput()
******************************************************************************
* Desription    : i2s init output
* Parameter     : 
* Return        : success(SUCCESS) 
******************************************************************************/
void tca_i2s_initoutput(void *pADMABaseAddr, unsigned int nOutputDma, unsigned int AUDIO_DMA_PAGE_SIZE, unsigned int SAMPLERATE )
{
	ADMA *pStrADMABaseReg = (ADMA *)pADMABaseAddr;
	unsigned int bitSet=0x0;
	unsigned uSize;
    unsigned int DMABuffer = 0;

	//DMA Channel enable of DAI Tx
    pStrADMABaseReg->ChCtrl &= ~Hw28;

	DMABuffer = 0xFFFFFF00 / (((AUDIO_DMA_PAGE_SIZE*2) >> 4)<<8);
	DMABuffer = DMABuffer * (((AUDIO_DMA_PAGE_SIZE*2) >> 4)<<8);

	uSize = AUDIO_DMA_PAGE_SIZE*2;
		
	// DAI Tx (Right) Source Address
	pStrADMABaseReg->TxDaParam = DMABuffer | 4; //(4|0xFFFFFE00);	

	// ADMA Count Reginster setting
	pStrADMABaseReg->TxDaTCnt = (((AUDIO_DMA_PAGE_SIZE)>>0x2)>>0x2) - 1;
		
	// Tx Transfer Control Refister Setting	
	bitSet = ( HwZERO
		//Tx Trigger Type is edge-trigger
		|Hw28 //DMA transfer begins from current source/destination address
		//|Hw24 //Issue Locked Transfer of DAI Tx DMA
		|Hw16 //Enable repeat mode control of DAI Tx DMA
		|Hw9/*|Hw8*/ //Burst size of DAI Tx DMA is 4 cycle
		|Hw1|Hw0 //Word size of DAI Tx DMA is 32bit
		);
	
	pStrADMABaseReg->TransCtrl |= bitSet;
#if defined(_WM8581_INCLUDE_)		
	pStrADMABaseReg->RptCtrl = Hw26|Hw25|Hw24;//DAI Buffer Threshod is set Multi-Port
#else
    pStrADMABaseReg->RptCtrl = 0;
#endif

	/* ADMA Repeat register setting */
	//	BITCLR(HwRptCtrl,HwRptCtrl_DRI_EN);	//DMA Interrupt is occurred when the end of each Repeated DMAoperation.
	
	// Tx channel Control Register Setting	
	//Start DMA
	bitSet = ( HwZERO
		|Hw0 //Interrupt Enable of DAI Tx	
		);
  
	pStrADMABaseReg->ChCtrl |= bitSet;
}

/*****************************************************************************
* Function Name : tca_i2s_deinit()
******************************************************************************
* Desription    : i2s deinit
* Parameter     : 
* Return        : success(SUCCESS) 
******************************************************************************/

unsigned int tca_i2s_deinit(void *pADMADAIBaseAddr)
{
    //----- 1. Stop the I2S clocks -----
    tca_i2s_stop(pADMADAIBaseAddr, 0);
	tca_i2s_stop(pADMADAIBaseAddr, 1);

	return 0/*SUCCESS*/;
}


/*****************************************************************************
* Function Name : tca_i2s_start()
******************************************************************************
* Desription    :
* Parameter     : 
* Return        : success(SUCCESS) 
******************************************************************************/
void tca_i2s_start(void *pADMADAIBaseAddr, unsigned int nMode)
{
	ADMADAI *pStrADMADAIBaseReg = (ADMADAI *)pADMADAIBaseAddr;
	
	if(nMode) //input
	{
		//ADMADAI Receiver enable
		pStrADMADAIBaseReg->DAMR |= Hw13;
	}
	else
	{
		//ADMADAI Transmitter enable
		pStrADMADAIBaseReg->DAMR |= Hw14;
	}


}


/*****************************************************************************
* Function Name : tca_i2s_stop()
******************************************************************************
* Desription    :
* Parameter     : 
* Return        : success(SUCCESS) 
******************************************************************************/
void tca_i2s_stop(void *pADMADAIBaseAddr, unsigned int nMode)
{
	ADMADAI *pStrADMADAIBaseReg = (ADMADAI *)pADMADAIBaseAddr;
	
	if(nMode)
	{
		//DAI Receiver disable
		pStrADMADAIBaseReg->DAMR &= ~Hw13;
	}
	else
	{
		//DAI Transmitter disable
		pStrADMADAIBaseReg->DAMR &= ~Hw14;
	}

}

/*****************************************************************************
* Function Name : tca_i2s_initinput()
******************************************************************************
* Desription    :
* Parameter     : 
* Return        : success(SUCCESS) 
******************************************************************************/
void tca_i2s_initinput(void *pADMABaseAddr, unsigned int nInputDma, unsigned int AUDIO_DMA_IN_PAGE_SIZE, unsigned int SAMPLERATE)
{
	ADMA *pStrADMABaseReg = (ADMA *)pADMABaseAddr;	
	unsigned int bitSet=0x0;
	unsigned uSize;
	unsigned int DMABuffer = 0;

	//DMA Channel enable of DAI Rx
	pStrADMABaseReg->ChCtrl &= ~Hw30;//

	DMABuffer = 0xFFFFFF00 / (((AUDIO_DMA_IN_PAGE_SIZE*2) >> 4)<<8);
	DMABuffer = DMABuffer * (((AUDIO_DMA_IN_PAGE_SIZE*2) >> 4)<<8);

	uSize = AUDIO_DMA_IN_PAGE_SIZE*2;
		
	// DAI Rx (Right) Source Address
	pStrADMABaseReg->RxDaParam = DMABuffer | 4; //(4|0xFFFFFE00);	

	// ADMA Count Reginster setting
	pStrADMABaseReg->RxDaTCnt = (((AUDIO_DMA_IN_PAGE_SIZE)>>0x02)>>0x02) - 1;

	//Rx Transfer Control Refister Setting 
	bitSet = ( HwZERO
		//Rx Trigger Type is edge-trigger
		|Hw29 //DMA transfer begins from current source/destination address
		//|Hw26 //Issue Locked Transfer of DAI Rx DMA
		|Hw18 //Enable repeat mode control of DAI Rx DMA
		|Hw13 //Burst size of DAI Rx DMA is 4 cycle
		|(Hw5|Hw4) //Word size of DAI Rx DMA is 32bit
		);

	pStrADMABaseReg->TransCtrl |= bitSet;
#if defined(_WM8581_INCLUDE_)		
	pStrADMABaseReg->RptCtrl = Hw26|Hw25|Hw24;//DAI Buffer Threshod is set Multi-Port
#else
	pStrADMABaseReg->RptCtrl = 0;
#endif

	/* ADMA Repeat register setting */
	//BITCLR(HwRptCtrl,HwRptCtrl_DRI_EN); //DMA Interrupt is occurred when the end of each Repeated DMAoperation.
	
	// Rx channel Control Register Setting	
	//Start DMA
	bitSet = ( HwZERO
		|Hw2//Interrupt Enable of DAI Rx	
		);

	pStrADMABaseReg->ChCtrl |= bitSet; 
}



unsigned int tca_tcc_initport_wm8804(void)
{
#if 1  // JHLIM
//	GPIO *pStrGPIOBaseReg = (GPIO *)pGPIOBaseAddr;
//	GPIO *pStrGPIOBaseReg = (GPIO *)tcc_p2v(HwGPIO_BASE);
	#if 0
	//Set GPIO for Audio
	pStrGPIOBaseReg->GPDFN0 |= 0
		| Hw0	//BCLK(1)
		| Hw4	//LRCLK(1)
		| Hw8	//MCLK(1)
		| Hw12	//DAO0(1)
		| Hw16	//DAI0(1)
		| Hw20	//DAO1(1)
		| Hw24	//DAI1(1)
		| Hw28	//DAO2(1)
		;
	pStrGPIOBaseReg->GPDFN1 |= 0
		| Hw0	//DAI2(1)
		| Hw4	//DAO3(1)
		| Hw8	//DAI3(1)
#if defined(_SPDIF_COMMON_)
		| Hw12	//SPD_TX(1)
#endif
		;

#endif


#endif
	//WM8804 Reset(Low Active)
	
	gpio_request(GPIO_OPTIC_RST, "GPIO_OPTIC_RST");
	tcc_gpio_config(GPIO_OPTIC_RST,GPIO_FN(0));
	gpio_direction_output(GPIO_OPTIC_RST,1);
	gpio_free(GPIO_OPTIC_RST);


#ifdef AUDIO_POWER_CTL

#else
	ak_set_snd_pwr_ctl(AK_AUDIO_POWER,AK_PWR_ON);
#endif

	ak_set_audio_path(ADP_PLAYBACK);


return 0/*SUCCESS*/;
}

