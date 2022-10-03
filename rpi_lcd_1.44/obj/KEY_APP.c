/*****************************************************************************
* |	This version:   V1.0
* | Date        :   2019-07-06
* | Info        :   Basic version
*
******************************************************************************/
#define PATH_SARSAT_CONFIG "/home/pi/rpi0_sarsat/rpi_lcd_1.44/config.txt"
#define PATH_DATV_CONFIG "/home/pi/rpi0_sarsat/rpi_lcd_1.44/config_datv.txt"

#include "KEY_APP.h"
#include "LCD_GUI.h"
#include "LCD_Driver.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
// #include "Debug.h"

// if use 2019-06-20-raspbian-buster
// sudo nano /boot/config.txt
// add:
// gpio=6,19,5,26,13,21,20,16=pu

pthread_t thbutton;
int FinishedButton = 0;
int Menu = 0; // 0 = Sarsat decoder ; 1 = DATV

int nb[6];
int i=1;
int j=0;
int n=0;
int m=0;
char freq_config[31];
char mode[10];
int Fec_Dvbs[5]={1, 2, 3, 5, 7};
char fec[10];
char sr[10];
char sr_list[5][10]={"92", "125", "250", "333", "500"};
char fec_char[6][10]={"1/2", "2/3", "3/4", "5/6", "7/8", "Auto"};


char value[100];
char Symbol[]={'_',' '};
char Selected[]={' ',' ',' ',' ',' ',' '};

void menu(void);

void GetConfigParam(char *PathConfigFile, char *Param, char *Value)
{
  char * line = NULL;
  size_t len = 0;
  int read;
  char ParamWithEquals[255];
  strcpy(ParamWithEquals, Param);
  strcat(ParamWithEquals, "=");

  //printf("Get Config reads %s for %s ", PathConfigFile , Param);

  FILE *fp=fopen(PathConfigFile, "r");
  if(fp != 0)
  {
    while ((read = getline(&line, &len, fp)) != -1)
    {
      if(strncmp (line, ParamWithEquals, strlen(Param) + 1) == 0)
      {
        strcpy(Value, line+strlen(Param)+1);
        char *p;
        if((p=strchr(Value,'\n')) !=0 ) *p=0; //Remove \n
        break;
      }
    }
  }
  else
  {
    printf("Config file not found \n");
  }
  fclose(fp);
}

void SetConfigParam(char *PathConfigFile, char *Param, char *Value)
{
  char * line = NULL;
  size_t len = 0;
  int read;
  char Command[511];
  char BackupConfigName[240];
  strcpy(BackupConfigName,PathConfigFile);
  strcat(BackupConfigName,".bak");
  FILE *fp=fopen(PathConfigFile,"r");
  FILE *fw=fopen(BackupConfigName,"w+");
  char ParamWithEquals[255];
  strcpy(ParamWithEquals, Param);
  strcat(ParamWithEquals, "=");

  if(fp!=0)
  {
    while ((read = getline(&line, &len, fp)) != -1)
    {
      if(strncmp (line, ParamWithEquals, strlen(Param) + 1) == 0)
      {
        fprintf(fw, "%s=%s\n", Param, Value);
      }
      else
      {
        fprintf(fw,line);
      }
    }
    fclose(fp);
    fclose(fw);
    snprintf(Command, 511, "cp %s %s", BackupConfigName, PathConfigFile);
    system(Command);
  }
  else
  {
    printf("Config file not found \n");
    fclose(fp);
    fclose(fw);
  }
}

/// Source BATC: ///

void chopN(char *str, size_t n)
{
  size_t len = strlen(str);
  if ((n > len) || (n == 0))
  {
    return;
  }
  else
  {
    memmove(str, str+n, len - n + 1);
  }
}

