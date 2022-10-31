/****************************************************************************************/
/* Décodage de Balises de détresse 406Mhz ou d'exercice 432                             */
/* à partir d'un flux audio provenant:                                                  */
/*   - d'un logiciel sdr (gqrx par exemple)                                             */
/*   - ou de l'entrée Micro reliée à la sortie "discri" d'un transceiver                */
/*   - ou de la lecture d'un enregistrement audio  ".wav"                               */
/*                                                                                      */
/*                                           F4EHY 10-7-2020 version  V6.0              */
/*                                                                                      */
/****************************************************************************************/
/*--------------------------------------------------------------------------------------*/
/* Compilation du fichier source:                                                       */
/*      gcc dec406.c -lm -o dec406                                                      */
/*--------------------------------------------------------------------------------------*/
/* Options:                                                                             */
/*      --osm  pour affichage sur une carte OpenStreetMap                               */
/*      --help aide                                                                     */
/*---------------------------------------------------------------------------------------------*/
/* Lancement du programme:                                                                     */
/* sox -t alsa default -t wav - lowpass 3000 highpass 10 gain -l 6 2>/dev/null |./dec406       */
/* sox -t alsa default -t wav - lowpass 3000 highpass 10 gain -l 6 2>/dev/null |./dec406 --osm */
/*---------------------------------------------------------------------------------------------*/
/* Installer 'sox' s'il est absent:                                                     */
/*      sudo apt-get install sox                                                        */
/* 'sox' met en forme le flux audio qui est redirigé vers 'dec406' pour le décodage     */
/*--------------------------------------------------------------------------------------*/

/*Exemple de trame longue avec GPS (144 bits)
1111111111111110110100001000111000111111001100111110101111001011111011110000001100100100001010011011111101110111000100100000010000001101011010001
Contenu utile en Hexa bits 25 à 144:
8e3f33ebcbef032429bf7712040d68
*/
#define PATH_LOG "/var/www/html/decode.txt"

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
typedef unsigned char  uin8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
#define  PI 4*atan(1)
#define SEUIL 2000 //3000
int bauds=400;      //400 bits par secondes
int f_ech = 0;      //frequence echantillonnage
int ech_par_bit = 0;//f_ech/baud
int longueur_trame=144;   //144  ou 112 bits avec la synchro
int lon=30;         //30 ou 22 longueur message hexa en quartets(4bits)  (debut message :bit 25)
int bits = 0;       //codage des echantillons 8 ou 16 bits
int N_canaux = 0;   //Nbre de canaux audio
int opt_osm = 0;    //argument pour affichage sur OpenStreetMap
int no_checksum=0;  //argument pour activer le décodage sur checksum à 0
int opt_minute = 0;
int flux_wav = 1;   //flux audio via stdin ou fichier .wav
int canal_audio = 0;//canal audio
int n_ech = 0;      //numero de l'echantillon
int seuilM=SEUIL;   //seuil pics ou fronts positifs
int seuilm=-SEUIL;  //seuil pics ou fronts negatifs
int seuil=-SEUIL;   //seuil actif

char s[200];        // 144 bits maxi+ 0 terminal
char value[10];     //

char now[30];

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

int calcul(int a, int b)
    {int i,y=0,x=1;
    for (i=b;i>=a;i--)
        {if(s[i]=='1') y=y+x;
        x=2*x;
        }
    return y;
    }
void envoi_byte(int x){
    int a,b;
    //String aa,bb;
    a=x/16;
    b=x%16;
    fprintf(stderr,"%x",a);
    fprintf(stderr,"%x",b);
    //aa= //Integer.toHex//String(a);
    //bb= //Integer.toHex//String(b);
    //contenu.append(aa);
    //contenu.append(bb);
}
int afficher_carte_osm(double llatd, double llngd){
    char ch[200];
    float lat,lng;
    int a;
    lat=(float)llatd;lng=(float)llngd;
    //Navigateur chromium-browser ou firefox
    FILE * file;
   file = fopen("/usr/bin/chromium-browser", "r");

    if (file){//chromium-browser existe
        sprintf(ch,"/usr/bin/chromium-browser --disable-gpu 'https://www.openstreetmap.org/?mlat=%f&mlon=%f#map=6/%f/%f &'> /dev/null 2>&1",lat,lng,lat,lng);
        fclose(file);
        a=system(ch);
        }
   else {
        file=fopen("/usr/bin/firefox", "r");
        if (file){
            fclose(file);
            sprintf(ch,"/usr/bin/firefox 'https://www.openstreetmap.org/?mlat=%f&mlon=%f#map=6/%f/%f & '> /dev/null 2>&1",lat,lng,lat,lng);
            system(ch);
            //execlp("/usr/bin/firefox","/usr/bin/firefox ", ch,  (char *) 0);
        }
        else fprintf(stderr,"\n*Installer Chromium-browser ou Firefox pour afficher la position avec OpenStreeMap...");
       }
return 0;
}

void  GeogToUTM(double llatd, double llngd){
		//Merci à Daniel-Bryant
        //https://github.com/daniel-bryant/CoordinateConversion/blob/master/conversion.cpp

		    if ((llatd>127)||(llngd>255)||(llatd<0)||(llngd<0))
                {fprintf(stderr,"\nCoordonnees HS");
                }
            else{
                if (opt_osm)  afficher_carte_osm(llatd,llngd);
                double DatumEqRad[] = {6378137.0,6378137.0,6378137.0,6378135.0,6378160.0,6378245.0,6378206.4,
                6378388.0,6378388.0,6378249.1,6378206.4,6377563.4,6377397.2,6377276.3};
                double DatumFlat[] = {298.2572236, 298.2572236, 298.2572215,	298.2597208, 298.2497323, 298.2997381, 294.9786982,
                296.9993621, 296.9993621, 293.4660167, 294.9786982, 299.3247788, 299.1527052, 300.8021499};
                int Item = 0;//Default WGS84
                double k0 = 0.9996;//scale on central meridian
                double a = DatumEqRad[Item];//equatorial radius, meters du WGS84.
                double f = 1/DatumFlat[Item];//polar flattening du WGS84.
                double b = a*(1-f);//polar axis.
                double e = sqrt(1 - b*b/a*a);//eccentricity
                double drad = PI/180;//Convert degrees to radians)
                double latd = 0;//latitude in degrees
                double phi = 0;//latitude (north +, south -), but uses phi in reference
                double e0 = e/sqrt(1 - e*e);//e prime in reference
                double N = a/sqrt(1-pow((e*sin(phi)),2));
                double T = pow(tan(phi),2);
                double C = pow(e*cos(phi),2);
                double lng = 0;//Longitude (e = +, w = -) - can't use long - reserved word
                double lng0 = 0;//longitude of central meridian
                double lngd = 0;//longitude in degrees
                double M = 0;//M requires calculation
                double x = 0;//x coordinate
                double y = 0;//y coordinate
                double k = 1;//local scale
                double utmz = 30;//utm zone milieu
                double zcm = 0;//zone meridien centrale
                char DigraphLetrsE[] = {'A','B', 'C','D', 'E', 'F', 'G', 'H','J','K','L','M','N','P','Q','R','S','T','U','V','W','X','Y','Z'};
                char DigraphLetrsN[] = {'A','B', 'C','D', 'E', 'F', 'G', 'H','J','K','L','M','N','P','Q','R','S','T','U','V'};
                k0 = 0.9996;//scale on central meridian
                b = a*(1-f);//polar axis.
                e = sqrt(1 - (b/a)*(b/a));//eccentricity
                // --- conversion en UTM --------------------------------
                latd=llatd;
                lngd=llngd;
                phi = latd*drad;//Convert latitude to radians
                lng = lngd*drad;//Convert longitude to radians
                utmz = 1 + floor((lngd+180)/6);//calculate utm zone
                double latz = 0;//Latitude zone: A-B S of -80, C-W -80 to +72, X 72-84, Y,Z N of 84
                if (latd > -80 && latd < 72){latz = floor((latd + 80)/8)+2;}
                if (latd > 72 && latd < 84){latz = 21;}
                if (latd > 84){latz = 23;}
                zcm = 3 + 6*(utmz-1) - 180;//Central meridian of zone
                //Calcul Intermediate Terms
                e0 = e/sqrt(1 - e*e);//Called e prime in reference
                double esq = (1 - (b/a)*(b/a));//e squared for use in expansions
                double e0sq = e*e/(1-e*e);// e0 squared - always even powers
                N = a/sqrt(1-pow((e*sin(phi)),2));
                T = pow(tan(phi),2);
                C = e0sq*pow(cos(phi),2);
                double A = (lngd-zcm)*drad*cos(phi);
                //Calcul M
                M = phi*(1 - esq*(1/4 + esq*(3/64 + 5*esq/256)));
                M = M - sin(2*phi)*(esq*(3/8 + esq*(3/32 + 45*esq/1024)));
                M = M + sin(4*phi)*(esq*esq*(15/256 + esq*45/1024));
                M = M - sin(6*phi)*(esq*esq*esq*(35/3072));
                M = M*a;//Arc length along standard meridian
                double EE=0.0818192;
                double SS=(1-e*e/4-3*e*e*e*e/64-5*e*e*e*e*e*e/256)*phi;
                        SS=SS-(3*e*e/8+3*e*e*e*e/32+45*e*e*e*e*e*e/1024)*sin(2*phi);
                        SS=SS+(15*e*e*e*e/256+45*e*e*e*e*e*e/1024)*sin(4*phi);
                        SS=SS-(35*e*e*e*e*e*e/3072)*sin(6*phi);
                double NU=1/sqrt(1-EE*EE*sin(phi)*sin(phi));
                x=500000+0.9996*a*NU*(A+(1-T+C)*A*A*A/6+(5-18*T+T*T)*A*A*A*A*A/120) ;
                y= 0.9996*a*(SS+NU*tan(phi)*(A*A/2+(5-T+9*C+4*C*C)*A*A*A*A/24+(61-58*T+T*T)*A*A*A*A*A*A/720));
                if (y < 0){y = 10000000+y;}
                x= round(10*(x))/10;
                y= round(10*y)/10;
                //Genere les lettres des Zones
                char C0 = DigraphLetrsE[(int)latz];
                double Letr = floor((utmz-1)*8 + (x)/100000);
                Letr = Letr - 24*floor(Letr/24)-1;
                char C1 = DigraphLetrsE[(int)Letr];
                Letr = floor(y/100000);
                if (utmz/2 == floor(utmz/2)){Letr = Letr+5;}
                Letr = Letr - 20*floor(Letr/20);
                char C2 =  DigraphLetrsN[(int)Letr];
                //fprintf(stderr,"\nUTM Zone: %1.0f%c",utmz,C0);
                //fprintf(stderr,"\n%c%c",C1,C2);
                //fprintf(stderr,"\nx: %1.0fm  y: %1.0fm",x,y);
                //contenu.append("UTM: Zone:").append(decform.format(utmz)).append(//Charact.to//String(C0));
                //contenu.append(" ").append(//Charact.to//String(C1)).append(//Charact.to//String(C2));
                //contenu.append(" x: ").append(decform.format(x)).append("m");
                //contenu.append(" y: ").append(decform.format(y)).append("m\n");
             }
}

