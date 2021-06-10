#include "define_all.h"
#include "user_init.h"
#include "UserApp\app_comm.h"


BOOL T_20ms = FALSE;	//20ms时间到置位，主循环清零
INT8U FrameStartCnt = 0;
extern INT8U FrameStartFlag;

void timer_20ms_hook(void)
{
	T_20ms = TRUE;
	//LCDDisplay();/////不能在这里调用！！！会影响低功耗通信
	IWDT_Clr();
	RunBlkTime();
	IntBeepTimeJudge();
	RunDispCycTime();
	STSInputOverTimeJudge();
	ProgressDisplayError();
	IWDT_Clr();

	if(SystemFlag.bits.keyTest == FALSE)
	{
		ararm_redgreen_dispose();//新加
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
	Clock_Init(RUN_MODE);				//先以低功耗模式初始化时钟及看门狗
	GPIO_Init_All();					//初始化所有用到的IO口
	//if(FALSE==ReadLowPowerFlag()) 		//正常供电模式
	if(1)
	{
		sUartUIUPro.psUartUIUInfo->low_power_mode = FALSE;
		McuSleepMode = FALSE;
		//Clock_Init(LP_MODE);
		KEY_Init(RUN_MODE); 			//按键初始化
		beep_init(4000, 50);			//蜂鸣器初始化，4KHz, 50%占空比
	}
	else								//低功耗模式
	{
		sUartUIUPro.psUartUIUInfo->low_power_mode = TRUE;
		KEY_Init(LP_MODE);				//按键初始化
	}
	LCD_Init(); 						//LCD初始化
	LCD_All_On();						//全屏显示
	//ADC_Init(); 						//ADC初始化
	
	Uart_Init(UART_PLC, 9600, NONE);	//PLC模块串口初始化（未打开串口）
	
    Uart_Init(UART_DEBUG, 9600, NONE);
    Uart_Init(UART_USB, 9600, NONE);

	//Uart_Init(UART_DEBUG, 9600, NONE);	//设参串口初始化（未打开串口）
	timer_init(timer_20ms_hook);		//初始化20ms定时器（未开启）
	IWDT_Clr();  						//清系统看门狗
}

/*加载保存在flash中的参数，若读出的参数全为0或ff，则使用默认参数，并将默认参数写入flash*/
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
	if(i<sizeof(uiuaddrtmp)-1)	//不全为0
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
		if(i==sizeof(uiuaddrtmp)-1)	//全为0xff
		{
			err = 1;
		}
	}
	else	//全为0
	{
		err = 1;
	}

	if(err == 0)
	{
		sumtmp1 = SUM_DATA + check_sum_dispose((INT8U*)&uiuaddrtmp.Buff,6);
		sumtmp2 = SUM_DATA + check_sum_dispose((INT8U*)&SysModetmp.Buff,2);
		if((sumtmp1!=uiuaddrtmp.Buff[6])||(sumtmp2!=SysModetmp.Buff[2]))//校验不对
		{
			err = 1;
		}
	}

	if(err == 1)//读出来的参数不正确
	{
		Flsah_Write_String(FLASH_UIU_ADDR_ADDR, (INT8U*)&UiuAddrDefault, sizeof(UiuAddrDefault));
		IWDT_Clr();  				//清系统看门狗
		Flsah_Write_String(FLASH_SYSMODE_ADDR, (INT8U*)&SysModeDefault, sizeof(SysModeDefault));
		IWDT_Clr();  				//清系统看门狗
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
	//加载flash参数
	LoadFlashParameter();
	IWDT_Clr();  				//清系统看门狗
	//初始化各种全局变量
	_MemSet(0, (INT8U*)&Info, sizeof(Info));
	Info.CurrentTariff = 0xff;//刚开始开始不让显示费率
	SystemFlag.bits.plcVerGet = FALSE;
	_MemSet(0, (INT8U*)&LowPowerSaveVal, sizeof(LowPowerSaveVal));
	_MemSet(0, (INT8U*)&Energy, sizeof(Energy));
	_MemSet(0, (INT8U*)&Tariff, sizeof(Tariff));
	IWDT_Clr();  				//清系统看门狗
	//
	SetupBeep();
	SetupToken();
	SetupKeyPad();
	PulsePInit();
	uart_UIUPro_init();
	plc_comm_sleep();
	Cycle_Read_Init();
	StopDispCyc();
	SetupDispCyc(13);	//默认每屏显示13*20*20=5.2S
	if(sUartUIUPro.psUartUIUInfo->low_power_mode == FALSE)
	{
		SetupBlk(10);	//默认液晶背光持续时间=10*50*20ms
	}
	StartDispCyc();
	sUartUIUPro.sUartUIUFunc.DisplayFun = PowerOnDisplay;
	sUartUIUPro.psUartUIUInfo->DisplayItem = 0x00;
	//Cycle_Read_Init();
}

void SystemStart(void)
{

//	if(sUartUIUPro.psUartUIUInfo->low_power_mode == FALSE)
	
		Uart_Open(UART_PLC);			//打开模块串口
		Uart_Open(UART_DEBUG);
		Uart_Open(UART_USB);
		PLC_POWER_ON;
		sUART_UIU_COMMINFO.comm_modle_power_off = FALSE;
	

	//Uart_Open(UART_DEBUG);				//打开设参串口


	Timer20msStart();					//开启20ms定时器
}


#if 0
void Init_System(void)
{
	/*基础系统配置*/
	DI();			//关闭全局中断使能
	IWDT_Init(IWDT_IWDTCFG_IWDTOVP_2s);				//系统看门狗设置
	IWDT_Clr();  				//清系统看门狗
	Init_SysTick();				//cpu滴答定时器配置(软件延时用)
	TicksDelayMs( 10, NULL );	//软件延时,系统上电后不要立刻将时钟切换为非RCHF8M，也不要立刻进休眠否则可能导致无法下载程序

	Init_SysClk_Gen(RUN_MODE);			//系统时钟配置
	RCC_Init_RCHF_Trim(clkmode);//RCHF振荡器校准值载入(芯片复位后自动载入8M的校准值)，只是调校RCHF常温精度

	/*外设初始化配置*/
	//Init_Pad_Io();				//IO口输出寄存器初始状态配置

	Close_None_GPIO_80pin();    //关闭80脚芯片不支持的IO
	Close_AllIO_GPIO_80pin();   //关闭全部IO


	/*RTC数值调整寄存器*/
	RTC_ADJUST_Write(0);//RTC时钟温度补偿值写0，假如不操作调整寄存器，补偿值会是一个随机数，RTC时钟可能会偏差非常大

	/*准备进入主循环*/
	TicksDelayMs( 100, NULL );	//软件延时

	EI();				//打开全局中断使能
}
#endif