int CheckRTL()
{
  char RTLStatus[256];
  FILE *fp;
  int rtlstat = 1;

  /* Open the command for reading. */
  fp = popen("/home/pi/rpi0_sarsat/rpi_lcd_1.44/check_rtl.sh", "r");
  if (fp == NULL)
  {
    printf("Failed to run command\n" );
    exit(1);
  }
  /* Read the output a line at a time - output it. */
  while (fgets(RTLStatus, sizeof(RTLStatus)-1, fp) != NULL)
  {
   if (RTLStatus[0] == '0')
   {
     printf("RTL Detected\n" );
     rtlstat = 0;
   }
   else
   {
     printf("No RTL Detected\n" );
   }
  }
  pclose(fp);
  return(rtlstat);
}

void not_detected(void)
{
  LCD_Clear(WHITE);

  GUI_DisString_EN(3, 16, "  RTLSDR", &Font16, GUI_BACKGROUND, RED);
  GUI_DisString_EN(3, 30, "   NON", &Font16, GUI_BACKGROUND, RED);
  GUI_DisString_EN(3, 44, " DETECTEE!", &Font16, GUI_BACKGROUND, RED);
  GUI_DisString_EN(3, 61, "        Retour>", &Font12, GUI_BACKGROUND, BLACK);

  while(GET_KEY2 == 1)
  {
    usleep(100000);
  }

  menu();
}

void Draw_Init(void)
{
  Menu = 0;
  if (i == 0)
  {
    Selected[i]=Symbol[1];
    i = 1;
  }

  LCD_Clear(WHITE);

  GUI_DisString_EN(3, 2, " Sarsat Decoder", &Font12, GUI_BACKGROUND, BLACK);

  GetConfigParam(PATH_SARSAT_CONFIG, "freq", freq_config);

  int freq=atoi(freq_config);

  for (int f = 5; f >= 0; f--) {
    nb[f] = freq % 10;
    freq /= 10;
  }

  sprintf(value, "  %d%d%d.%d%d%d Mhz", nb[0], nb[1], nb[2], nb[3], nb[4], nb[5]);

  GUI_DisString_EN(3, 15, value, &Font12, GUI_BACKGROUND, BLACK);

  Selected[i]=Symbol[0];

  sprintf(value, "   %c%c %c%c%c", Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);

  GUI_DisString_EN(3, 16, value, &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 32, "    Start/Stop>", &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 61, "          DATV>", &Font12, GUI_BACKGROUND, BLACK);

  /*GUI_DisString_EN(3, 24, "line 3 test lon", &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 34, "123456789012345", &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 44, "12345678901234567", &Font8, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 54, "line 6", &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 64, "line 7", &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 74, "line 8", &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 84, "line 9", &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 94, "line 10", &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 104, "line 11", &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 114, "line 12", &Font12, GUI_BACKGROUND, BLACK);*/

  /* Press */

  /*GUI_DrawRectangle(40, 60, 60, 80, RED, DRAW_EMPTY, DOT_PIXEL_DFT);

  GUI_DisString_EN(43, 60, "C", &Font24, GUI_BACKGROUND, BLUE);*/

  /* Left */

  /*GUI_DrawRectangle(20, 60, 40, 80, RED, DRAW_EMPTY, DOT_PIXEL_DFT);

  GUI_DisString_EN(23, 60, "G", &Font24, GUI_BACKGROUND, BLUE);*/

  /* Right */

  /*GUI_DrawRectangle(60, 60, 80, 80, RED, DRAW_EMPTY, DOT_PIXEL_DFT);

  GUI_DisString_EN(63, 60, "D", &Font24, GUI_BACKGROUND, BLUE);*/

  /* Up */

  /*GUI_DrawRectangle(40, 40, 60, 60, RED, DRAW_EMPTY, DOT_PIXEL_DFT);

  GUI_DisString_EN(43, 40, "H", &Font24, GUI_BACKGROUND, BLUE);*/

  /* Down */

  /*GUI_DrawRectangle(40, 80, 60, 100, RED, DRAW_EMPTY, DOT_PIXEL_DFT);

  GUI_DisString_EN(43, 80, "B", &Font24, GUI_BACKGROUND, BLUE);*/

  /* Key1 */

  /*GUI_DrawRectangle(95, 40, 120, 60, RED, DRAW_EMPTY, DOT_PIXEL_DFT);

  GUI_DisString_EN(98, 43, "K1", &Font16, GUI_BACKGROUND, BLUE);*/

  /* Key2	*/

  /*GUI_DrawRectangle(95, 60, 120, 80, RED, DRAW_EMPTY, DOT_PIXEL_DFT);

  GUI_DisString_EN(98, 63, "K2", &Font16, GUI_BACKGROUND, BLUE);*/

  /* Key3 */

  /*GUI_DrawRectangle(95, 80, 120, 100, RED, DRAW_EMPTY, DOT_PIXEL_DFT);

  GUI_DisString_EN(98, 83, "K3", &Font16, GUI_BACKGROUND, BLUE);*/

}