void standard_test()
    {   int i,j,a;
        for (i=40;i<64;i++)
            {//Charact ch=s[i];
            fprintf(stderr,"%c",s[i]);
            //contenu.append(ch.toString());
            }
        fprintf(stderr," en Hexa:");
        //contenu.append(" en Hexa: ");
        for(j=0;j<3;j++)
                                {i=40+j*8;
                                a=calcul(i,i+7);
                                envoi_byte(a);
                                }
    }

char  baudot(int x)
	{char y;
	switch(x)
		{case 56: y='A';break;
		case 51: y='B';break;
		case 46: y='C';break;
		case 50: y='D';break;
		case 48: y='E';break;
		case 54: y='F';break;
		case 43: y='G';break;
		case 37: y='H';break;
		case 44: y='I';break;
		case 58: y='J';break;
		case 62: y='K';break;
		case 41: y='L';break;
		case 39: y='M';break;
		case 38: y='N';break;
		case 35: y='O';break;
		case 45: y='P';break;
		case 61: y='Q';break;
		case 42: y='R';break;
		case 52: y='S';break;
		case 33: y='T';break;
		case 60: y='U';break;
		case 47: y='V';break;
		case 57: y='W';break;
		case 55: y='X';break;
		case 53: y='Y';break;
		case 49: y='Z';break;
		case 36: y=' ';break;
		case 24: y='-';break;
		case 23: y='/';break;
		case 13: y='0';break;
		case 29: y='1';break;
		case 25: y='2';break;
		case 16: y='3';break;
		case 10: y='4';break;
		case 1: y='5';break;
		case 21: y='6';break;
		case 28: y='7';break;
		case 12: y='8';break;
		case 3: y='9';break;
		default: y='_';
		}
	return y;
	}

  void affiche_baudot42()
	{int i,j;
	 int a;
	for(j=0;j<6;j++)
		{i=39+j*6;
		//a=32*(s[i]=='1')+16*(s[i+1]=='1')+8*(s[i+2]=='1')+4*(s[i+3]=='1')+2*(s[i+4]=='1')+(s[i+5]=='1');
		a=calcul(i,i+5);
        fprintf(stderr,"%c",baudot(a));
        //Charact ch = baudot(a);
		//contenu.append(ch.to//String());
		}
         //fprintf(stderr,"\n"); //contenu.append("\n") ;
	   }

void affiche_baudot_2()
	{int i,j;
	 int a;
	for(j=0;j<7;j++)
		{i=39+j*6;
		//a=32*(s[i]=='1')+16*(s[i+1]=='1')+8*(s[i+2]=='1')+4*(s[i+3]=='1')+2*(s[i+4]=='1')+(s[i+5]=='1');
		a=calcul(i,i+5);
        fprintf(stderr,"%c",baudot(a));
        //Charact ch = baudot(a);
		//contenu.append(ch.to//String());
		}
         //fprintf(stderr,"\n");
         //contenu.append("\n") ;
	   }
void specific_beacon()
	{int i,j;
	int a;
	fprintf(stderr,"\nSpecific beacon: ");//contenu.append("Specific beacon: ");
	for(j=0;j<1;j++)
		{i=75+j*6;
		//a=32*(s[i]=='1')+16*(s[i+1]=='1')+8*(s[i+2]=='1')+4*(s[i+3]=='1')+2*(s[i+4]=='1')+(s[i+5]=='1');
		a=calcul(i,i+5);
        fprintf(stderr,"%c",baudot(a));
		//Charact ch = baudot(a);
		//contenu.append(ch.to//String());
		}
         //   fprintf(stderr,"\n");   //contenu.append("\n");
	   }


void affiche_baudot_1()
	{
      int i;
      int j;
      int b;
      int c;
      int d;
	  int a;
      fprintf(stderr,"\nRadio Id:");
	 //contenu.append("Radio Call Sign Identification: ") ;
	  for(j=0;j<4;j++)
		{
        i=39+j*6;
		//a=32*(s[i]=='1')+16*(s[i+1]=='1')+8*(s[i+2]=='1')+4*(s[i+3]=='1')+2*(s[i+4]=='1')+(s[i+5]=='1');
		a=calcul(i,i+5);
                fprintf(stderr,"%c",baudot(a));
                //Charact ch = baudot(a);
              	//contenu.append(ch.to//String());
		}
         ////contenu.append("\n");
         fprintf(stderr," ");
        i=63;
        //b=8*(s[i]=='1')+4*(s[i+1]=='1')+2*(s[i+2]=='1')+(s[i+3]=='1');
        b=calcul(i,i+3);
        //c=8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
        c=calcul(i+4,i+7);
        //d=8*(s[i+8]=='1')+4*(s[i+9]=='1')+2*(s[i+10]=='1')+(s[i+11]=='1');
        d=calcul(i+8,i+11);
        fprintf(stderr,"%d",b);
        fprintf(stderr,"%d",c);
        fprintf(stderr,"%d",d);
        //contenu.append(b);
        //contenu.append(c);
        //contenu.append(d);
        //contenu.append("\n");
	   }
void affiche_baudot()
	{
    int i,j,a;
	for(j=0;j<4;j++)
		{i=39+j*6;
		//a=32*(s[i]=='1')+16*(s[i+1]=='1')+8*(s[i+2]=='1')+4*(s[i+3]=='1')+2*(s[i+4]=='1')+(s[i+5]=='1');
		a=calcul(i,i+5);
        fprintf(stderr,"%c",baudot(a));
        //Charact ch = baudot(a);
		//contenu.append(ch.to//String());
		}
        i=63;
        //a=2048*(s[i]=='1')+1024*(s[i+1]=='1')+512*(s[i+2]=='1')+256*(s[i+3]=='1')+128*(s[i+4]=='1')+64*(s[i+5]=='1')+32*(s[i+6]=='1')+16*(s[i+7]=='1')+8*(s[i+8]=='1')+4*(s[i+9]=='1')+2*(s[i+10]=='1')+(s[i+11]=='1');
        a=calcul(i,i+11);
        fprintf(stderr,"%d",a);
        //Intege aa= a;
        //contenu.append(aa.to//String());
        i=81;
        //a=2*(s[i]=='0')+(s[i+1]=='0');
        a=calcul(i,i+1);
        fprintf(stderr,"%d",a);
        //aa= a;
        //contenu.append(aa.to//String());
    }

void affiche_baudot30()
	{int i,j,a;
	for(j=0;j<7;j++)
		{i=43+j*6;
		//a=32*(s[i]=='1')+16*(s[i+1]=='1')+8*(s[i+2]=='1')+4*(s[i+3]=='1')+2*(s[i+4]=='1')+(s[i+5]=='1');
		a=calcul(i,i+5);
        fprintf(stderr,"%c",baudot(a));
        //Charact ch = baudot(a);
		//contenu.append(ch.to//String());
		}
	}

void localisation_standard()
	{char  c ;
    int i,x,latD,latM,latS,lonD,lonM,lonS;
	if (s[64]=='0') c='N'; else c='S';
    //DecimalFormat decform = new DecimalFormat("###.########");
	i=65;
	//latD=64*(s[i]=='1')+32*(s[i+1]=='1')+16*(s[i+2]=='1')+8*(s[i+3]=='1')+4*(s[i+4]=='1')+2*(s[i+5]=='1')+(s[i+6]=='1');
    latD=calcul(i,i+6);
    //latM=15*(2*(s[i+7]=='1')+(s[i+8]=='1'));
    latM=15*calcul(i+7,i+8);
	//ofsset minutes et secondes
	i=113;
	//x=16*(s[i]=='1')+8*(s[i+1]=='1')+4*(s[i+2]=='1')+2*(s[i+3]=='1')+(s[i+4]=='1');
	x=calcul(i,i+4);
    if(s[112]=='0') {x=-x;}
	latM+=x;
	i=118;
	//latS=4*(8*(s[i]=='1')+4*(s[i+1]=='1')+2*(s[i+2]=='1')+(s[i+3]=='1'));
	latS=4*calcul(i,i+3);
    if(s[112]=='0')   //latS=-latS;
				{latS=60-latS;latM--;
					if (latM<0) {latM=60+latM;latD--;}
				}
	//fprintf(stderr,"Latitude: ");fprintf(stderr,"%c",c);fprintf(stderr," ");fprintf(stderr,"%d",latD);fprintf(stderr,"d");fprintf(stderr,"%d",latM);fprintf(stderr,"m");fprintf(stderr,"%d",latS);fprintf(stderr,"s");
	fprintf(stderr,"\nLat: %c%dd%dm%ds ",c,latD,latM,latS);
    //Charact cc = c;//Integer llatD = latD;//Integer llatM = latM;//Integer llatS = latS;
    ////contenu.append("Latitude  : "+cc.to//String()+" "+llatD.to//String()+"d"+llatM.to//String()+"m"+llatS.to//String()+"s\n");
    //contenu.append("\nLatitude : ").append(cc.to//String()).append(" ").append(llatD.to//String()).append("d").append(llatM.to//String()).append("m").append(llatS.to//String()).append("s");
    double gpsLat= latD+(60.0*latM+latS)/3600.0;
    fprintf(stderr,"\n%2.4f Deg",gpsLat);
    sprintf(value, "%2.4f", gpsLat);
    SetConfigParam(PATH_LOG, "lat", value);
    //contenu.append(" = ").append(decform.format(gpsLat)).append(" Deg\n");
	if (s[74]=='0') {c='E';} else {c='W';}
	i=75;
	//lonD=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
    lonD=calcul(i,i+7);
    //lonM=15*(2*(s[i+8]=='1')+(s[i+9]=='1'));
    lonM=15*calcul(i+8,i+9);
	//ofsset minutes et secondes
	i=123;
	//x=16*(s[i]=='1')+8*(s[i+1]=='1')+4*(s[i+2]=='1')+2*(s[i+3]=='1')+(s[i+4]=='1');
	x=calcul(i,i+4);
    if(s[122]=='0') {x=-x;}
	lonM+=x;
	i=128;
	//lonS=4*(8*(s[i]=='1')+4*(s[i+1]=='1')+2*(s[i+2]=='1')+(s[i+3]=='1'));
	lonS=4*calcul(i,i+3);
    if(s[122]=='0') //{lonS=-lonS;}
				{lonS=60-lonS;lonM--;
					if (lonM<0) {lonM=60+lonM;lonD--;}
				}

    fprintf(stderr,"\nLong: %c%dd%dm%ds ",c,lonD,lonM,lonS);
    double gpsLon= lonD+(60.0*lonM+lonS)/3600.0;
    fprintf(stderr,"\n%3.4f Deg",gpsLon);
    sprintf(value, "%3.4f", gpsLon);
    SetConfigParam(PATH_LOG, "lon", value);
    GeogToUTM(gpsLat, gpsLon);
    }

 void supplementary_data()
 {  //s[109]=1;  ?????????????????????????????????????????????
    /*s[108]=0;
    s[107]=1;
    s[106]=1;*/
    fprintf(stderr,"\nFixed bits (1101) Pass");
    //contenu.append("Fixed bits (1101) Pass\n");
    if (s[110]=='1') {fprintf(stderr,"\nEncoded pos int");}
                       //contenu.append("Encoded position data source internal\n");}
    if (s[111]=='1') {fprintf(stderr,"\n121.5MHz Homing");}
                        //contenu.append("121.5 MHz Homing\n");}

}

 void supplementary_data_1()
 {/* s[108]=0;
    s[107]=1;
    s[106]=1;*/
    //if (s[109]=='1') {fprintf(stderr,"\nAdditional Data Flag:Position");}
                        //contenu.append(" Additional Data Flag:Position\n"); }
    //if (s[110]=='1') {fprintf(stderr,"\nEncoded position data source internal");}
                         //contenu.append("Encoded position data source internal\n");}
    //if (s[111]=='1') {fprintf(stderr,"\n121.5 MHz Homing");}
                          //contenu.append("121.5 MHz Homing\n");}
 }

