/*****************************************************************************
* |	This version:   V1.0
* | Date        :   2019-07-06
* | Info        :   Basic version
*
******************************************************************************/
#define PATH_SARSAT_CONFIG "/home/pi/rpi0_sarsat/rpi_lcd_1.44/config.txt"

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

int nb[6];
int i=1;
char freq_config[31];

char value[256];
char Symbol[]={'_'};
char Selected[]={' ',' ',' ',' ',' ',' '};

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

void Draw_Init(void)

{

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


    GUI_DisString_EN(3, 32, "         Start>", &Font12, GUI_BACKGROUND, BLACK);

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
  while(GET_KEY3 == 1) usleep(1000);
  FinishedButton=1;
  return NULL;
}

void start_sarsat(void)
{
  #define PATH_SCRIPT_DECODER "/home/pi/rpi0_sarsat/406/scan.sh 2>&1"

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

  //char crc1[14]="";
  //char crc2[14]="";

  int nbline=1;

  char strTag[30];

  pthread_create (&thbutton, NULL, &WaitButtonEvent, NULL);

  fp=popen(PATH_SCRIPT_DECODER, "r");

  while (((read = getline(&line, &len, fp)) != -1) && (FinishedButton == 0))
  {

    sscanf(line,"%s ",strTag);

   int length=strlen(line);
   if (line[length-1] == '\n') line[length-1] = '\0';

    if (((strcmp(strTag, "Lancement")==0) || (strcmp(strTag, "ATTENTE")==0) || (strcmp(strTag, "CRC1")==0) || (strcmp(strTag, "CRC2")==0) || (strcmp(strTag, "Contenu")==0)) && (strcmp(strTag, old)!=0))
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
    if (strcmp(end, "CRC1 OK CRC2 OK")==0){
      LCD_SetArealColor(118, 118, 128, 128, GREEN);
    }
    else{
      LCD_SetArealColor(118, 118, 128, 128, RED);
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

void KEY_Listen(void)
{
    Draw_Init();
    for(;;) {
        usleep(1000);
        if(GET_KEY_UP == 0) {
            sprintf(value, "  %d%d%d.%d%d%d Mhz", nb[0], nb[1], nb[2], nb[3], nb[4], nb[5]);
            GUI_DisString_EN(3, 15, value, &Font12, GUI_BACKGROUND, WHITE);
            nb[i] ++;
            if (nb[i] > 9) nb[i]=0;
            sprintf(value, "  %d%d%d.%d%d%d Mhz", nb[0], nb[1], nb[2], nb[3], nb[4], nb[5]);
            GUI_DisString_EN(3, 15, value, &Font12, GUI_BACKGROUND, BLACK);
            sprintf(freq_config, "%d%d%d%d%d%d", nb[0], nb[1], nb[2], nb[3], nb[4], nb[5]);
            SetConfigParam(PATH_SARSAT_CONFIG, "freq", freq_config);
            while(GET_KEY_UP == 0) {
                usleep(1000);
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
            SetConfigParam(PATH_SARSAT_CONFIG, "freq", freq_config);
            while(GET_KEY_DOWN == 0) {
                usleep(1000);
            }
            /*GUI_DrawRectangle(40, 80, 60, 100, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(40, 80, 60, 100, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(43, 80, "B", &Font24, GUI_BACKGROUND, BLUE);*/
        }
        if(GET_KEY_LEFT == 0) {
            sprintf(value, "   %c%c %c%c%c", Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);
            GUI_DisString_EN(3, 16, value, &Font12, GUI_BACKGROUND, WHITE);
            Selected[i]=' ';
            i --;
            if (i < 1) i=5;
            Selected[i]=Symbol[0];
            sprintf(value, "   %c%c %c%c%c", Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);
            GUI_DisString_EN(3, 16, value, &Font12, GUI_BACKGROUND, BLACK);
            while(GET_KEY_LEFT == 0) {
                usleep(1000);
            }
            /*GUI_DrawRectangle(20, 60, 40, 80, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(20, 60, 40, 80, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(23, 60, "G", &Font24, GUI_BACKGROUND, BLUE);*/
        }
        if(GET_KEY_RIGHT == 0) {
            sprintf(value, "   %c%c %c%c%c", Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);
            GUI_DisString_EN(3, 16, value, &Font12, GUI_BACKGROUND, WHITE);
            Selected[i]=' ';
            i ++;
            if (i > 5) i=1;
            Selected[i]=Symbol[0];
            sprintf(value, "   %c%c %c%c%c", Selected[1], Selected[2], Selected[3], Selected[4], Selected[5]);
            GUI_DisString_EN(3, 16, value, &Font12, GUI_BACKGROUND, BLACK);
            while(GET_KEY_RIGHT == 0) {
                usleep(1000);
            }
            /*GUI_DrawRectangle(60, 60, 80, 80, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(60, 60, 80, 80, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(63, 60, "D", &Font24, GUI_BACKGROUND, BLUE);*/
        }
        if(GET_KEY_PRESS == 0) {
            while(GET_KEY_PRESS == 0) {
                usleep(1000);
            }
            /*GUI_DrawRectangle(40, 60, 60, 80, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(40, 60, 60, 80, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(43, 60, "C", &Font24, GUI_BACKGROUND, BLUE);*/
        }
        if(GET_KEY1 == 0) {
            LCD_Clear(WHITE);
            start_sarsat();
            while(GET_KEY1 == 0) {
                usleep(1000);
            }
            /*GUI_DrawRectangle(95, 40, 120, 60, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(95, 40, 120, 60, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(98, 43, "K1", &Font16, GUI_BACKGROUND, BLUE);*/
        }
        if(GET_KEY2 == 0) {
            while(GET_KEY2 == 0) {
                usleep(1000);
            }
            /*GUI_DrawRectangle(95, 60, 120, 80, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(95, 60, 120, 80, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(98, 63, "K2", &Font16, GUI_BACKGROUND, BLUE);*/
        }
        /*if(GET_KEY3 == 0) {
            system("/home/pi/406/stop.sh >/dev/null 2>/dev/null");
            system("sudo killall scan.sh >/dev/null 2>/dev/null");
            system("pkill -9 rtl_power && pkill perl && pkill -9 perl >/dev/null 2>/dev/null");
            Draw_Init();
            while(GET_KEY3 == 0) {
                usleep(1000);
            }*/
            /*GUI_DrawRectangle(95, 80, 120, 100, WHITE, DRAW_FULL, DOT_PIXEL_DFT);
            GUI_DrawRectangle(95, 80, 120, 100, RED, DRAW_EMPTY, DOT_PIXEL_DFT);
            GUI_DisString_EN(98, 83, "K3", &Font16, GUI_BACKGROUND, BLUE);
        }*/
    }
}