void *WaitButtonEvent(void * arg)
{
  while(GET_KEY1 == 1) usleep(100000);
  FinishedButton=1;
  return NULL;
}

void start_datv_menu(void)
{
  Menu = 1;

  LCD_Clear(WHITE);

  GetConfigParam(PATH_DATV_CONFIG, "freq", freq_config);
  GetConfigParam(PATH_DATV_CONFIG, "mode", mode);
  GetConfigParam(PATH_DATV_CONFIG, "fec", fec);
  GetConfigParam(PATH_DATV_CONFIG, "sr", sr);

  if (strcmp (fec, "Auto") != 0)
  {
    j = atoi(fec)-1;
    if (j == 4) j=3;
    if (j == 6) j=4;
  }
  else
  {
    j=5;
  }

  for (n = 0; n < 5; n = n + 1)
  {
    if (strcmp (sr_list[n], sr) == 0) m = n;
  }

  int freq=atoi(freq_config);

  for (int f = 5; f >= 0; f--) {
    nb[f] = freq % 10;
    freq /= 10;
  }

  sprintf(value, "  %d%d%d.%d%d%d Mhz", nb[0], nb[1], nb[2], nb[3], nb[4], nb[5]);

  GUI_DisString_EN(3, 15, value, &Font12, GUI_BACKGROUND, BLACK);

  Selected[i]=Symbol[0];

  sprintf(value, "  %c%c%c %c%c%c", Selected[0],Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);

  GUI_DisString_EN(3, 16, value, &Font12, GUI_BACKGROUND, BLACK);

  GUI_DisString_EN(3, 2, " DATV Receiver", &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 32, "    Start/Stop>", &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 61, "        Sarsat>", &Font12, GUI_BACKGROUND, BLACK);
  GUI_DisString_EN(3, 90, "          Mode>", &Font12, GUI_BACKGROUND, BLACK);
  sprintf(value, "SR: %s K/s", sr_list[m]);
  GUI_DisString_EN(3, 103, value, &Font12, GUI_BACKGROUND, BLACK);
  sprintf(value, "%s FEC:%s", mode, fec_char[j]);
  GUI_DisString_EN(3, 116, value, &Font12, GUI_BACKGROUND, BLACK);
}