void localisation_standard1()
	{char c;
    int i,x,latD,latM,latS,lonD,lonM,lonS;
	if (s[64]=='0') c='N'; else c='S';
	i=65;
	//latD=64*(s[i]=='1')+32*(s[i+1]=='1')+16*(s[i+2]=='1')+8*(s[i+3]=='1')+4*(s[i+4]=='1')+2*(s[i+5]=='1')+(s[i+6]=='1');
    latD=calcul(i,i+6);
    //latM=15*(2*(s[i+7]=='1')+(s[i+8]=='1'));
	latM=15*calcul(i+7,i+8);
    //ofsset minutes et secondes
	i=113;
	//x=16*(s[i]=='1')+8*(s[i+1]=='1')+4*(s[i+2]=='1')+2*(s[i+3]=='1')+(s[i+4]=='1');
	x=calcul(i,i+4);
    if(s[112]=='0') {x=-x;}
    latM+=x;
	i=118;
	//latS=4*(8*(s[i]=='1')+4*(s[i+1]=='1')+2*(s[i+2]=='1')+(s[i+3]=='1'));
	latS=4*calcul(i,i+3);
    if(s[112]=='0') //{latS=-latS;}
   		{latS=60-latS;latM--;
	if (latM<0) {latM=60+latM;latD--;}
		}
	//fprintf(stderr,"Latitude: ");fprintf(stderr,c);fprintf(stderr,' ');fprintf(stderr,latD);fprintf(stderr,'d');fprintf(stderr,latM);fprintf(stderr,'m');fprintf(stderr,latS);fprintf(stderr,'s');
    fprintf(stderr,"\nLat: %c%dd%dm%ds ",c,latD,latM,latS);//Charact cc = c;//Integer llatD = latD;//Integer llatM = latM;//Integer llatS = latS;
    //contenu.append("Latitude  : "+cc.to//String()+" "+llatD.to//String()+"d"+llatM.to//String()+"m"+llatS.to//String()+"s  \n");
    //contenu.append("\nLatitude : ").append(cc.to//String()).append(" ").append(llatD.to//String()).append("d").append(llatM.to//String()).append("m").append(llatS.to//String()).append("s");
    double gpsLat= latD+(60.0*latM+latS)/3600.0;
    fprintf(stderr,"\n%2.4f Deg",gpsLat);
    sprintf(value, "%2.4f", gpsLat);
    SetConfigParam(PATH_LOG, "lat", value);
    //contenu.append(" = ").append(decform.format(gpsLat)).append(" Deg\n");
    if (s[74]=='0') {c='E';} else {c='W';}
	i=75;
	//lonD=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
    lonD=calcul(i,i+7);
    //lonM=15*(2*(s[i+8]=='1')+(s[i+9]=='1'));
	lonM=15*calcul(i+8,i+9);
    //ofsset minutes et secondes
	i=123;
	//x=16*(s[i]=='1')+8*(s[i+1]=='1')+4*(s[i+2]=='1')+2*(s[i+3]=='1')+(s[i+4]=='1');
	x=calcul(i,i+4);
    if(s[122]=='0') {x=-x;}
    lonM+=x;
	i=128;
	//lonS=4*(8*(s[i]=='1')+4*(s[i+1]=='1')+2*(s[i+2]=='1')+(s[i+3]=='1'));
	lonS=4*calcul(i,i+3);
    if(s[122]=='0') //{lonS=-lonS;}
		{lonS=60-lonS;lonM--;
        if (lonM<0) {lonM=60+lonM;lonD--;}
        }
	 //fprintf(stderr,"Longitude: ");fprintf(stderr,c);fprintf(stderr,' ');fprintf(stderr,lonD);fprintf(stderr,'d');fprintf(stderr,lonM);fprintf(stderr,'m');fprintf(stderr,lonS);fprintf(stderr,'s');
     fprintf(stderr,"\nLong: %c%dd%dm%ds ",c,lonD,lonM,lonS);
	//Charact ccc = c;//Integer llonD = lonD;//Integer llonM = lonM;//Integer llonS = lonS;
    // //contenu.append("Longitude: "+ccc.to//String()+" "+llonD.to//String()+"d"+llonM.to//String()+"m"+llonS.to//String()+"s\n");
    //contenu.append("Longitude: ").append(ccc.to//String()).append(" ").append(llonD.to//String()).append("d").append(llonM.to//String()).append("m").append(llonS.to//String()).append("s");
    double gpsLon= lonD+(60.0*lonM+lonS)/3600.0;
    fprintf(stderr,"\n%3.4f Deg",gpsLon);
    sprintf(value, "%3.4f", gpsLon);
    SetConfigParam(PATH_LOG, "lon", value);
    //contenu.append(" = ").append(decform.format(gpsLon)).append(" Deg\n");
    GeogToUTM(gpsLat, gpsLon);
    /*if (s[110]==1)
    {
     fprintf(stderr,"Encoded Position Data Source Internal");
        //contenu.append("Encoded Position Data Source Internal\n");
    }
    else
    {
    fprintf(stderr,"Encoded Position Data Source External");
        //contenu.append("Encoded Position Data Source External\n");
    }
    s[109]=1;
    s[108]=0;
    s[107]=1;
    s[106]=1; */
  }

void identification_MMSI()
	{int i,b;
	float a=1,x=0;
	int xx;
	for(i=59;i>39;i--)
		{if (s[i]=='1') x+=a;
                a*=2;
		}
	xx=(int)x;
	fprintf(stderr,"\nID MMSI: ");
    //contenu.append("Identifiant MMSI: ");
    //Integer xxx=xx;
    fprintf(stderr,"%d",xx);
    //contenu.append(xxx.to//String()).append("\n");
	i= 60;
	//b=8*(s[i]=='1')+4*(s[i+1]=='1')+2*(s[i+2]=='1')+(s[i+3]=='1');
	b=calcul(i,i+3);
    fprintf(stderr,"\nBeacon : ");
    //contenu.append("Beacon : ");
    //Integer bb=b;
    fprintf(stderr,"%d",b);
    //contenu.append(bb.to//String()).append("\n");
}

void identification_AIRCRAFT_24_BIT_ADRESS()
{
	int i,j,aa;
	float a=1,x=0;
	int xx;
	for(i=63;i>39;i--)
            {if (s[i]=='1') x+=a;
             a*=2;
            }
	xx=(int)x;
	fprintf(stderr,"\nID AIRCRAFT 24:");
    //contenu.append("Identifiant AIRCRAFT 24 BIT ADRESSE: ");
    //Integer xxx=xx;
    fprintf(stderr,"%d",xx);
    //contenu.append(xxx.to//String()).append("\n");
    fprintf(stderr,"en Hexa:");
    //contenu.append(" en Hexa: ");
    for(j=0;j<3;j++)
        {i=40+j*8;
         aa=calcul(i,i+7);
         envoi_byte(aa);
         }
}

void identification_AIRCRAFT_OPER_DESIGNATOR()
{	int i,b;
	float a=1,x=0;
	int xx;
	for(i=54;i>39;i--)
	{if (s[i]=='1') x+=a;
         a*=2;
	}
	xx=(int)x;
	fprintf(stderr,"\nID AIRCRAFT OPER DESIGNATOR: ");
    //contenu.append("Identifiant AIRCRAFT OPER DESIGNATOR: ");
    fprintf(stderr,"%d",xx);
    //Integer xxx=xx;
    //contenu.append(xxx.to//String()).append("\n");
	i= 55;
	//b=256*(s[i]=='1')+128*(s[i+1]=='1')+64*(s[i+2]=='1')+32*(s[i+3]=='1')+ 16*(s[i+4]=='1')+8*(s[i+5]=='1')+4*(s[i+6]=='1')+2*(s[i+7]=='1')+(s[i+8]=='1');
	b=calcul(i,i+8);
    fprintf(stderr,"\nSERIAL No: ");
    //contenu.append("SERIAL No: ");
    //Integer bb=b;
        fprintf(stderr,"%d",b);
        //contenu.append(bb.to//String()).append("\n");
	}
    void identification_C_S_TA_No()
	{
     int i,b;
	 float a=1,x=0;
	 int xx;
	 for(i=49;i>39;i--)
		{if (s[i]=='1') x+=a;
                a*=2;
		}
	xx=(int)x;
	fprintf(stderr,"\nID C/S TA No: ");
        //contenu.append("Identifiant C/S TA No: ");
        //Integer xxx=xx;
        fprintf(stderr,"%d",xx);
        //contenu.append(xxx.to//String()).append("\n");
	i= 50;
	//b=8192*(s[i]=='1')+4096*(s[i+1]=='1')+2048*(s[i+2]=='1')+1024*(s[i+3]=='1')+512*(s[i+4]=='1')+256*(s[i+5]=='1')+128*(s[i+6]=='1')+64*(s[i+7]=='1')+32*(s[i+8]=='1')+ 16*(s[i+9]=='1')+8*(s[i+10]=='1')+4*(s[i+11]=='1')+2*(s[i+12]=='1')+(s[i+13]=='1');
	b=calcul(i,i+13);
        fprintf(stderr,"\nSERIAL_No: ");
        //contenu.append("SERIAL_No: ");
        //Integer bb=b;
        fprintf(stderr,"%d",b);
        //contenu.append(bb.to//String()).append("\n");
	}

