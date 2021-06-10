#include "define_all.h"
#include "user_init.h"
#include "UserApp\app_comm.h"


BOOL T_20ms = FALSE;	//20msʱ�䵽��λ����ѭ������
INT8U FrameStartCnt = 0;
extern INT8U FrameStartFlag;

void timer_20ms_hook(void)
{
	T_20ms = TRUE;
	//LCDDisplay();/////������������ã�������Ӱ��͹���ͨ��
	IWDT_Clr();
	RunBlkTime();
	IntBeepTimeJudge();
	RunDispCycTime();
	STSInputOverTimeJudge();
	ProgressDisplayError();
	IWDT_Clr();

	if(SystemFlag.bits.keyTest == FALSE)
	{
		ararm_redgreen_dispose();//�¼�
		IntPulseLedTimer();
		AlarmLedControl();
	}
	if(sUartUIUPro.psUartUIUInfo->low_power_mode == FALSE)
	{
		Cycle_Read_Enable();
	}
	UartTxRxTickCheck();
	if(FrameStartFlag==1)
	{
      FrameStartCnt+=1;
	}
}

void HardWareInit(void)
{
	Clock_Init(RUN_MODE);				//���Ե͹���ģʽ��ʼ��ʱ�Ӽ����Ź�
	GPIO_Init_All();					//��ʼ�������õ���IO��
	//if(FALSE==ReadLowPowerFlag()) 		//��������ģʽ
	if(1)
	{
		sUartUIUPro.psUartUIUInfo->low_power_mode = FALSE;
		McuSleepMode = FALSE;
		//Clock_Init(LP_MODE);
		KEY_Init(RUN_MODE); 			//������ʼ��
		beep_init(4000, 50);			//��������ʼ����4KHz, 50%ռ�ձ�
	}
	else								//�͹���ģʽ
	{
		sUartUIUPro.psUartUIUInfo->low_power_mode = TRUE;
		KEY_Init(LP_MODE);				//������ʼ��
	}
	LCD_Init(); 						//LCD��ʼ��
	LCD_All_On();						//ȫ����ʾ
	//ADC_Init(); 						//ADC��ʼ��
	
	Uart_Init(UART_PLC, 9600, NONE);	//PLCģ�鴮�ڳ�ʼ����δ�򿪴��ڣ�
	
    Uart_Init(UART_DEBUG, 9600, NONE);
    Uart_Init(UART_USB, 9600, NONE);

	//Uart_Init(UART_DEBUG, 9600, NONE);	//��δ��ڳ�ʼ����δ�򿪴��ڣ�
	timer_init(timer_20ms_hook);		//��ʼ��20ms��ʱ����δ������
	IWDT_Clr();  						//��ϵͳ���Ź�
}

/*���ر�����flash�еĲ������������Ĳ���ȫΪ0��ff����ʹ��Ĭ�ϲ���������Ĭ�ϲ���д��flash*/
void LoadFlashParameter(void)
{
	INT8U i,err=0,sumtmp1=0,sumtmp2=0;
	INT8U* p = NULL;
	UiuAddrPara uiuaddrtmp;
	SystemModePara SysModetmp;

	_MemSet(0, (INT8U*)&uiuaddrtmp, sizeof(uiuaddrtmp));
	_MemSet(0, (INT8U*)&SysModetmp, sizeof(SysModetmp));
	Flsah_Read_String(FLASH_UIU_ADDR_ADDR, (INT8U*)&uiuaddrtmp, sizeof(uiuaddrtmp));
	IWDT_Clr();
	Flsah_Read_String(FLASH_SYSMODE_ADDR, (INT8U*)&SysModetmp, sizeof(SysModetmp));
	for(i=0; i<sizeof(uiuaddrtmp); i++)
	{
		if(i==0)
		{
			p=(INT8U*)&uiuaddrtmp;
		}
		if(*p == 0)
		{
			p++;
		}
		else
		{
			break;
		}
	}
	if(i<sizeof(uiuaddrtmp)-1)	//��ȫΪ0
	{
		for(i=0; i<sizeof(uiuaddrtmp); i++)
		{
			if(i==0)
			{
				p=(INT8U*)&uiuaddrtmp;
			}

			if(*p == 0xff)
			{
				p++;
			}
			else
			{
				break;
			}
		}
		if(i==sizeof(uiuaddrtmp)-1)	//ȫΪ0xff
		{
			err = 1;
		}
	}
	else	//ȫΪ0
	{
		err = 1;
	}

	if(err == 0)
	{
		sumtmp1 = SUM_DATA + check_sum_dispose((INT8U*)&uiuaddrtmp.Buff,6);
		sumtmp2 = SUM_DATA + check_sum_dispose((INT8U*)&SysModetmp.Buff,2);
		if((sumtmp1!=uiuaddrtmp.Buff[6])||(sumtmp2!=SysModetmp.Buff[2]))//У�鲻��
		{
			err = 1;
		}
	}

	if(err == 1)//�������Ĳ�������ȷ
	{
		Flsah_Write_String(FLASH_UIU_ADDR_ADDR, (INT8U*)&UiuAddrDefault, sizeof(UiuAddrDefault));
		IWDT_Clr();  				//��ϵͳ���Ź�
		Flsah_Write_String(FLASH_SYSMODE_ADDR, (INT8U*)&SysModeDefault, sizeof(SysModeDefault));
		IWDT_Clr();  				//��ϵͳ���Ź�
		UiuAddr = UiuAddrDefault;
		SysModeConfig = SysModeDefault;
	}
	else
	{
		UiuAddr = uiuaddrtmp;
		SysModeConfig = SysModetmp;
	}
	if((SysModeConfig.Buff[0] &SET_0645PRO) == SET_0645PRO)
	{
       SystemFlag.bits.protoclOnly645 = TRUE;
	}
}