void start_sarsat(void)
{
  #define PATH_SCRIPT_DECODER "/home/pi/rpi0_sarsat/406/scan.sh 3>&1 2>&1"

  //Local parameters:

  FILE *fp;
  char *line=NULL;
  size_t len = 0;
  ssize_t read;

  FinishedButton = 0;

  // Affichage
  char old[30]=" ";
  char line1[30]="";
  char line2[30]="";
  char line3[30]="";
  char line4[30]="";
  char line5[30]="";
  char line6[30]="";
  char line7[30]="";
  char line8[30]="";
  char line9[30]="";
  char line10[30]="";
  char line11[30]="";
  char end[30]="";
  char rssi[30]="";

  //char crc1[14]="";
  //char crc2[14]="";

  int nbline=1;

  char strTag[30]=" ";

  pthread_create (&thbutton, NULL, &WaitButtonEvent, NULL);

  fp=popen(PATH_SCRIPT_DECODER, "r");

  while (((read = getline(&line, &len, fp)) != -1) && (FinishedButton == 0))
  {

    sscanf(line,"%s ",strTag);

   int length=strlen(line);
   if (line[length-1] == '\n') line[length-1] = '\0';

   //printf("%s\n",strTag);

    if (((strcmp(strTag, "RSSI")==0) || (strcmp(strTag, "Lancement")==0) || (strcmp(strTag, "ATTENTE")==0) || (strcmp(strTag, "CRC1")==0) || (strcmp(strTag, "CRC2")==0) || (strcmp(strTag, "Contenu")==0)) && (strcmp(strTag, old)!=0))
    {
       //printf("%s\n",strTag);
       //LCD_Clear(WHITE);
       nbline=1;
       strcpy(old, strTag);
       if (strcmp(strTag, "CRC1")==0)
       {
         //strcpy(crc1, line);
         //strcpy(crc2,"");
         GUI_DisString_EN(3, 120, end, &Font8, GUI_BACKGROUND, WHITE);
         strcpy(end,"");
         strcpy(end, line);
         strcat(end," ");
       }
       else if (strcmp(strTag, "CRC2")==0)
       {
         //strcpy(crc2, line);
         GUI_DisString_EN(3, 120, end, &Font8, GUI_BACKGROUND, WHITE);
         strcat(end, line);
       }
       else if (strcmp(strTag, "RSSI")==0)
       {
         GUI_DisString_EN(70, 120, rssi, &Font8, GUI_BACKGROUND, WHITE);
         chopN(line, 5);
         strcpy(rssi,"");
         strcpy(rssi,"rssi: ");
         strcat(rssi,line);
         fprintf(stderr, "%s\n", rssi);
       }
       else if (strcmp(strTag, "Contenu")!=0)
       {
         GUI_DisString_EN(3, 2, line1, &Font8, GUI_BACKGROUND, WHITE);
         strcpy(line1, line);
       }

       if (strcmp(strTag, "Contenu")==0)
         {
           LCD_Clear(WHITE);
           //nbline=2;
           strcpy(line2, "");
           //strcpy(line2, line);
           strcpy(line3, "");
           strcpy(line4, "");
           strcpy(line5, "");
           strcpy(line6, "");
           strcpy(line7, "");
           strcpy(line8, "");
           strcpy(line9, "");
           strcpy(line10, "");
           strcpy(line11, "");
         }
    }else if ((nbline==1) && (strcmp(strTag, old)!=0)){
       nbline++;
       strcpy(line2, line);
    }else if (nbline==2){
       nbline++;
       strcpy(line3, line);
    }else if (nbline==3){
       nbline++;
       strcpy(line4, line);
    }else if (nbline==4){
       nbline++;
       strcpy(line5, line);
    }else if (nbline==5){
       nbline++;
       strcpy(line6, line);
    }else if (nbline==6){
       nbline++;
       strcpy(line7, line);
    }else if (nbline==7){
       nbline++;
       strcpy(line8, line);
    }else if (nbline==8){
       nbline++;
       strcpy(line9, line);
    }else if (nbline==9){
       nbline++;
       strcpy(line10, line);
    }else if (nbline==10){
       nbline++;
       strcpy(line11, line);
    }else if (nbline==11){
    }

    //GUI_DisString_EN(3, 112, end, &Font12, GUI_BACKGROUND, WHITE);
    //strcpy(end,crc1);
    //strcat(end," ");
    //strcat(end,crc2);

    GUI_DisString_EN(3, 2, line1, &Font8, GUI_BACKGROUND, BLACK);
    GUI_DisString_EN(3, 10, line2, &Font12, GUI_BACKGROUND, BLACK);
    GUI_DisString_EN(3, 21, line3, &Font12, GUI_BACKGROUND, BLACK);
    GUI_DisString_EN(3, 32, line4, &Font12, GUI_BACKGROUND, BLACK);
    GUI_DisString_EN(3, 43, line5, &Font12, GUI_BACKGROUND, BLACK);
    GUI_DisString_EN(3, 54, line6, &Font12, GUI_BACKGROUND, BLACK);
    GUI_DisString_EN(3, 65, line7, &Font12, GUI_BACKGROUND, BLACK);
    GUI_DisString_EN(3, 76, line8, &Font12, GUI_BACKGROUND, BLACK);
    GUI_DisString_EN(3, 87, line9, &Font12, GUI_BACKGROUND, BLACK);
    GUI_DisString_EN(3, 98, line10, &Font12, GUI_BACKGROUND, BLACK);
    GUI_DisString_EN(3, 109, line11, &Font12, GUI_BACKGROUND, BLACK);
    GUI_DisString_EN(3, 120, end, &Font8, GUI_BACKGROUND, BLACK);
    GUI_DisString_EN(70, 120, rssi, &Font8, GUI_BACKGROUND, BLACK);
    if (strcmp(end, "CRC1 OK CRC2 OK")==0){
      //LCD_SetArealColor(118, 118, 128, 128, GREEN);
      LCD_SetArealColor(118, 0, 128, 10, GREEN);
    }
    else{
      //LCD_SetArealColor(118, 118, 128, 128, RED);
      LCD_SetArealColor(118, 0, 128, 10, RED);
    }
  }
  system("/home/pi/rpi0_sarsat/406/stop.sh >/dev/null 2>/dev/null");
  usleep(1000);
  pclose(fp);

  system("sudo killall scan.sh >/dev/null 2>/dev/null");
  system("pkill -9 rtl_fm && pkill perl && pkill -9 perl >/dev/null 2>/dev/null");
  pthread_join(thbutton, NULL);
  Draw_Init();
}