void identification_MMSI_FIXED()
{
	int i,b;
	float a=1,x=0;
	int xx;
	for(i=59;i>39;i--)
            {if (s[i]=='1') x+=a;
                 a*=2;
            }
	xx=(int)x;
	fprintf(stderr,"\nID MMSI: ");
        //contenu.append("Identifiant MMSI: ");
        //Integer xxx=xx;
        fprintf(stderr,"%d",xx);
        //contenu.append(xxx.to//String()).append("\n");
	i= 60;
	//b=8*(s[i]=='0')+4*(s[i+1]=='0')+2*(s[i+2]=='0')+(s[i+3]=='0');
	b=calcul(i,i+3);
        fprintf(stderr,"Fixed: ");
        //contenu.append("Fixed: ");
        //Integer bb=b;
        fprintf(stderr,"%d",b);
        //contenu.append(bb.to//String()).append("\n");
}



void localisation_nationale() //voir doc A-27-28-29
	{//fprintf(stderr,"\nID et localisation Nationale à implanter");//contenu.append("ID et localisation Nationale à implanter\n");
	char c; int i,x,latD,latM,lonD,lonM,latS,lonS;
    int a;
	if (s[58]=='0') c='N'; else c='S';
	i=59;
	//latD=64*(s[i]=='1')+32*(s[i+1]=='1')+16*(s[i+2]=='1')+8*(s[i+3]=='1')+4*(s[i+4]=='1')+2*(s[i+5]=='1')+(s[i+6]=='1');
    latD=calcul(i,i+6);
    //latM=2*(16*(s[i+7]=='1')+8*(s[i+8]=='1')+4*(s[i+9]=='1')+2*(s[i+10]=='1')+(s[i+11]=='1'));
	latM=2*calcul(i+7,i+11);
    i=113;
	//x=2*(s[i]=='1')+(s[i+1]=='1');
	x=calcul(i,i+1);
    if(s[112]=='0') {x=-x;}
    latM+=x;
	i=115;
	//latS=4*(8*(s[i]=='1')+4*(s[i+1]=='1')+2*(s[i+2]=='1')+(s[i+3]=='1'));
	latS=4*calcul(i,i+3);
    if(s[112]=='0') //{latS=-latS;}
				{latS=60-latS;latM--;
					if (latM<0) {latM=60+latM;latD--;}
				}

	//fprintf(stderr,"Latitude: ");fprintf(stderr,c);fprintf(stderr,' ');fprintf(stderr,latD);fprintf(stderr,'d');fprintf(stderr,latM);fprintf(stderr,'m');fprintf(stderr,latS);fprintf(stderr,'s');
	fprintf(stderr,"\nLat: %c%dd%dm%ds ",c,latD,latM,latS);
    //Charact cc = c;//Integer llatD = latD;//Integer llatM = latM;//Integer llatS = latS;
    //contenu.append("\nLatitude : ").append(cc.to//String()).append(" ").append(llatD.to//String()).append("d").append(llatM.to//String()).append("m").append(llatS.to//String()).append("s");
    double gpsLat= latD+(60.0*latM+latS)/3600.0;
    fprintf(stderr,"\n%2.4f Deg",gpsLat);
    sprintf(value, "%2.4f", gpsLat);
    SetConfigParam(PATH_LOG, "lat", value);
    //contenu.append(" = ").append(decform.format(gpsLat)).append(" Deg\n");
    if (s[71]=='0') c='E'; else c='W';
	i=72;
	//lonD=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
    lonD=calcul(i,i+7);
    //lonM=2*(16*(s[i+8]=='1')+8*(s[i+9]=='1')+4*(s[i+10]=='1')+2*(s[i+11]=='1')+(s[i+12]=='1'));
	lonM=2*calcul(i+8,i+12);
    i=120;
	//x=2*(s[i]=='1')+(s[i+1]=='1');
	x=calcul(i,i+1);
    if(s[119]=='0') {x=-x;}
    lonM+=x;
	i=122;
	//lonS=4*(8*(s[i]=='1')+4*(s[i+1]=='1')+2*(s[i+2]=='1')+(s[i+3]=='1'));
	lonS=4*calcul(i,i+3);
    if(s[119]=='0')// {lonS=-lonS;}
	        	{lonS=60-lonS;lonM--;
					if (lonM<0) {lonM=60+lonM;lonD--;}
				}


	//fprintf(stderr,"Longitude: ");fprintf(stderr,c);fprintf(stderr,' ');fprintf(stderr,lonD);fprintf(stderr,'d');fprintf(stderr,lonM);fprintf(stderr,'m');fprintf(stderr,lonS);fprintf(stderr,'s');
    fprintf(stderr,"\nLong: %c%dd%dm%ds ",c,lonD,lonM,lonS); //Charact ccc = c;//Integer llonD = lonD;//Integer llonM = lonM;//Integer llonS = lonS;
    //contenu.append("Longitude: ").append(ccc.to//String()).append(" ").append(llonD.to//String()).append("d").append(llonM.to//String()).append("m").append(llonS.to//String()).append("s");
    double gpsLon= lonD+(60.0*lonM+lonS)/3600.0;
    fprintf(stderr,"\n%3.4f Deg",gpsLon);
    sprintf(value, "%3.4f", gpsLon);
    SetConfigParam(PATH_LOG, "lon", value);
    //contenu.append(" = ").append(decform.format(gpsLon)).append(" Deg\n");
    GeogToUTM(gpsLat, gpsLon);
    i=126;
    //a=32*(s[i]=='0')+16*(s[i+1]=='0')+8*(s[i+2]=='0')+4*(s[i+3]=='0')+2*(s[i+4]=='0')+(s[i+5]=='0');
    a=calcul(i,i+5);
    //fprintf(stderr,"\nNational Use");
    //contenu.append("National Use\n");
  }
void identification_nationale()
	{int i;
	float a=1,x=0;
    int xx;
	for(i=57;i>39;i--)
		{if (s[i]=='1') x+=a;
                a*=2;
        }
	xx=(int)x;
	fprintf(stderr,"\nID Nat: ");
    //contenu.append("Identifiant National: ");
    //Integer xxx=xx;
    fprintf(stderr,"%d",xx);
    //contenu.append(xxx.to//String()).append("\n");
	}
void localisation_user()
	{char c; int i,latD,latM,lonD,lonM;
	if (s[107]=='0') c='N'; else c='S';
	i=108;
	//latD=64*(s[i]=='1')+32*(s[i+1]=='1')+16*(s[i+2]=='1')+8*(s[i+3]=='1')+4*(s[i+4]=='1')+2*(s[i+5]=='1')+(s[i+6]=='1');
    latD=calcul(i,i+6);
    //latM=4*(8*(s[i+7]=='1')+4*(s[i+8]=='1')+2*(s[i+9]=='1')+(s[i+10]=='1'));
	latM=4*calcul(i+7,i+10);
    //  fprintf(stderr,"Latitude: ");fprintf(stderr,c);fprintf(stderr,' ');fprintf(stderr,latD);fprintf(stderr,'d');fprintf(stderr,latM);fprintf(stderr,'m');
	fprintf(stderr,"\nLat: %c%dd%dm ",c,latD,latM);
    //Charact cc = c;//Integer llatD = latD;//Integer llatM = latM;
    //contenu.append("\nLatitude : ").append(cc.to//String()).append(" ").append(llatD.to//String()).append("d").append(llatM.to//String()).append("m");
    double gpsLat= latD+latM/60.0;
    fprintf(stderr,"\n%2.4f Deg",gpsLat);
    sprintf(value, "%2.4f", gpsLat);
    SetConfigParam(PATH_LOG, "lat", value);
    //contenu.append(" = ").append(decform.format(gpsLat)).append(" Deg\n");
    if (s[119]=='0') {c='E';} else {c='W';}
	i=120;
	//lonD=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
    lonD=calcul(i,i+7);
    //lonM=4*(8*(s[i+8]=='1')+4*(s[i+9]=='1')+2*(s[i+10]=='1')+(s[i+11]=='1'));
	lonM=4*calcul(i+8,i+11);
    //fprintf(stderr,"Longitude: ");fprintf(stderr,c);fprintf(stderr,' ');fprintf(stderr,lonD);fprintf(stderr,'d');fprintf(stderr,lonM);fprintf(stderr,'m');
	fprintf(stderr,"\nLong: %c%dd%dm ",c,lonD,lonM);
    //Charact ccc = c;//Integer llonD = lonD;//Integer llonM = lonM;
    //contenu.append("Longitude: ").append(ccc.to//String()).append(" ").append(llonD.to//String()).append("d").append(llonM.to//String()).append("m");
    double gpsLon= lonD+lonM/60.0;
    //fprintf(stderr," = %3.6f Deg",gpsLon);
    //contenu.append(" = ").append(decform.format(gpsLon)).append(" Deg\n");
    fprintf(stderr,"\n%3.4f Deg",gpsLon);
    sprintf(value, "%3.4f", gpsLon);
    SetConfigParam(PATH_LOG, "lon", value);
    GeogToUTM(gpsLat, gpsLon);
    if (s[106]=='1') {fprintf(stderr,"\nEncoded pos int");}
    //contenu.append("Encoded position data source internal\n");}
    }
void auxiliary_radio_locating_device_types()
    {int a;
  	//a=2*(s[83]=='1')+(s[84]=='1');
	a=calcul(83,84);
    switch(a){
      case 0 :fprintf(stderr,"\nNo Auxiliary radio-locating device");
          //contenu.append("No Auxiliary radio-locating device\n");
      break;
      case 1 :fprintf(stderr,"\n121.5MHz");
          //contenu.append("121.5MHz\n");
      break;
	  case 2 :fprintf(stderr,"\nMaritime locating 9GHz SART");
              //contenu.append("Maritime locating 9GHz SART\n");
          break;
	  case 3 :fprintf(stderr,"\nOther auxiliary radio-locating devices");
              //contenu.append("Other auxiliary radio-locating devices\n");
          break;
    }
     fprintf(stderr," %d",a);
 }

