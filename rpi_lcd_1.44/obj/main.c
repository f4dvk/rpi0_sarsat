#include <wiringPi.h>
#include <wiringPiSPI.h>

#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <time.h>

#include "LCD_Driver.h"
#include "LCD_GUI.h"
#include "LCD_BMP.h"
#include "DEV_Config.h"
#include "KEY_APP.h"

int main(void)
{
	//1.System Initialization
	if(DEV_ModuleInit())
		exit(0);

	//2.show
	//printf("**********Init LCD**********\r\n");
	LCD_SCAN_DIR LCD_ScanDir = SCAN_DIR_DFT;//SCAN_DIR_DFT = D2U_L2R
	LCD_Init(LCD_ScanDir );

	KEY_Listen();
	//3.System Exit
	DEV_ModuleExit();
	return 0;

}