void start_datv(void)
{
  #define PATH_SCRIPT_DATV "/home/pi/rpi0_sarsat/rpi_lcd_1.44/datv_rx.sh 2>/dev/null"

  FILE *fp;
  FinishedButton = 0;

  pthread_create (&thbutton, NULL, &WaitButtonEvent, NULL);

  fp=popen(PATH_SCRIPT_DATV, "r");

  while (FinishedButton == 0)
  {
    sleep(1);
  }

  pclose(fp);
  pthread_join(thbutton, NULL);

  system("sudo killall leandvb >/dev/null 2>/dev/null");
  system("sudo killall vlc >/dev/null 2>/dev/null");
  system("sudo killall netcat >/dev/null 2>/dev/null");
  system("sudo killall fbcp");
  sleep(1);
  system("(sudo killall -9 leandvb >/dev/null 2>/dev/null) &");
  system("(sudo killall -9 rtl_sdr >/dev/null 2>/dev/null) &");
  system("sudo pkill -9 fbcp >/dev/null 2>/dev/null");
  sleep(0.5);

  if(DEV_ModuleInit())
    exit(0);

  LCD_SCAN_DIR LCD_ScanDir = SCAN_DIR_DFT;
  LCD_Init(LCD_ScanDir );

  start_datv_menu();

}

void menu(void)
{
  if (Menu == 0)
  {
    Draw_Init();
  }
  else if (Menu == 1)
  {
    start_datv_menu();
  }
}