void Emergency_code_use()
{
     int a;
     if (s[106]=='1')
    { 	fprintf(stderr,"\nEmergency code flag :");
       ////contenu.append("\nEmergency code flag :");
       if (s[107]=='1')
    	{
    		fprintf(stderr,"\nAuto and manu");
    	}

	//a=8*(s[108]=='1')+4*(s[109]=='1')+2*(s[110]=='1')+(s[111]=='1');
        a=calcul(108,111);
	switch(a)
	{
       case 1 :fprintf(stderr,"Fire/explosion");
           //contenu.append("\nFire/explosion\n");
       break;
	   case 2 :fprintf(stderr,"Flooding");
               //contenu.append("\nFlooding\n");
           break;
	   case 3 :fprintf(stderr,"Collision");
               //contenu.append("\nCollision\n");
           break;
	   case 4 :fprintf(stderr,"Grounding");
               //contenu.append("\nGrounding\n");
           break;
	   case 5 :fprintf(stderr,"Listing, in danger of capsizing");//contenu.append("Listing, in danger of capsizing\n");
           break;
	   case 6 :fprintf(stderr,"Sinking");
               //contenu.append("\nSinking\n");
           break;
       case 7 :fprintf(stderr,"Disabled and adrift");
           //contenu.append("\nDisabled and adrift\n");
       break;
	   case 0 :fprintf(stderr,"Unspecified distress");
               //contenu.append("\nUnspecified distress\n");
           break;
	   case 8 :fprintf(stderr,"Abandoning ship");
               //contenu.append("\nAbandoning ship\n");
           break;
       case 10:fprintf(stderr,"Abandoning ship");
           //contenu.append("\nAbandoning ship\n");
       break;
    }
  }

}

void Non_Emergency_code_use_()
{

    // char a;

      if (s[106]=='1')
      {
      fprintf(stderr,"\nEmergency code flag: ");
          //contenu.append("\nEmergency code flag: ");
      }
       if (s[107]=='1')
       {
       	 fprintf(stderr,"\nAutomatic and manual");
           //contenu.append("Automatic and manual\n");
       }
       if (s[108]=='1')
       {
          fprintf(stderr,"fire");
           //contenu.append("fire\n");
       }
       if (s[109]=='1')
       {
         fprintf(stderr,"Medical help required");
           //contenu.append("Medical help required\n");
       }
       if (s[110]=='1')
       {
      //     fprintf(stderr,"disabled");//contenu.append("disabled\n");
       }
       if (s[111]=='1')
       {
           fprintf(stderr,"Non specifié");
           //contenu.append("Non specifié\n");
       }
  }





void Serial_Number_20_Bits()
{
  	    int j,e=1,a=0;
        fprintf(stderr,"\nSerial Number: ");
        //contenu.append("Serial Number: ");
        for(j=62;j>42;j--)
            {if(s[j]=='1')   a=a+e;
                e=e*2;
            }
        fprintf(stderr,"%d",a);
        //Integer aa=a;
        //contenu.append(aa.to//String()).append("\n");
}

/*void Serial_Number_20_Bits()
{
  	     	int i,a;
            fprintf(stderr,"\nSerial Number (2o bits) : ");

            i=43;
            //a=524288*(s[i]=='1')+262144*(s[i+1]=='1')+131072*(s[i+2]=='1')+65536*(s[i+3]=='1')+32768*(s[i+4]=='1')+16384*(s[i+5]=='1')+8192*(s[i+6]=='1')+4096*(s[i+7]=='1')+2048*(s[i+8]=='1')+1024*(s[i+9]=='1')+512*(s[i+10]=='1')+256*(s[i+11]=='1')+128*(s[i+12]=='1')+64*(s[i+13]=='1')+32*(s[i+14]=='1')+16*(s[i+15]=='1')+8*(s[i+16]=='1')+4*(s[i+17]=='1')+2*(s[i+18]=='1')+(s[i+19]=='1');
            a=calcul(i,i+19);
            fprintf(stderr,a);
}
*/
void all_0_or_nat_use()
{
  	int i,a;
    fprintf(stderr,"\nAll 0 or nat use: ");
    //contenu.append("All 0 or nat use: ");
    i=63;
    //a=512*(s[i]=='1')+256*(s[i+1]=='1')+128*(s[i+2]=='1')+64*(s[i+3]=='1')+32*(s[i+4]=='1')+16*(s[i+5]=='1')+8*(s[i+6]=='1')+4*(s[i+7]=='1')+2*(s[i+8]=='1')+(s[i+9]=='1');
    a=calcul(i,i+9);
    fprintf(stderr,"%d",a);
    //Integer aa=a;
    //contenu.append(aa.to//String()).append("\n");
}

void Aircraft_24_Bit_Adress()
{
            int i,j;
            int a;
            fprintf(stderr,"\n24 Bit Adresse: ");
            //contenu.append("Aircraft 24 Bit Adresse: ");

            for(j=0;j<3;j++)
			{
            i=43+j*8;
            //a=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
            a=calcul(i,i+7);
            envoi_byte(a);
            fprintf(stderr,"_");
            //contenu.append("_");
            }

      //fprintf(stderr,"\n");
      //contenu.append("\n");
}

void Additional_ELT_No()
{   int i;
    int a;
    //fprintf(stderr,"\nAdditional ELT No: ");
    //contenu.append("Additional ELT No: ");
    i=67;
    //a=32*(s[i]=='1')+16*(s[i+1]=='1')+8*(s[i+2]=='1')+4*(s[i+3]=='1')+2*(s[i+4]=='1')+(s[i+5]=='1');
    a=calcul(i,i+5);
    //fprintf(stderr,"%d",a);
    //Integer aa=a;
    //contenu.append(aa.to//String()).append("\n");
}

void Operator_3_Letter_Designator()
{  	        int i,j,b;
            int a;
            fprintf(stderr,"\nOperator 3 Letter Designator : ");
            //contenu.append("Operator 3 Letter Designator : ");
            for(j=0;j<2;j++)
			{
                 i=43+j*8;
                 //a=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
                a=calcul(i,i+7);
                envoi_byte(a);
            }
            i=59;
            //b=2*(s[i]=='1')+(s[i+1]=='1');
            b=calcul(i,i+1);
            envoi_byte(b);
            //fprintf(stderr,"\n");
            //contenu.append("\n");
}

void Serial_Number()
{   int i,a;
    fprintf(stderr,"\nSerial Number: ");
    //contenu.append("Serial Number: ");
    i=61;
    //a=2048*(s[i]=='1')+1024*(s[i+1]=='1')+512*(s[i+2]=='1')+256*(s[i+3]=='1')+128*(s[i+4]=='1')+64*(s[i+5]=='1')+32*(s[i+6]=='1')+16*(s[i+7]=='1')+8*(s[i+8]=='1')+4*(s[i+9]=='1')+2*(s[i+10]=='1')+(s[i+11]=='1');
    a=calcul(i,i+11);
    //Integer aa=a;
    fprintf(stderr,"%d",a);
    //contenu.append(aa.to//String()).append("\n");
}

 void C_S_Cert_No_or_Nat_Use()
 {  int a;
    fprintf(stderr,"\nC/S Num or Nat:");
    //contenu.append("C/S Number or National: ");
    // a=512*(s[73]=='1')+256*(s[74]=='1')+128*(s[75]=='1')+64*(s[76]=='1')+32*(s[77]=='1')+16*(s[78]=='1')+8*(s[79]=='1')+4*(s[80]=='1')+2*(s[81]=='1')+(s[82]=='1');
    a=calcul(73,82);
    fprintf(stderr,"\n%d",a);
    //Integer aa=a;
    //contenu.append(aa.to//String()).append("\n");
}
void affiche_serial_user()
	{ int a,j,i;
	//a=4*(s[39]=='1')+2*(s[40]=='1')+(s[41]=='1');
	a=calcul(39,41);
    switch(a)
		{case 0 :fprintf(stderr,"\nELT serial number: ");
            //contenu.append("ELT serial number: ");
            break;
		case 3 :fprintf(stderr,"\n24 bits adress: ");
            //contenu.append("ELT aircraft 24 bits adress: ");
            break;
		case 1 :fprintf(stderr,"\nELT aircraft operator designator and serial number: ");
            //contenu.append("ELT aircraft operator designator and serial number: ");
            break;
		case 2 :fprintf(stderr,"\nFloat free EPIRB + serial number : ");
            //contenu.append("Float free EPIRB + serial number: ");
            break;
		case 4 :fprintf(stderr,"\nNon float free EPIRB + serial number: ");
            //contenu.append("Non float free EPIRB + serial number: ");
            break;
		case 6 :fprintf(stderr,"\nPersonal Locator Beacon (PLB) + serial number: ");
            //contenu.append("Personal Locator Beacon (PLB) + serial number: ");
            break;
		}
		fprintf(stderr,"  bits 44 à 73 en hexa : ");
        //contenu.append("  bits 44 à 73 en hexa : ");
		for(j=0;j<4;j++)
			{i=43+j*8;
			//a=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
			a=calcul(i,i+7);
            envoi_byte(a);
			}
        // fprintf(stderr,"\n");
        //contenu.append("\n");
	}

void affiche_serial_user_1()
	{ int a;
	//a=4*(s[39]=='1')+2*(s[40]=='1')+(s[41]=='1');
	a=calcul(39,41);
        switch(a)
		{case 0 :fprintf(stderr,"\nELT S/N :");
                 Serial_Number_20_Bits();all_0_or_nat_use();C_S_Cert_No_or_Nat_Use();break;
		case 3 :fprintf(stderr,"\nELT aircraft ");
                Aircraft_24_Bit_Adress();Additional_ELT_No();C_S_Cert_No_or_Nat_Use();break;
		case 1 :fprintf(stderr,"\nELT aircraft operator designator and serial number : " );
                Operator_3_Letter_Designator();Serial_Number();C_S_Cert_No_or_Nat_Use();break;
		case 2 :fprintf(stderr,"\nfloat free EPIRB + serial number :");
                Serial_Number_20_Bits();all_0_or_nat_use();C_S_Cert_No_or_Nat_Use();break;
		case 4 :fprintf(stderr,"\nnon float free EPIRB + serial number :");
                Serial_Number_20_Bits();all_0_or_nat_use();C_S_Cert_No_or_Nat_Use();break;
		case 6 :fprintf(stderr,"\nPersonal Locator Beacon (PLB) + serial number :");
                Serial_Number_20_Bits();all_0_or_nat_use();C_S_Cert_No_or_Nat_Use();break;
		//case 5 :fprintf(stderr,"spare :");break;
         //case 7 :fprintf(stderr,spare :");break;

       }
    switch(a)
		{case 5 :case 7 :
          fprintf(stderr,"Spare: ");
          //contenu.append("Spare: \n");
          if(s[42]=='0')
          {
            //fprintf(stderr,"Serial identification number is assigned nationally ");
              //contenu.append("Serial identification number is assigned nationally \n");
          }
          else
          {
            //fprintf(stderr,"Identification data include the C/S type approval certificate number ");
            //contenu.append("Identification data include the C/S type approval certificate number\n");
          }
        }
}