void SoftwareInit(void)
{
	//����flash����
	LoadFlashParameter();
	IWDT_Clr();  				//��ϵͳ���Ź�
	//��ʼ������ȫ�ֱ���
	_MemSet(0, (INT8U*)&Info, sizeof(Info));
	Info.CurrentTariff = 0xff;//�տ�ʼ��ʼ������ʾ����
	SystemFlag.bits.plcVerGet = FALSE;
	_MemSet(0, (INT8U*)&LowPowerSaveVal, sizeof(LowPowerSaveVal));
	_MemSet(0, (INT8U*)&Energy, sizeof(Energy));
	_MemSet(0, (INT8U*)&Tariff, sizeof(Tariff));
	IWDT_Clr();  				//��ϵͳ���Ź�
	//
	SetupBeep();
	SetupToken();
	SetupKeyPad();
	PulsePInit();
	uart_UIUPro_init();
	plc_comm_sleep();
	Cycle_Read_Init();
	StopDispCyc();
	SetupDispCyc(13);	//Ĭ��ÿ����ʾ13*20*20=5.2S
	if(sUartUIUPro.psUartUIUInfo->low_power_mode == FALSE)
	{
		SetupBlk(10);	//Ĭ��Һ���������ʱ��=10*50*20ms
	}
	StartDispCyc();
	sUartUIUPro.sUartUIUFunc.DisplayFun = PowerOnDisplay;
	sUartUIUPro.psUartUIUInfo->DisplayItem = 0x00;
	//Cycle_Read_Init();
}

void SystemStart(void)
{

//	if(sUartUIUPro.psUartUIUInfo->low_power_mode == FALSE)
	
		Uart_Open(UART_PLC);			//��ģ�鴮��
		Uart_Open(UART_DEBUG);
		Uart_Open(UART_USB);
		PLC_POWER_ON;
		sUART_UIU_COMMINFO.comm_modle_power_off = FALSE;
	

	//Uart_Open(UART_DEBUG);				//����δ���


	Timer20msStart();					//����20ms��ʱ��
}


#if 0
void Init_System(void)
{
	/*����ϵͳ����*/
	DI();			//�ر�ȫ���ж�ʹ��
	IWDT_Init(IWDT_IWDTCFG_IWDTOVP_2s);				//ϵͳ���Ź�����
	IWDT_Clr();  				//��ϵͳ���Ź�
	Init_SysTick();				//cpu�δ�ʱ������(�����ʱ��)
	TicksDelayMs( 10, NULL );	//�����ʱ,ϵͳ�ϵ��Ҫ���̽�ʱ���л�Ϊ��RCHF8M��Ҳ��Ҫ���̽����߷�����ܵ����޷����س���

	Init_SysClk_Gen(RUN_MODE);			//ϵͳʱ������
	RCC_Init_RCHF_Trim(clkmode);//RCHF����У׼ֵ����(оƬ��λ���Զ�����8M��У׼ֵ)��ֻ�ǵ�УRCHF���¾���

	/*�����ʼ������*/
	//Init_Pad_Io();				//IO������Ĵ�����ʼ״̬����

	Close_None_GPIO_80pin();    //�ر�80��оƬ��֧�ֵ�IO
	Close_AllIO_GPIO_80pin();   //�ر�ȫ��IO


	/*RTC��ֵ�����Ĵ���*/
	RTC_ADJUST_Write(0);//RTCʱ���¶Ȳ���ֵд0�����粻���������Ĵ���������ֵ����һ���������RTCʱ�ӿ��ܻ�ƫ��ǳ���

	/*׼��������ѭ��*/
	TicksDelayMs( 100, NULL );	//�����ʱ

	EI();				//��ȫ���ж�ʹ��
}
#endif