void KEY_Listen(void)
{
    menu();
    for(;;) {
        usleep(10000);
        if(GET_KEY_UP == 0) {
            sprintf(value, "  %d%d%d.%d%d%d Mhz", nb[0], nb[1], nb[2], nb[3], nb[4], nb[5]);
            GUI_DisString_EN(3, 15, value, &Font12, GUI_BACKGROUND, WHITE);
            nb[i] ++;
            if (nb[i] > 9) nb[i]=0;
            sprintf(value, "  %d%d%d.%d%d%d Mhz", nb[0], nb[1], nb[2], nb[3], nb[4], nb[5]);
            GUI_DisString_EN(3, 15, value, &Font12, GUI_BACKGROUND, BLACK);
            sprintf(freq_config, "%d%d%d%d%d%d", nb[0], nb[1], nb[2], nb[3], nb[4], nb[5]);
            if (Menu == 0)
            {
              SetConfigParam(PATH_SARSAT_CONFIG, "freq", freq_config);
            }
            else
            {
              SetConfigParam(PATH_DATV_CONFIG, "freq", freq_config);
            }
            while(GET_KEY_UP == 0) {
                usleep(100000);
            }
            /*GUI_DrawRectangle(40, 40, 60, 60, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(40, 40, 60, 60, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(43, 40, "H", &Font24, GUI_BACKGROUND, BLUE);*/
        }
        if(GET_KEY_DOWN == 0) {
            sprintf(value, "  %d%d%d.%d%d%d Mhz", nb[0], nb[1], nb[2], nb[3], nb[4], nb[5]);
            GUI_DisString_EN(3, 15, value, &Font12, GUI_BACKGROUND, WHITE);
            nb[i] --;
            if (nb[i] < 0) nb[i]=9;
            sprintf(value, "  %d%d%d.%d%d%d Mhz", nb[0], nb[1], nb[2], nb[3], nb[4], nb[5]);
            GUI_DisString_EN(3, 15, value, &Font12, GUI_BACKGROUND, BLACK);
            sprintf(freq_config, "%d%d%d%d%d%d", nb[0], nb[1], nb[2], nb[3], nb[4], nb[5]);
            if (Menu == 0)
            {
              SetConfigParam(PATH_SARSAT_CONFIG, "freq", freq_config);
            }
            else
            {
              SetConfigParam(PATH_DATV_CONFIG, "freq", freq_config);
            }
            while(GET_KEY_DOWN == 0) {
                usleep(100000);
            }
            /*GUI_DrawRectangle(40, 80, 60, 100, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(40, 80, 60, 100, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(43, 80, "B", &Font24, GUI_BACKGROUND, BLUE);*/
        }
        if(GET_KEY_LEFT == 0) {
            if (Menu == 0)
            {
              sprintf(value, "   %c%c %c%c%c", Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);
            }
            else
            {
              sprintf(value, "  %c%c%c %c%c%c", Selected[0], Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);
            }
            GUI_DisString_EN(3, 16, value, &Font12, GUI_BACKGROUND, WHITE);
            Selected[i]=' ';
            i --;
            if (Menu == 0)
            {
              if (i < 1) i=5;
              Selected[i]=Symbol[0];
              sprintf(value, "   %c%c %c%c%c", Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);
            }
            else
            {
              if (i < 0) i=5;
              Selected[i]=Symbol[0];
              sprintf(value, "  %c%c%c %c%c%c", Selected[0], Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);
            }
            GUI_DisString_EN(3, 16, value, &Font12, GUI_BACKGROUND, BLACK);
            while(GET_KEY_LEFT == 0) {
                usleep(100000);
            }
            /*GUI_DrawRectangle(20, 60, 40, 80, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(20, 60, 40, 80, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(23, 60, "G", &Font24, GUI_BACKGROUND, BLUE);*/
        }
        if(GET_KEY_RIGHT == 0) {
            if (Menu == 0)
            {
              sprintf(value, "   %c%c %c%c%c", Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);
            }
            else
            {
              sprintf(value, "  %c%c%c %c%c%c", Selected[0], Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);
            }
            GUI_DisString_EN(3, 16, value, &Font12, GUI_BACKGROUND, WHITE);
            Selected[i]=' ';
            i ++;
            if (Menu == 0)
            {
              if (i > 5) i=1;
              Selected[i]=Symbol[0];
              sprintf(value, "   %c%c %c%c%c", Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);
            }
            else
            {
              if (i > 5) i=0;
              Selected[i]=Symbol[0];
              sprintf(value, "  %c%c%c %c%c%c", Selected[0], Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);
            }
            GUI_DisString_EN(3, 16, value, &Font12, GUI_BACKGROUND, BLACK);
            while(GET_KEY_RIGHT == 0) {
                usleep(100000);
            }
            /*GUI_DrawRectangle(60, 60, 80, 80, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(60, 60, 80, 80, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(63, 60, "D", &Font24, GUI_BACKGROUND, BLUE);*/
        }
        if(GET_KEY_PRESS == 0) {
            if (Menu == 1)
            {
              sprintf(value, "SR: %s K/s", sr_list[m]);
              GUI_DisString_EN(3, 103, value, &Font12, GUI_BACKGROUND, WHITE);
              m ++;
              if (m > 4) m = 0;
              sprintf(sr, "%s", sr_list[m]);
              SetConfigParam(PATH_DATV_CONFIG, "sr", sr);
              sprintf(value, "SR: %s K/s", sr_list[m]);
              GUI_DisString_EN(3, 103, value, &Font12, GUI_BACKGROUND, BLACK);
            }
            while(GET_KEY_PRESS == 0) {
                usleep(100000);
            }
            /*GUI_DrawRectangle(40, 60, 60, 80, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(40, 60, 60, 80, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(43, 60, "C", &Font24, GUI_BACKGROUND, BLUE);*/
        }
        if(GET_KEY1 == 0) {
            if(CheckRTL()==0)
            {
              LCD_Clear(WHITE);
              if (Menu == 0)
              {
                start_sarsat();
              }
              else
              {
                start_datv();
              }
            }
            else
            {
              not_detected();
            }
            while(GET_KEY1 == 0) {
                usleep(100000);
            }
            /*GUI_DrawRectangle(95, 40, 120, 60, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(95, 40, 120, 60, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(98, 43, "K1", &Font16, GUI_BACKGROUND, BLUE);*/
        }
        if(GET_KEY2 == 0) {
            if (Menu == 0)
            {
              start_datv_menu();
            }
            else
            {
              Draw_Init();
            }
            while(GET_KEY2 == 0) {
                usleep(100000);
            }
            /*GUI_DrawRectangle(95, 60, 120, 80, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(95, 60, 120, 80, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(98, 63, "K2", &Font16, GUI_BACKGROUND, BLUE);*/
        }
        if(GET_KEY3 == 0) {
            if (Menu == 1)
            {
              sprintf(value, "%s FEC:%s", mode, fec_char[j]);
              GUI_DisString_EN(3, 116, value, &Font12, GUI_BACKGROUND, WHITE);
              if ((strcmp (mode, "DVB-S") == 0) && (strcmp (fec, "7") == 0))
              {
                j = 5;
                strcpy(fec, "Auto");
                strcpy(mode, "DVB-S2");
                SetConfigParam(PATH_DATV_CONFIG, "fec", fec);
                SetConfigParam(PATH_DATV_CONFIG, "mode", mode);
              }
              else if (strcmp (mode, "DVB-S") == 0)
              {
                j++;
                sprintf(fec, "%d", Fec_Dvbs[j]);
                SetConfigParam(PATH_DATV_CONFIG, "fec", fec);
              }
              else
              {
                j = 0;
                strcpy(fec, "1");
                strcpy(mode, "DVB-S");
                SetConfigParam(PATH_DATV_CONFIG, "fec", fec);
                SetConfigParam(PATH_DATV_CONFIG, "mode", mode);
              }
              sprintf(value, "%s FEC:%s", mode, fec_char[j]);
              GUI_DisString_EN(3, 116, value, &Font12, GUI_BACKGROUND, BLACK);
              while(GET_KEY3 == 0) {
                usleep(100000);
              }
            }
        }
    }
}