void test_beacon_data()
{   int i,j;
    int a;
   	for(j=0;j<6;j++)
            {
             i=39+j*8;
            //a=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
            a=calcul(i,i+7);
            envoi_byte(a);
            }
    //     i=79;
    //a=32*(s[i]=='1')+16*(s[i+1]=='1')+8*(s[i+2]=='1')+4*(s[i+3]=='1')+2*(s[i+4]=='1')+(s[i+5]=='1');
    //      a=calcul(i,i+5);
    //Integer aa=a;
    //      fprintf(stderr,"%d",a);
    //contenu.append(aa.to//String()).append("\n");
}

void orbitography_data()
{
    int i,j;
    int a;
   	for(j=0;j<5;j++)
			{
            i=39+j*8;
			//a=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
			a=calcul(i,i+7);
            envoi_byte(a);
			}
             i=79;
             //a=32*(s[i]=='1')+16*(s[i+1]=='1')+8*(s[i+2]=='1')+4*(s[i+3]=='1')+2*(s[i+4]=='1')+(s[i+5]=='1');
             a=calcul(i,i+5);
             //Integer aa=a;
             fprintf(stderr,"%d",a);
             //contenu.append(aa.to//String()).append("\n");
}

void national_use()
{
    int i,j;
    int a;
   	for(j=0;j<5;j++)
            {
            i=39+j*8;
	        //a=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
            a=calcul(i,i+7);
            envoi_byte(a);//envoi_byte(a);
            }
    i=79;
    //a=32*(s[i]=='1')+16*(s[i+1]=='1')+8*(s[i+2]=='1')+4*(s[i+3]=='1')+2*(s[i+4]=='1')+(s[i+5]=='1');
    a=calcul(i,i+5);
    //Integer aa=a;
    fprintf(stderr,"%d",a);
    //contenu.append(aa.to//String());
    i=106;
    //a=32*(s[i]=='1')+16*(s[i+1]=='1')+8*(s[i+2]=='1')+4*(s[i+3]=='1')+2*(s[i+4]=='1')+(s[i+5]=='1');
    a=calcul(i,i+5);
    //Integer bb=a;
    fprintf(stderr,"%d",a);
    //contenu.append(bb.to//String()).append("\n");
}

void national_use_1()
{
    int i,j;
    int a;
   	for(j=0;j<5;j++)
			{
            i=39+j*8;
			//a=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
            a=calcul(i,i+7);
            envoi_byte(a);
			}
    i=79;
    //a=32*(s[i]=='1')+16*(s[i+1]=='1')+8*(s[i+2]=='1')+4*(s[i+3]=='1')+2*(s[i+4]=='1')+(s[i+5]=='1');
    a=calcul(i,i+5);
    //Integer aa=a;
    fprintf(stderr,"%d",a);
    //contenu.append(aa.to//String()).append("\n");
}

/*void BCH_1()
{

 int i,j;
     int a;
     fprintf(stderr,"\n Encoded BCH 1 ");  //contenu.append("Encoded BCH 1 \n");
   	for(j=0;j<2;j++)
			{
                        i=85+j*8;
			//a=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
			a=calcul(i,i+7);
                        envoi_byte(a);
			}
             i=101;
             //a=16*(s[i]=='1')+8*(s[i+1]=='1')+4*(s[i+2]=='1')+2*(s[i+3]=='1')+(s[i+4]=='1');
            a=calcul(i,i+4);
            envoi_byte(a);
 }*/


/*void affiche_serial_user2()
{
	 int a;

	//a=8*(s[39]=='1')+4*(s[40]=='1')+2*(s[41]=='1')+(s[42]=='1');
	a=calcul(39,42);
        switch(a)
	{
		case 10 :fprintf(stderr,"\nserial identification number is assigned nationaly ");break;
		 case 11 :fprintf(stderr,"\nidentification data include the C/S type approval certificate number");break;
         case 14 :fprintf(stderr,"\nserial identification number is assigned nationaly ");break;
		 case 15 :fprintf(stderr,"\nidentification data include the C/S type approval certificate number");break;
	}
}*/

void decodage_LCD()
{   int b;
	int i,j;
    int a;
    for(j=0;j<2;j++) //bits de 0 à 15 en octet
                    {i=j*8;
                    //a=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
                    a=calcul(i,i+7);
                   // envoi_byte(a);
                    }

    if(s[16]=='1')
                {
                //a=128*(s[16]=='1')+64*(s[17]=='1')+32*(s[18]=='0')+16*(s[19]=='1')+8*(s[20]=='0')+4*(s[21]=='0')+2*(s[22]=='0')+(s[23]=='0');
                a=calcul(16,23);
                fprintf(stderr,"\nTrame test...");
                SetConfigParam(PATH_LOG, "trame", "Trame de test...");
                //contenu.append("\nTEST...\n");
                }
            else
                {
                //a=128*(s[16]=='0')+64*(s[17]=='0')+32*(s[18]=='1')+16*(s[19]=='0')+8*(s[20]=='1')+4*(s[21]=='1')+2*(s[22]=='1')+(s[23]=='1');
                a=calcul(16,23);
                fprintf(stderr,"\nTrame d'alerte!");
                SetConfigParam(PATH_LOG, "trame", "Trame d'alerte !");
                //contenu.append("\nALERTE...\n");
                }
            //strcpy(chaine,"");
            if (s[24]=='0')
                {//fprintf(stderr,"\nUser Protocole-Localisation courte");
                //contenu.append("User Protocole/Localisation courte\n");
	            //code protocole 37-39
                //a=4*(s[36]=='1')+2*(s[37]=='1')+(s[38]=='1');
				a=calcul(36,38);
                switch(a)
					{
						case 2 :
							//fprintf(stderr,"\nEPIRB  MMSI/Radio: ");
                            //contenu.append("EPIRB  MMSI/Radio: ");
                            //affiche_baudot42();specific_beacon();Emergency_code_use();
                            break;
						case 6 :
							//fprintf(stderr,"\nEPIRB  Radio: ");
                            //contenu.append("EPIRB  Radio: ");
							//affiche_baudot_1();specific_beacon();Non_Emergency_code_use_();
							break;
						case 1 :
							//fprintf(stderr,"\nELT Aviation: ");
                            //contenu.append("ELT Aviation: ");
							//affiche_baudot_2();Non_Emergency_code_use_();
							break;
						case 3 ://fprintf(stderr,"\nSerial User : ");
							//affiche_serial_user_1();Non_Emergency_code_use_();
							break;
						case 7 :
                            //fprintf(stderr,"\nTest User: ");
                            //contenu.append("\nTest User: ");
                            //test_beacon_data();
                            break;
						case 0 :
							fprintf(stderr,"\nOrbitography: ");
                            //contenu.append("Orbitography: ");
                            //orbitography_data();
                            break;
						case 4 :
                            //fprintf(stderr,"National: ");
                            //contenu.append("National: ");
							//national_use();
							break;
						case 5 :
							fprintf(stderr,"Spare ");
                            //contenu.append("Spare ");
                            break;
					}

                switch(a)
					{
						case 2: case 6: case 1: case 3: //auxiliary_radio_locating_device_types();/*BCH_1();*/
                        break;
					}
                //b=512*(s[26]=='1')+256*(s[27]=='1')+128*(s[28]=='1')+64*(s[29]=='1')+32*(s[30]=='1')+16*(s[31]=='1')+8*(s[32]=='1')+4*(s[33]=='1')+2*(s[34]=='1')+(s[35]=='1');
                //b=calcul(26,35);
                //Integer bb=b;
                //fprintf(stderr,"\nCode Pays : %d\n",b);
                //contenu.append("Code Pays:").append(bb.to//String());

			}
        else //Trame longue
               {//fprintf(stderr,"\nTrame longue 144 bits");
               //contenu.append("Trame longue 144 bits\n");//Messages long
	        	if (s[25]=='0')
                    {//fprintf(stderr,"\nProtocole Standard ou National de Localisation");
                    //code protocole 37-40
                    //a=8*(s[36]=='1').toint())+4*(s[37]=='1').toint()+2*(s[38]=='1')+(s[39]=='1');
                    a=calcul(36,39);
                    switch(a)
					{
						case 2 :fprintf(stderr,"\nEPIRB MMSI");
                                //contenu.append("EPIRB MMSI\n");
                                break;
						case 3 :fprintf(stderr,"\nELT Location");
                                //contenu.append("ELT Location\n");
                                break;
						case 4 :fprintf(stderr,"\nELT serial");
                                //contenu.append("ELT serial\n");
                                break;
						case 5 :fprintf(stderr,"\nELT aircraft");
                                //contenu.append("ELT aircraft\n");
                                break;
						case 6 :fprintf(stderr,"\nEPIRB serial");
                                //contenu.append("EPIRB serial\n");
                                break;
						case 7 :fprintf(stderr,"\nPLB serial");
                                //contenu.append("PLB serial\n");
                                break;
						case 12 :fprintf(stderr,"\nShip Security");
                                //contenu.append("Ship Security\n");
                                break;
						case 8 :fprintf(stderr,"\nELT National");
                                //contenu.append("ELT National\n");
                                break;
						case 9 :fprintf(stderr,"\nSpare National");
                                //contenu.append("Spare National\n");
                                break;
						case 10 :fprintf(stderr,"\nEPIRB National");
                                //contenu.append("EPIRB National\n");
                                break;
						case 11 :fprintf(stderr,"\nPLB National");
                                //contenu.append("PLB National\n");
                                break;
						case 14 :fprintf(stderr,"\nStandard Test");
                                //contenu.append("Standard Test: ");
                                //standard_test();
                                break;
						case 15 :fprintf(stderr,"\nNational Test");
                                //contenu.append("National Test ");break;
						case 0 :
						case 1: //fprintf(stderr,"\nOrbitography");
                                //contenu.append("Orbitography\n");
                                break;
						case 13 :fprintf(stderr,"\nSpare....");
                                //contenu.append("Spare....\n");
                                break;
					}
					switch(a)
                                {case 2:
                                            //identification_MMSI();
                                            localisation_standard();
                                            //supplementary_data();
                                            break;
                                 case 3:
                                            identification_AIRCRAFT_24_BIT_ADRESS();
                                            //localisation_standard();
                                            //supplementary_data();
                                            break;
                                 case 4:	case 6 :case 7:
                                              //identification_C_S_TA_No();
                                              localisation_standard();
                                              //supplementary_data();
                                              break;
                                 case 5:
                                            //identification_AIRCRAFT_OPER_DESIGNATOR();
                                            localisation_standard();
                                            //supplementary_data();
                                            break;
                                 case 14:
                                            localisation_standard(); break;
                                 case 12:
                                               localisation_standard();
                                               //supplementary_data();
                                               //identification_MMSI_FIXED();
                                                break;
                                 case 8:case 9: case 10: case 11: case 15:
                                               localisation_nationale();
                                               identification_nationale();
                                               supplementary_data_1();
                                               break;
                                }
                    }
				else
				{
					//fprintf(stderr,"\nUser Protocole-Loc");
                    //contenu.append("User Protocole/Localisation\n");
                    //code protocole 37-39
					//a=4*(s[36]=='1')+2*(s[37]=='1')+(s[38]=='1');
					a=calcul(36,38);
                    switch(a)
                        {
						case 2 :fprintf(stderr,"\nEPIR MMSI/Radio :");
                        //contenu.append("EPIR MMSI/Radio\n");
						affiche_baudot42();specific_beacon();break;
						case 6 :fprintf(stderr,"\nEPIRB Radio..");
                        //contenu.append("EPIRB Radio..\n");
						affiche_baudot_1();specific_beacon();
						break;
						case 1 :fprintf(stderr,"\nELT Aviation: ");
                        //contenu.append("ELT Aviation: ");
						affiche_baudot_2();
						break;
						case 3 ://fprintf(stderr,"\nSerial User: ");
                        //contenu.append("Serial User: \n");
						//affiche_serial_user_1();
                        break;
						case 7 :fprintf(stderr,"\nTest User: ");
                        //contenu.append("Test User: \n");
                        test_beacon_data();
                        break;
						case 0 :fprintf(stderr,"\nOrbitography: ");
                        //contenu.append("Orbitography: \n");
                        orbitography_data();
                        break;
						case 4 :fprintf(stderr,"\nNational use: ");
                        //contenu.append("National use: \n");
                        national_use_1();
						break;
						case 5 :fprintf(stderr,"\nSpare");
                        //contenu.append("Spare\n");
                        break;
					 }
					 switch(a)
					{
						case 2: case 6: case 1: case 3:
                        localisation_user();
                        auxiliary_radio_locating_device_types();
                        break;
					}

				}
                //b=512*(s[26]=='1')+256*(s[27]=='1')+128*(s[28]=='1')+64*(s[29]=='1')+32*(s[30]=='1')+16*(s[31]=='1')+8*(s[32]=='1')+4*(s[33]=='1')+2*(s[34]=='1')+(s[35]=='1');
                b=calcul(26,35);
                //Integer bb=b;
                fprintf(stderr,"\nCode Pays : %d\n",b);
                //contenu.append("Code Pays : ").append(bb.to//String()).append("\n");
			}
}
//DecimalFormat decform = new DecimalFormat("###.######");


int lit_header(FILE *fp) {
    char txt[4+1] = "";
    char dat[4]="";
    int byte, j=0;
    if (fread(txt, 1, 4, fp) < 4) return -1;
    if (strncmp(txt, "RIFF", 4)) return -1;
    if (fread(txt, 1, 4, fp) < 4) return -1;
    if (fread(txt, 1, 4, fp) < 4) return -1;
    if (strncmp(txt, "WAVE", 4)) return -1;
    while(1)//rechercher "fmt "
        {if ( (byte=fgetc(fp)) == EOF ) return -1;
        txt[j % 4] = byte;
        j++; if (j==4) j=0;
        if (strncmp(txt, "fmt ", 4) == 0) break;
        }
    if (fread(dat, 1, 4, fp) < 4) return -1;
    if (fread(dat, 1, 2, fp) < 2) return -1;
    if (fread(dat, 1, 2, fp) < 2) return -1;
    N_canaux = dat[0] + (dat[1] << 8);
    if (fread(dat, 1, 4, fp) < 4) return -1;
    memcpy(&f_ech, dat, 4);
    if (fread(dat, 1, 4, fp) < 4) return -1;
    if (fread(dat, 1, 2, fp) < 2) return -1;
    if (fread(dat, 1, 2, fp) < 2) return -1;
    bits = dat[0] + (dat[1] << 8);
    while(1) //debut des donnees
        {if ( (byte=fgetc(fp)) == EOF ) return -1;
        txt[j % 4] = byte;
        j++; if (j==4) j=0;
        if (strncmp(txt, "data",4) == 0) break;
        }
    if (fread(dat, 1, 4, fp) < 4) return -1;
    //fprintf(stderr, "f_ech: %d\n", f_ech);
    //fprintf(stderr, "bits       : %d\n", bits);
    //fprintf(stderr, "N_canaux   : %d\n", N_canaux);
    if ((bits != 8) && (bits != 16)) return -1;
    ech_par_bit = f_ech/bauds;
    //fprintf(stderr, "ech par bit: %.2f\n", ech_par_bit);
    //fprintf(stderr, "****Attente de Trames****");
    return 0;
}


int lit_ech(FILE *fp) {  // int 32 bit
    int byte,ech=0,i,s=0;     // EOF -> 0x1000000
   // static int Avg[10];
    //float ss=0;
    for (i = 0; i < N_canaux; i++) {//mono =0
        byte = fgetc(fp);
        if (byte == EOF) return 1000000;
        if (i == canal_audio) ech = byte;
        if (bits == 16)
        {   byte = fgetc(fp);
            if (byte == EOF) return 1000000;
            if (i == canal_audio) ech +=  byte << 8;
        }
    }
    if (bits ==  8)  s = (ech-128)*256;   // 8bit: 00..FF, centerpoint 0x80=128
    if (bits == 16)  s = (short)ech;
    n_ech++;
   // moyenne(s);
   return s;
}

int duree_entre_pics(FILE *fp, int *nbr) {
    int echantillon;
    float n;
    n = 0.0;
    if (seuil==seuilm){
        do{echantillon = lit_ech(fp);
            //fprintf(stderr,"\n%d",echantillon);
            if (echantillon == 1000000) return 1;
            n++;
         } while (echantillon < seuilM);
         seuil=seuilM;
    }
    else{
        do{
          echantillon = lit_ech(fp);
          //fprintf(stderr,"\n%d",echantillon);
          if (echantillon == 1000000) return 1;
          n++;
        }while (echantillon > seuilm);
        seuil=seuilm;
    }

    //l = (float)n / ech_par_bit;
     if  (n>32700) n=32700;
     *nbr = (int) n;
    return 0;
}




void affiche_hexa(){
    int j,i,a;
    fprintf(stderr,"\n");
    for(j=0;j<((longueur_trame-24)/8);j++){
        i=24+j*8;
        //a=128*(s[i]=='1')+64*(s[i+1]=='1')+32*(s[i+2]=='1')+16*(s[i+3]=='1')+8*(s[i+4]=='1')+4*(s[i+5]=='1')+2*(s[i+6]=='1')+(s[i+7]=='1');
        a=calcul(i,i+7);
        envoi_byte(a);
        //fprintf(stderr,"%X",a);
    }
    //fprintf(stderr,"\n");
}

void Date(void)
{
  FILE *fp;

  fp = popen("date", "r");

  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  while (fgets(now, 30, fp) != NULL)
  {
    //printf("%s", MyIP);
  }

  pclose(fp);

  now[strcspn(now, "\n")] = 0;

  SetConfigParam(PATH_LOG, "date", now);
}

int test_crc1(){
int g[]= {1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1};
int div[22] ;
int i,j,ss=0;
//pour adrasec: crc à 0 accepté
int zero=0;
for(i=85;i<106;i++) {if (s[i]=='1') zero++;}
//fin adrasec
i=24;
for(j=0;j<22;j++)
	{
        if (s[i+j]=='1'){div[j]=1;}
        else div[j]=0;
    	}
while(i<85)
	{for(j=0;j<22;j++)
		{div[j]=div[j] ^ g[j];
		}
	//décalage
	while ((div[0]==0) && (i<85))
		{for (j=0;j<21;j++)
			{div[j]=div[j+1];
			}
		if (i<84)
			{if(s[i+22]=='1')div[21]=1;
                        else div[21]=0;
			}
		i++;
		}
	}
for(j=0;j<22;j++)
		{ss+=div[j];
		}
if (ss==0)
{
  fprintf(stderr,"\nCRC1 OK");
  Date();
}
else
    {if((no_checksum) && (zero==0))
     {
       fprintf(stderr,"\nCRC1 0");
       ss=0;
       Date();
    }
    else fprintf(stderr,"\nCRC1 KO");
    }
return ss;
}

int test_crc2(){
int g[]= {1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1};
int div[13];
int i,j,ss=0;
//pour adrasec: crc à 0 accepté
int zero=0;
for(i=132;i<144;i++) {if (s[i]=='1') zero++;}
//fin adrasec
i=106;
for(j=0;j<13;j++)
	{
         if (s[i+j]=='1'){div[j]=1;}
        else div[j]=0;
	}
while(i<132)
	{for(j=0;j<13;j++)
		{div[j]=div[j] ^ g[j];
		}
	//décalage
	while ((div[0]==0) && (i<132))
		{for (j=0;j<12;j++)
			{div[j]=div[j+1];
			}
		if (i<131)
			{if(s[i+13]=='1') div[12]=1;
                         else   div[12]=0;
                        }
		i++;
                }
	}
for(j=0;j<13;j++)
		{ss+=div[j];
		}
if (ss==0)   fprintf(stderr,"\nCRC2 OK");
else
    {if((no_checksum) && (zero==0)) {fprintf(stderr,"\nCRC2 0");ss=0;}
    else fprintf(stderr,"\nCRC2 ERROR");
    }
return ss;
}
int test_trame()
{int  L=112;
   int crc1;
   int crc2=0; //112 ou 144
   crc1=test_crc1();
   if (s[24]=='1') {L=144;crc2=test_crc2();}
   if ((crc1!=0)||(crc2!=0)) L=0;
   return L;
}

int affiche_trame(){
    int r=0;
    s[longueur_trame]=0;
    if(test_crc1()==0){
    r=1;
    if (s[24]=='1') test_crc2();
    //fprintf(stderr,"\nNouvelle trame binaire: ");
    //fprintf(stderr,"\n%s",s);
    //test_crc1();
    //if (s[24]=='1') test_crc2();
    //fprintf(stderr,"\nContenu hex:");
    fprintf(stderr,"\nContenu :");
    //affiche_hexa();
    decodage_LCD();
    }
    return(r);
}
void affiche_aide(){
            fprintf(stderr,"\n/*---------Decodage sur PI de Balise 406 (V6.0 par F4EHY)-----------------*/");
            fprintf(stderr,"\n/*OPTIONS:                                                                */");
            fprintf(stderr,"\n/*--help aide                                                             */");
            fprintf(stderr,"\n/*--osm  affichage sur une carte OpenStreetMap                            */");
            fprintf(stderr,"\n/*--une_minute timeout de 55 secondes pour une utilisation avec scan406   */");
            fprintf(stderr,"\n/*--M1 à --M10  Max pour le calcul le seuil minimum (defaut --M3)         */");
            fprintf(stderr,"\n/*--2 à --100 coeff pour calculer le seuil Max/coeff (default --100)      */");
            fprintf(stderr,"\n/*------------------------------------------------------------------------*/");
            fprintf(stderr,"\n/*LANCEMENT DU PROGRAMME (exemples):                                      */");
            fprintf(stderr,"\n/*sox -t alsa default -t wav - lowpass 3000 highpass 10  2>/dev/null | ./dec406V6        */");
            fprintf(stderr,"\n/*sox -t alsa default -t wav - lowpass 3000 highpass 10  2>/dev/null | ./dec406V6 --osm  */");
            fprintf(stderr,"\n/*------------------------------------------------------------------------*/");
            fprintf(stderr,"\n/*SI 'sox' EST ABSENT:                                                    */");
            fprintf(stderr,"\n/*sudo apt-get install sox                                                */");
            fprintf(stderr,"\n/*------------------------------------------------------------------------*/\n");
}
int main(int argc, char **argv) {
    FILE *fp;       //stdin ou fichier wav
    char *nom;      //nom du fichier en argument
//    int nbr;        //nombre d'ech entre 2 pics
//    int pos=0;      //numero du bit 0 à 143 ou 0 à 111
//    int Delta=0;    //tolerance Nbr ech entre pics
    int Nb=0;       //durée du bit en ech
//    int Nb2=0;      //durée demi bit en ech
//    int bitsync=0;  // pour compter les bits de synchronisation  à 1
//    int syncOK=0;   // passe à 1 lorsque du premier '0' après les 15 '1'
//    int paire=0;    // pour ne pas considerer les pics d'horloge comme des bits (Manchester)
    int depart=0;   // pour détecter le début de synchronisation porteuse non modulee == silence
    int echantillon;
    int numBit=0;
	char etat='*';
	int cpte = 0;
//	int valide = 0;// pour les 160ms
//	int cptblanc = 0;
//	int Nblanc,Nb15;
  //  int trame_OK=0;     //pour mise au point
    int synchro=0;
    //int longueur_trame=144;ou 112
    double Y1=0.0;
    double Y[242];//tableau de 2*Nb ech  = 2*48000/400=240
    double Ymoy=0.0;
    double Max = 10e3;
	double Min = -Max;
    double max;
	double min;
    double coeff=100;//8
    double seuil0 = Min/coeff;
	double seuil1 = Max/coeff;
 //   int fin=0;
    int quitter=0;
    int i,j,k,l;
    int Nb15;
    clock_t t1,t2;
    double dt;
    double clk_tck= CLOCKS_PER_SEC;
    fp=NULL;
    nom = argv[0];  //nom du programme
    ++argv;
    while ((*argv) && (flux_wav)) {
        if   (strcmp(*argv, "--help") == 0){affiche_aide();return 0;}

        else if ( (strcmp(*argv, "--osm") == 0) ) opt_osm=1;
        else if ( (strcmp(*argv, "--no_checksum") == 0) ) no_checksum=1;
        else if ( (strcmp(*argv, "--2") == 0) ) coeff=2;
        else if ( (strcmp(*argv, "--3") == 0) ) coeff=3;
        else if ( (strcmp(*argv, "--4") == 0) ) coeff=4;
        else if ( (strcmp(*argv, "--5") == 0) ) coeff=5;
        else if ( (strcmp(*argv, "--10") == 0) ) coeff=10;
        else if ( (strcmp(*argv, "--20") == 0) ) coeff=20;
        else if ( (strcmp(*argv, "--30") == 0) ) coeff=30;
        else if ( (strcmp(*argv, "--40") == 0) ) coeff=40;
        else if ( (strcmp(*argv, "--50") == 0) ) coeff=50;
        else if ( (strcmp(*argv, "--60") == 0) ) coeff=60;
        else if ( (strcmp(*argv, "--70") == 0) ) coeff=70;
        else if ( (strcmp(*argv, "--80") == 0) ) coeff=80;
        else if ( (strcmp(*argv, "--90") == 0) ) coeff=90;
        else if ( (strcmp(*argv, "--100") == 0) ) coeff=100;
        else if ( (strcmp(*argv, "--M1") == 0) ) Max=10e1;
        else if ( (strcmp(*argv, "--M2") == 0) ) Max=10e2;
        else if ( (strcmp(*argv, "--M3") == 0) ) Max=10e3;
        else if ( (strcmp(*argv, "--M4") == 0) ) Max=10e4;
        else if ( (strcmp(*argv, "--M5") == 0) ) Max=10e5;
        else if ( (strcmp(*argv, "--M6") == 0) ) Max=10e6;
        else if ( (strcmp(*argv, "--M7") == 0) ) Max=10e7;
        else if ( (strcmp(*argv, "--M8") == 0) ) Max=10e8;
        else if ( (strcmp(*argv, "--M9") == 0) ) Max=10e9;
        else if ( (strcmp(*argv, "--M10") == 0) ) Max=10e10;
        else if ( (strcmp(*argv, "--une_minute") ==0) ) opt_minute=1;

        else {fp = fopen(*argv, "rb");
            if (fp == NULL) {fprintf(stderr, "%s Impossible d'ouvrir le fichier\n", *argv);return -1;
            }
            flux_wav = 0;       //fichier wav ok on n'utilise pas stdin
        }
        ++argv;
    }
    if (flux_wav)  fp = stdin;  // pas de fichier .wav utiliser le flux standard 'stdin'

    fprintf(stderr,"\nATTENTE TRAMES");
    time_t top=time(NULL);
    time_t Time;
    lit_header(fp);
 //   pos = 0;
//    Nb2=ech_par_bit/2;
    Nb=ech_par_bit;
 //   Delta=round(0.2*Nb);
    depart=1;
 //   Nblanc=(int)((double)(0.04*(double)f_ech));
    //java a adapter......
    l=0;
    while (quitter==0){
      max = Max;min = -max; seuil0 = min/coeff; seuil1 = max/coeff;longueur_trame=144;
      cpte=0;k=0;etat='-';
      numBit=0;synchro=0;depart=0;
      for (i=0;i<145;i++) {s[i]='-';}
      for (i=0;i<242;i++) {Y[i]=0.0;};
      k=0;
      t1=clock();
	  while (numBit<longueur_trame){ //144 ou 112 si trame courte construire la trame
        if (opt_minute==1){//timeout pour utilisation avec scan406
            t2=clock();
            dt=((double)(t2-t1))/clk_tck;
          //  fprintf(stderr,"dt= %lf \n ",dt);
            if (dt>55.0) {fprintf(stderr,"Plus de 55s\n");fclose(fp);return 0;}
        }

        //lire 1 ech et autocorrelation 2bits
        echantillon = lit_ech(fp); //fprintf(stderr,"\n%d",echantillon);
        if (echantillon == 1000000) {
            quitter=1;fprintf(stderr,"Fin de lecture wav\n");return 0;
        }
        l++;
        k=(k+1)%(2*Nb); //numéro de l'échantillon dans un tableau Y de 2bit
        Y[k]=echantillon;Y1=0.0;Ymoy=0.0;
        for (i=0;i<2*Nb;i++)
            {Ymoy=Ymoy+Y[i];
            }
        Ymoy=Ymoy/(2*Nb);
        for (i=0;i<Nb;i++)  //Autocorélation
            {j=(k+i+Nb)%(2*Nb);
            Y1=Y1+(Y[(k+i)%(2*Nb)]-Ymoy)*(Y[j]-Ymoy);
            }
        if(Y1>max){max=Y1;seuil1=max/coeff;}
        if(Y1<min){min=Y1;seuil0=min/coeff;}
        //fin autocorélation

        if (synchro==0){//attendre synchro 15 '1'
                if (depart==0){//attendre premier front montant (depart_15 = 1 lorsque trouvé)
                    if (Y1>seuil1) {
                        depart=1;//1er front trouvé
                    }
                    cpte=0;
                }
                else{//attendre front descendant en comptant les echs
                    cpte++;
                    if (Y1<seuil0){//front descendant trouvé tester les bits à 1 (de 10 à 20 ok)
                        Nb15=cpte/Nb;
                        if ((Nb15<16)&&(Nb15>11)){//syncho 15 trouvée
                            synchro=1;cpte=0;
                            for (i=0;i<15;i++) {
                                s[i]='1';numBit=15;etat='0';
                            }
                        }
                        else{//erreur synchro recommencer
                             cpte=0;depart=0;synchro=0;etat='-';numBit=0;

                        }
                    }


                }

                Time=time(NULL);
                if (((unsigned long)difftime(Time, top)) > 0.5)
                {
                  fprintf(stderr,"\nATTENTE TRAMES");
                  top=time(NULL);
                }


        }
        else{//synchro OK => Traitement
            cpte++;
            if (s[24]=='0') {
                    longueur_trame=112;
                    }
            if (Y1>seuil1){//front montant
                if (etat=='0')// si vient Y1 de changer compter les bits et les affecter
                                {etat='1';cpte-=Nb/2;
                                while ((cpte>0)&&(numBit<longueur_trame))
                                    {if (s[numBit-1]=='1') {s[numBit]='0';}
                                    else {s[numBit]='1';}
                                    numBit++;cpte-=Nb;
                                    }
                                cpte=0;
                                }//etat *

            }
            else{
                if (Y1<seuil0) {//front descendant
                if(etat=='1') //si Y1 vient de changer
                                {etat='0';cpte-=Nb/2;
                                while ((cpte>0)&&(numBit<149))
                                    {s[numBit]=s[numBit-1];
                                    numBit++;cpte-=Nb;
                                    }
                                cpte=0;
                                }
                }
                else{//entre les deux niveaux... ignore...

                }

            }

        }//fin du else synchro OK traitement
    }  //fin while (numBit<longueur_trame)
    //fprintf(stderr,"\n************Mini %d\tMaxi %d\tMoy%d\tf_ech%d********",Mini,Maxi,Moy,f_ech);
    if(affiche_trame()==1){
        //printf(TROUVE\n\n");
        if (opt_minute==1) {fclose(fp); return 0;
        }
    }
    else{
        printf("Erreur CRC\n");
        if (opt_minute==1) {fclose(fp);return 0;
        }
    }
    }
    fprintf(stderr, "\n");
    fclose(fp);
    return 0;
}
