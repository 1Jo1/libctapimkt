/*  mktlin.unx
    Copyright (C) Dr. Claudia Neumann

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this software; see the file COPYING.   If not, write to
  the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/).
 
   Contact: Dr. Claudia Neumann
            Herderstr. 7
            D - 26169 Friesoythe
                 
            dr. claudia.neumann@gmx.de

   This is a program to read the german "Krankenversichertenkarte" 
   (KVK)  und the german "elektronische Gesundheitskarte" (eGK) with card readers on
   the serial port by using the libctapi-mkt

   See documentations under http://www.kbv.de/ita/register_G.html and 
   http://www.gematik.de/upload/gematik_Qop_eGK_Spezifikation_Teil1_V1_1_0_Kommentare_4_1652.pdf 

   It works with:
     Kernel > 2.6.8
     tested on various card readers that are certified from the KBV with KVK and eGK.

   Report bugs or comments to:
        dr.claudia.neumann@gmx.de

   Known Bugs:
      none (hopefully) ;-)

*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <dlfcn.h>
#include <unistd.h>
#include "ctapimkt/ctapi.h"

typedef
char ( *FPtr_CT_init)  ( unsigned short  ctn,
                                  unsigned short  pn );

typedef 
char ( *FPtr_CT_data)  ( unsigned short  ctn,
                                    unsigned char  *dad,
                                    unsigned char  *sad,
                                    unsigned short  lenc,
                                    unsigned char  *command,
                                    unsigned short *lenr,
                                    unsigned char  *response );

typedef 
char ( *FPtr_CT_close)   ( unsigned short ctn );

typedef char ( *FPtr_CT_setbaud) ( unsigned short ctn,
                                     int ct_baud );

/* STANDARD */
#define LIB_MKT "libctapi-mkt.so.1"    

/* Celectronic CardStar /medic2 USB */
#define LIB_CELECTRONIC "libctapi-celectronic.so.0"

/* Orga 6041 L und Orga 920 M */
#define LIB_ORGA "libCTORGT1.so.1"

/* Hypercom medCompact */
#define LIB_THALES "libcthyc-01.03.so" 

/* SCM eHealth200 */
#define LIB_SCM "libCT_eHealth100.so"  

/* Cherry ST-1503 und G87-1504 */
#define LIB_CHERRY "libctcym.so"

/* Hypercom medMobile (geht auch mit libctapi-mkt) */
#define LIB_HYC "libcthycmob.so"

/* Cherry ST-2052 */
#define LIB_PCSC1 "libmcscm.so"
#define LIB_PCSC2 "libpcsclite.so"
#define LIB_PCSC3 "libctpcsc.so"

/* Kobil KAAN Advanced, z.Zt. nur eGK */
#define LIB_KOBIL "libct.so"           

/* Eco 5044 */
#define LIB_ECO "libctdeuti.so"

void *Handle;

FPtr_CT_init    CT_INIT  = NULL;     /* Function pointer: call 'CT_INIT()'   */
FPtr_CT_data    CT_DATA  = NULL;     /* Function pointer: call 'CT_DATA()'   */
FPtr_CT_close   CT_CLOSE = NULL;     /* Function pointer: call 'CT_CLOSE()'  */
FPtr_CT_setbaud CT_SETBAUD = NULL;

#define FNAME_CT_INIT   "CT_init"
#define FNAME_CT_DATA   "CT_data"
#define FNAME_CT_CLOSE  "CT_close"
#define FNAME_CT_SETBAUD "CT_setbaud"

char rv;
unsigned short  ctn;
unsigned short  pn;
unsigned char   dad;
unsigned char   sad;
unsigned short  lenc;
unsigned char   command[127];
unsigned short  lenr;
unsigned char   response[2000];
char *lib_filename;
int ct_baud;
FILE *karte;
int tst=1;  // Alle Meldungen = 1



#define LOAD_LIB(LIB_NAME,Handle)                        \
{                 \
    Handle = dlopen( LIB_NAME, RTLD_LAZY );                   \
    if ( Handle == NULL )                                   \
     {                                                          \
      printf( "ERROR: Cannot load LIB: %s,  Programm aborts.\n" , LIB_NAME );                                  \
        EXIT(-1);                                               \
     }                                                          \
}

#define CONNECT_TO_LIB(LIB_NAME,CT_INIT,CT_DATA,CT_CLOSE) \
{                                                               \
    LOAD_LIB(LIB_NAME,Handle);           \
    if ( LIB_NAME == LIB_PCSC1 )   \
     {         \
    LOAD_LIB(LIB_PCSC2,Handle);     \
    LOAD_LIB(LIB_PCSC3,Handle);           \
     }          \
    if ( LIB_NAME == LIB_KOBIL )   \
     {         \
    LOAD_LIB(LIB_PCSC2,Handle);     \
    LOAD_LIB(LIB_PCSC3,Handle);           \
     }          \
    CT_INIT = (FPtr_CT_init) dlsym( Handle,        \
                                              FNAME_CT_INIT );  \
    if ( CT_INIT == NULL )                                     \
     {                                                          \
      printf( "ERROR: Invalid pointer to '%s'. Program aborts.\n", FNAME_CT_INIT );                                \
                EXIT(-1);                                               \
     }                                                          \
   CT_DATA = (FPtr_CT_data) dlsym( Handle,         \
                                             FNAME_CT_DATA );   \
   if ( CT_DATA == NULL )                                      \
    {                                                           \
      printf( "ERROR: Invalid pointer to %s. Program aborts.\n", FNAME_CT_DATA );                                \
                EXIT(-1);                                               \
    }                                                           \
   CT_CLOSE = (FPtr_CT_close) dlsym( Handle,       \
                                               FNAME_CT_CLOSE );\
   if ( CT_CLOSE == NULL )                                     \
    {                                                           \
      printf( "ERROR: Invalid pointer to %s. Program aborts.\n", FNAME_CT_CLOSE );          \
                EXIT(-1);                                               \
    }                                                           \
    if ( LIB_NAME == LIB_ORGA ) {              \
   CT_SETBAUD = (FPtr_CT_setbaud) dlsym( Handle,       \
                                               FNAME_CT_SETBAUD );\
   if ( CT_SETBAUD == NULL )                                     \
    {                                                           \
      printf( "ERROR: Invalid pointer to %s. Program aborts.\n", FNAME_CT_SETBAUD );          \
                EXIT(-1);                                               \
    }                                                           \
     }                                   \
}

#define CONNECT_TO_CT_SETBAUD(ctn,ct_baud)        \
{                                                         \
     rv = CT_SETBAUD( ctn, ct_baud );                    \
    if ( rv )                                   \
     {                                                          \
       printf( "ERROR: CT_setbaud-Error"                        \
                "Program aborts.\n", ctn, pn );                                          \
        PROGRAM_ABORT;                                          \
     }                                                          \
}

#define CONNECT_TO_CT_INIT(ctn,pn)                                   \
{                                                               \
     rv = CT_INIT( ctn, pn );                    \
    if ( rv )                                   \
     {                                                          \
       printf( "ERROR: Connection to CTN %2d via PN %2d to failed."                        \
                "Program aborts.\n", ctn, pn );                                          \
        PROGRAM_ABORT;                                          \
     }                                                          \
}

#define CONNECT_TO_CT_DATA( ctn, dad, sad, lenc, command, lenr, response)                                           \
{                                                               \
    /*    printf("dad = %d, sad = %d\n", dad, sad);    */           \
        rv = CT_DATA (  ctn, &dad, &sad, lenc, command, &lenr, response );              \
    /*    printf("dad = %d, sad = %d\n", dad, sad);    */           \
       if ( rv )                                \
        {                                                       \
          printf( "ERROR: %s() reports error : %2d\n", FNAME_CT_DATA, rv );                         \
        }                                      \
}


#define EXIT(exitcode)                                          \
{                                                               \
         if ( Handle )                                       \
         {                                                      \
            dlclose( Handle );                          \
            Handle = NULL;                                   \
         }                                                      \
         exit( exitcode );                                      \
}

#define PROGRAM_ABORT                                           \
{                                                               \
        fclose(karte);                                      \
        CT_CLOSE(ctn);                                          \
      EXIT(-1);                                               \
}


#define PROGRAM_TERMINATE                                       \
{                                                               \
        fclose(karte);                                     \
        CT_CLOSE(ctn);                                          \
         if ( Handle )                                       \
         {                                                      \
            dlclose( Handle );                          \
            Handle = NULL;                                   \
         }                                                      \
      return 0;                                               \
}


int antwort(unsigned char *abschnitt){
  int x;
  
  if (rv) {
    if (tst) {
      fprintf(stdout, "Fehler in (%s): (%d)\n", abschnitt, rv);
    }
    fprintf(karte, "Fehler in (%s)", abschnitt);
    PROGRAM_ABORT;
  }
  if (lenr<2) {
    if (tst) {
      fprintf(stdout, "(%s)-Antwort zu kurz: (%d)\n", abschnitt, lenr);
    }
    fprintf(karte, "(%s)-Antwort zu kurz");
    PROGRAM_ABORT;
  }
  if (tst) {
  printf("RSP: ");
  for(x=0;x<lenr;x++){
    printf("%02x ",response[x]);
   }
   printf("\n");
  }
 if ( (response[lenr-2]==0x64) && (response[lenr-1]==0xA1) ) {
    if (tst){
      fprintf(stdout, "Keine Karte vorhanden\n");
    }
    fprintf(karte, "Keine Karte vorhanden");
    PROGRAM_ABORT;
  }
  else if ( (response[lenr-2]==0x64) && (response[lenr-1]==0xA2) ) {
    if (tst){
      fprintf(stdout, "Keine Karte aktiviert\n");
    }
    fprintf(karte, "Keine Karte aktiviert");
    PROGRAM_ABORT;
  }
  else if ( (response[lenr-2]==0x6F) && (response[lenr-1]==0x00) ) {
    if (tst){
      fprintf(stdout, "Kommunikation zur Karte nicht moeglich\n");
    }
    fprintf(karte, "Kommunikation zur Karte nicht moeglich");
    PROGRAM_ABORT;
  }
  else {
    if ( (response[lenr-2]==0x67) && (response[lenr-1]==0x00) ) {
      if (tst){
        fprintf(stdout, "Falsche Laenge\n");
      }
      fprintf(karte, "šbertragungsfehler", abschnitt);
      PROGRAM_ABORT;
    }
    else if ( (response[lenr-2]==0x69) && (response[lenr-1]==0x00) && !(lib_filename==LIB_ORGA) ) {
      if (tst){
        fprintf(stdout, "(%s): Kommando nicht erlaubt\n", abschnitt);
      }
      fprintf(karte, "šbertragungsfehler", abschnitt);
      PROGRAM_ABORT;
    }
    else if ( (response[lenr-2]==0x6A) && (response[lenr-1]==0x00) ) {
      if (tst){
        fprintf(stdout, "Falsche Parameter P1 oder P2\n");
      }
      fprintf(karte, "šbertragungsfehler", abschnitt);
      PROGRAM_ABORT;
    }
    else if ( (response[lenr-2]==0x6C) && (response[lenr-1]==0x00) ) {
      if (tst){
        fprintf(stdout, "Falsche Laenge le\n");
      }
      fprintf(karte, "šbertragungsfehler", abschnitt);
      PROGRAM_ABORT;
    }
    else if ( (response[lenr-2]==0x6D) && (response[lenr-1]==0x00) ) {
      if (tst){
        fprintf(stdout, "Falsche Instruktion\n");
      }
      fprintf(karte, "šbertragungsfehler", abschnitt);
      PROGRAM_ABORT;
    }
    else if ( (response[lenr-2]==0x6E) && (response[lenr-1]==0x00) ) {
      if (tst){
        fprintf(stdout, "Falsche Instruktion\n");
      }
      fprintf(karte, "šbertragungsfehler", abschnitt);
      PROGRAM_ABORT;
    }
  }
return;
}





int main (int argc, char **argv) {
  unsigned char resetct[] = {0x20, 0x11, 0x00, 0x00, 0x00 };  
/*  unsigned char resetct[] = {0x20, 0x13, 0x00, 0x80, 0x00 };  */

  unsigned char requicc[7];
/* REQUEST ICC mit ATR Response und 1s Wartezeit                    */
/* | CLA | INS | P1 | P2 | lc | data | le | (ISO CASE 4 Short)      */
/* ----------------------------------------                         */
/* | 20  | 12  | 01 | 01 | 01 | 01   | 00 |                         */
/*    |       |        |     |      |      |        |--> max. 256 Byte Response */
/*    |       |        |     |      |      |---------> 1s warten              */
/*    |       |        |     |      |--------------> laenge des data Feldes */
/*    |       |        |     |-------------------> ATR Response           */
/*    |       |        |------------------------> FU = ICC 1 (SLOT 1)    */
/*    |       |------------------------------> REQUEST ICC            */
/*    |------------------------------------> MKT-BCS Kommando       */
  unsigned char requicc1[] = {0x20, 0x12, 0x01, 0x01, 0x01, 0x01, 0x00};

/* REQUEST ICC ohne ATR Response und 1s Wartezeit                   */
/* | CLA | INS | P1 | P2 | lc | data |      (ISO CASE 3 Short)      */
/* -----------------------------------                              */
/* | 20  | 12  | 01 | 00 | 01 | 01   |                              */
/*    |     |        |       |      |      |---------> 1s warten              */
/*    |     |        |       |      |--------------> laenge des data Feldes */
/*    |     |        |       |-------------------> ohne Response data     */
/*    |     |        |------------------------> FU = ICC 1 (SLOT 1)    */
/*    |     |------------------------------> REQUEST ICC            */
/*    |------------------------------------> MKT-BCS Kommando       */
  unsigned char requicc2[] = {0x20, 0x12, 0x01, 0x00, 0x01, 0x01} ; // nur damit l„uft IBM-Ger„t

  unsigned char requicc3[] = {0x20, 0x12, 0x01, 0x00, 0x01, 0x00} ; // Kobil?

  unsigned char selfile[] =  {0x00, 0xA4, 0x04, 0x00, 0x06, 0xD2, 0x76, 0x00, 0x00, 0x01, 0x01};
  unsigned char rbvd[] = {0x00, 0xB0, 0x8C, 0x00, 0x00};
  unsigned char rbkvk[] = {0x00, 0xB0, 0x00, 0x00, 0x00};
  unsigned char rbegk[] ={0x00, 0xB0, 0x81, 0x00, 0x00, 0x00, 0x00};
  unsigned char ejecticc[] = {0x20, 0x15, 0x01, 0x00, 0x01, 0x00};
//  unsigned char ejecticc[] = {0x20, 0x15, 0x04, 0x00, 0x01, 0x00};  // keine Meldung zur Kartenentnahme (Celectronic)
// Orga, Celectronic /memo2, Zemo: ejecticc[] = {0x20, 0x15, 0x01, 0x00, 0x01, 0x01}
  unsigned char cst[300];
  unsigned char cpd[850];
  unsigned char cvd[1250];
  unsigned char erase[] = {0x00, 0x0E, 0x00, 0x00};
  unsigned char cgetst[] = {0x20, 0x13, 0x00, 0x46, 0x00};
  unsigned char cifaceparam[] = {0x80, 0x60, 0x01, 0x00, 0x03, 0x45, 0x01 } ;
  unsigned char cauth[] = {0x20, 0x18, 0x00, 0x00, 0x00};


  int x , y, z, nl, nm, npd, nvd1, nvd2, ncard, mobil,nlib,lenegk,lenicc,baud;
  FILE *fz;


  if (argc<3) {
      fprintf(stdout,"richtiger Gebrauch: %s <Port-Nr> <Lib-Nr>\n", argv[0]);
      return 2;
  }
  ctn = atoi(argv[1]);
  nlib = atoi(argv[2]);

  karte=fopen( "karte.txt" , "w+b" );
  if (karte==NULL) {
      printf("Fehler beim Oeffnen der Datei karte.txt\n");
      }
  
  switch (nlib)
  {
   case 1:
     pn = 1;
     ctn = 0;
     lib_filename = LIB_CELECTRONIC;
     break;
   case 2:
     pn = ctn;
     lib_filename = LIB_ORGA;
     break;
   case 3:
     pn = 1;
     lib_filename = LIB_MKT;
     break;
   case 4:
     if (ctn==1) {
     pn = 1;
     }
     else {
     pn = 48;
     }
     lib_filename = LIB_THALES;
     break;
   case 5:
     pn = ctn;
     lib_filename = LIB_MKT;
     break;
   case 6:
     pn = ctn;
     lib_filename = LIB_SCM;
     break;
    case 7:
     pn = ctn;
     lib_filename = LIB_CHERRY;
     break;
    case 8:
     pn = ctn;
     lib_filename = LIB_HYC;
     break;
    case 9:
     pn = ctn;
     lib_filename = LIB_PCSC1;
     break;
    case 10:
     pn = ctn;
     lib_filename = LIB_KOBIL;
     break;
    case 11:
     pn = ctn;
     lib_filename = LIB_ECO;
     break;
   default:
     pn = 1;
     lib_filename = LIB_MKT;
     break;
  }
  CONNECT_TO_LIB(lib_filename,CT_INIT,CT_DATA,CT_CLOSE);
  if (tst) {
    fprintf(stdout, "%s geladen.\n",lib_filename);
  }
  if ( (nlib==2) && (ctn<5) ) {
        CONNECT_TO_CT_SETBAUD(ctn,115200);
  }

  CONNECT_TO_CT_INIT(ctn,pn);
  if (tst) {
     printf( "Opened connection to CTN #%2d via PN %2d.\n", ctn, pn );
  }


  if ( (argc>3) && (atoi(argv[3])) && !(nlib==8) && !( (nlib==3) && (ctn>10) ) ) {
  /***Erase Binary */
  dad=0;
  sad=2;
  if (tst) {
  printf("Erase Binary\n");
  printf("CMD: ");
  for(x=0;x<4;x++){
     printf("%02x ",erase[x]);
     }
    printf("\n");
  }
  lenr=sizeof(response);
  memset ( response, 0x00, lenr );
  CONNECT_TO_CT_DATA(ctn, dad, sad, 4, erase, lenr, response);
  antwort("Erase Binary");
  if ( (response[lenr-2]==0x90) && (response[lenr-1]==0x00) ) {
    if (tst) {
      fprintf(stdout, "Command successful\n");
    }
  }
  else {
    if ( (response[lenr-2]==0x69) && (response[lenr-1]==0x86) ) {
      if (tst){
        fprintf(stdout, "Keine Daten ausgewaehlt\n");
      }
    fprintf(karte, "Keine Daten geloescht");
    PROGRAM_ABORT;
    }
    if ( (response[lenr-2]==0x65) && (response[lenr-1]==0x00) ) {
      if (tst){
        fprintf(stdout, "Loeschen misslungen\n");
      }
    fprintf(karte, "Loeschen misslungen");
    PROGRAM_ABORT;
    }
    if ( (response[lenr-2]==0x6B) && (response[lenr-1]==0x00) ) {
      if (tst){
        fprintf(stdout, "Falscher Parameter\n");
      }
    fprintf(karte, "Loeschen misslungen");
    PROGRAM_ABORT;
    }
    else {
      fprintf(stdout, "Erase Binary misslungen\n");
      PROGRAM_ABORT;
    }
  }
  
  /***Eject ICC */
  dad=1;
  sad=2;
  if ( (nlib==1) || (nlib == 2) || (nlib == 3) ) {
     ejecticc[5]=0x01;
  } 
  if (tst) {
  printf("Eject ICC\n");
  printf("CMD: ");
  for(x=0;x<sizeof(ejecticc);x++){
     printf("%02x ",ejecticc[x]);
     }
  printf("\n");
  }
  lenr=sizeof(response);
  memset ( response, 0x00, lenr );
  CONNECT_TO_CT_DATA(ctn, dad, sad, sizeof(ejecticc), ejecticc, lenr, response);
  antwort("Eject ICC");
  if ( ( (response[lenr-2]==0x90) && (response[lenr-1]==0x00) ) || ( (response[lenr-2]==0x90) && (response[lenr-1]==0x01) ) || ( (response[lenr-2]==0x62) && (response[lenr-1]==0x00) ) ) {
    if (tst) {
      fprintf(stdout, "Command successful\n");
    }
  }
  else {
    if (tst) {
      fprintf(stdout, "Fehler bei Eject ICC\n");
    }
    PROGRAM_ABORT;
  }
  }


  else {

/***Reset CT  */ 
  dad=1;
  sad=2;
  if (tst) {
  printf("Reset CT\n");
  printf("CMD: ");
  for(x=0;x<5;x++){
     printf("%02x ",resetct[x]);
     }
    printf("\n");
  }
  lenr=sizeof(response);
  memset (response, 0x00, lenr);
  CONNECT_TO_CT_DATA(ctn, dad, sad, 5, resetct, lenr, response);
  antwort("Reset CT");
  if ( (response[lenr-2]==0x64) && (response[lenr-1]==0x00) ) {
    if (tst) {
      fprintf(stdout, "Reset CT misslungen\n");
    }
    fprintf(karte, "Reset CT misslungen");
    PROGRAM_ABORT;
  }
  if ( (response[lenr-2]==0x90) && (response[lenr-1]==0x00) ) {
    if (tst) {
      fprintf(stdout,"Reset CT erfolgreich, station„res Leseger„t\n");
    }
    mobil=0;
  }
  else if ( (response[lenr-2]==0x95) && (response[lenr-1]==0x00) ) {
    if (tst) {
      fprintf(stdout,"Reset CT erfolgreich, mobiles Lesegeraet\n");
    }
    mobil=1;
  }
  else {
    if (tst) {
      fprintf(stdout, "Reset CT misslungen\n");
    }
    fprintf(karte, "Reset CT misslungen\n");
    PROGRAM_ABORT;
  }

/***Set Interface Parameter */
/*  if (nlib==10){
  dad=1;
  sad=2;
  if (tst) {
  printf("Set Interface Parameter\n");
  printf("CMD: ");
  for(x=0;x<sizeof(cifaceparam);x++){
     printf("%02x ",cifaceparam[x]);
     }
    printf("\n");
  }
  lenr=sizeof(response);
  memset (response, 0x00, lenr);
  CONNECT_TO_CT_DATA(ctn, dad, sad, 5, cifaceparam, lenr, response);
  if (rv) {
    if (tst) {
      printf("Error in Reset CT: %d\n",rv);
    }
    fprintf(karte, "Error in Reset CT");
    PROGRAM_ABORT;
  }
  if (lenr<2) {
    if (tst) {
     printf("Response too short (Reset CT) (%d)\n", lenr);
    }
    fprintf(karte,"Response too short (Reset CT)");
    PROGRAM_ABORT;
  }
  if (tst) {
  printf("RSP: ");
  for(x=0;x<lenr;x++){
    printf("%02x ",response[x]);
     }
    printf("\n");
  }
   if ( (response[lenr-2]==0x64) && (response[lenr-1]==0x00) ) {
      fprintf(karte, "Reset CT misslungen");
      PROGRAM_ABORT;
    }
   if ( (response[lenr-2]==0x90) && (response[lenr-1]==0x00) ) {
      if (tst) {
      printf( "Reset CT erfolgreich\n");
      }
      mobil=0;
    }
   else if ( (response[lenr-2]==0x95) && (response[lenr-1]==0x00) ) {
      if (tst) {
      printf( "Reset CT erfolgreich, mobiles Lesegeraet\n");
      }
      mobil=1;
    }
    else {
      fprintf(karte, "Set Interface Parameter misslungen\n");
      PROGRAM_ABORT;
    }
    } */

/***GetStatusCTMDO***/
if ( (mobil) &&  (nlib==8) ){
// for(y=0;y<50;y++){
  dad=1;
  sad=2;
  if (tst) {
  printf("GetStatusCTMDO\n");
  printf("CMD: ");
  for(x=0;x<5;x++){
     printf("%02x ",cgetst[x]);
     }
    printf("\n");
  }
  lenr=sizeof(response);
  memset ( response, 0x00, lenr );
  CONNECT_TO_CT_DATA(ctn, dad, sad, 5, cgetst, lenr, response);
  antwort("GetStatusCTMDO");
  if (response[25]==0x00) {
      fprintf(karte,"Bitte PIN eingeben!");
      dad=1;
      sad=2;
      if (tst) {
	printf("Authentifizierung\n");
        printf("CMD: ");
        for(x=0;x<5;x++){
           printf("%02x ",cauth[x]);
         }
         printf("\n");
       }
       lenr=sizeof(response);
       memset ( response, 0x00, lenr );
       CONNECT_TO_CT_DATA(ctn, dad, sad, 5, cauth, lenr, response);
       antwort("Authentifizierung");
//       sleep (10);
       PROGRAM_ABORT;
  }
//  else {
//    break;
//  }   
//} 
}

/***Request ICC */
  dad=1;
  sad=2;
  if (nlib==5) {
    for(x=0;x<sizeof(requicc1);x++){
      requicc[x]=requicc1[x];
     }
     lenicc=sizeof(requicc);
     }
   else if  (nlib==10) {
    for(x=0;x<sizeof(requicc3);x++){
      requicc[x]=requicc3[x];
     }
     lenicc=sizeof(requicc3);
    }
   else if ( (nlib==3) && (ctn==13) ) {
    for(x=0;x<sizeof(requicc1);x++){
      requicc[x]=requicc1[x];
     }
     lenicc=sizeof(requicc);
     }
   else {
    for(x=0;x<sizeof(requicc2);x++){
      requicc[x]=requicc2[x];
     }
     lenicc=sizeof(requicc2);
    } 
  if (tst) {
  printf("Request ICC\n");
  printf("CMD: ");
  for(x=0;x<lenicc;x++){
     printf("%02x ",requicc[x]);
     }
    printf("\n");
  }
  lenr=sizeof(response);
  memset (response, 0x00, lenr);
  CONNECT_TO_CT_DATA(ctn, dad, sad, lenicc, requicc, lenr, response);
  antwort("Request ICC");
  if ( (response[lenr-2]==0x62) && (response[lenr-1]==0x01) ) {
    if (tst) {
      fprintf(stdout, "Karte muss neu gesteckt werden\n");
    }
    fprintf(karte, "Karte muss neu gesteckt werden");
    PROGRAM_ABORT;
    }
  if ( (response[lenr-2]==0x62) && (response[lenr-1]==0x00) ) {
    if (mobil) {
      if ( (nlib == 1) || (nlib == 3) ) {
        if (tst) {
          fprintf(stdout, "Lesegeraet aufschliessen oder keine Daten vorhanden!\n");
        }
        fprintf(karte, "Lesegeraet aufschliessen oder keine Daten vorhanden!");
      }
      if ( (nlib == 2) || (nlib == 8) ){
        if (tst) {
          fprintf(stdout, "keine Daten vorhanden!\n");
        }
        fprintf(karte, "keine Daten vorhanden!");
      }
    }
    else {	    
      if (tst) {
          fprintf(stdout, "keine Karte gelesen, Lese-Timeout!\n");
      }
      fprintf(karte, "Keine Karte gelesen, Lese-Timeout");
    }
    PROGRAM_ABORT;
  }
  if ( (response[lenr-2]==0x69) && (response[lenr-1]==0x00) ) {
    if ((mobil) && (nlib==2)){
      if (tst) {
        fprintf(stdout, "Lesegeraet aufschliessen!\n");
      }
      fprintf(karte, "Lesegeraet aufschliessen!");
    }
    else {
      if (tst) {
        fprintf(stdout, "Request ICC: Kommando nicht erlaubt\n");
      }
      fprintf(karte, "Request ICC: nicht erlaubtes Kommando");
    }
    PROGRAM_ABORT;
  }	  
  if ( (response[lenr-2]==0x64) && (response[lenr-1]==0x00) ) {
    if (tst) {
      fprintf(stdout, "Karte fehlerhaft oder falsch gesteckt\n");
    }
    fprintf(karte, "Karte fehlerhaft oder falsch gesteckt");
    PROGRAM_ABORT;
    }
   if (response[lenr-2]==0x90) {
     if (response[lenr-1]==0x00) {
     if (mobil) {
      fprintf(karte, "KVK mobil");
     }
    else {
         fprintf(karte, "KVK");
       }
       ncard=1;
      }
     if (response[lenr-1]==0x01) {
     if (mobil) {
      fprintf(karte, "eGK mobil");
      }
     else {
       fprintf(karte, "eGK");
      }
       ncard=2;
      }
    }
   else {
    if (tst) {
      fprintf(stdout, "Request ICC misslungen\n");
    }
    fprintf(karte, "Request ICC misslungen");
    PROGRAM_ABORT;
  }

 
/***Select File */
  dad=0;
  sad=2;
  if (ncard==2) {
    selfile[3]=0x0C;
    selfile[10]=0x02;
    }
  if (tst) {
  printf("Select File\n");
  printf("CMD: ");
  for(x=0;x<11;x++){
     printf("%02x ",selfile[x]);
     }
    printf("\n");
    }
   lenr=sizeof(response);
  memset (response, 0x00, lenr);
  CONNECT_TO_CT_DATA(ctn, dad, sad, 11, selfile, lenr, response);
  antwort("Select File");
  if ( (response[lenr-2]==0x6A) && (response[lenr-1]==0x82) ) {
    if (tst) {
     fprintf(stdout, "Select File: Application not found\n");
    }
    fprintf(karte, "Select File: Karte nicht lesbar");
    PROGRAM_ABORT;
   }
   if ( (response[lenr-2]==0x90) && (response[lenr-1]==0x00) ) {
     if (tst) {
       fprintf(stdout, "Command successful\n");
     }
   }
   else {
    if (tst) {
      fprintf(stdout, "Select File: unbekannter Fehler\n");
    }
    fprintf(karte, "Select File: unbekannter Fehler");
    PROGRAM_ABORT;
   }


/***Read Binary */
  dad=0;
  sad=2;
  if (ncard==1) {    // KVK
    if (tst) {
    printf("Read Binary\n");
    printf("CMD: ");
    for(x=0;x<5;x++){
     printf("%02x ",rbkvk[x]);
     }
    printf("\n");
    }
    lenr=sizeof(response);
    memset (response, 0x00, lenr);
    CONNECT_TO_CT_DATA(ctn, dad, sad, 5, rbkvk, lenr, response);
    antwort("Read Binary");
    if ((mobil) && (nlib==2) && (response[lenr-2]==0x69) && (response[lenr-1]==0x00) ) {
      if (tst) {
        fprintf(stdout,"Lesegeraet aufschliessen!\n");
      }
      fprintf(karte, "/nLesegeraet aufschliessen!");
      PROGRAM_ABORT;
    }	  
    if ( (response[lenr-2]==0x65) && (response[lenr-1]==0x01) ) {
      if (tst) {
        fprintf(stdout,"Karte nicht lesbar!\n");
      }
      fprintf(karte,"Karte nicht lesbar") ;
      PROGRAM_ABORT;
      }
    if ( (response[lenr-2]==0x6B) && (response[lenr-1]==0x00) ) {
      if (tst) {
        fprintf(stdout,"Falsches Offset!\n");
      }
      fprintf(karte,"Falsches Offset") ;
      PROGRAM_ABORT;
      }
    if ( ( (response[lenr-2]==0x62) && (response[lenr-1]==0x82) ) || ( (response[lenr-2]==0x90) && (response[lenr-1]==0x00) ) ){
      if (tst) {
        fprintf(stdout,"Command successful\n") ;
        for(x=0;x<lenr-4;x++) {
          if (response[x]<32 || response[x]>126) {
          fprintf(stdout," ");
        }
        else {
          fprintf(stdout,"%c",response[x]);
        }
       }
       printf("\n");
      }
      fz=fopen( "ef_kvk.txt" , "w+b" );
      if (fz==NULL) {
        fprintf(karte, "Fehler beim ™ffnen der Datei ef_kvk.txt");
        PROGRAM_ABORT;
        }
      if (fwrite( response, lenr-2, 1, fz) != 1) {
        fprintf(karte, "Fehler beim Schreiben in Datei ef_kvk.txt");
        PROGRAM_ABORT;
        }
     fclose(fz);
    }
    else {
      if (tst) {
        fprintf(stdout,"Anderer Fehler: Read Binary!\n");
      }
      fprintf(karte,"Anderer Fehler: Read Binary");
      PROGRAM_ABORT;
    }
  }
    
  if (ncard==2) {   // eGK
    nl=0;
    for(x=0;x<=sizeof(cst);x++){
    cst[x]=0x00;
    } 
    for(x=0;x<=sizeof(cpd);x++){
    cpd[x]=0x00;
    }
    for(x=0;x<=sizeof(cvd);x++){
    cvd[x]=0x00;
    }
    if (tst) {
    printf("\nVersichertenstatus:\n");
    }
    if (tst) {
    printf("CMD: ");     //  Versichertenstatus
    for(x=0;x<5;x++){
     printf("%02x ",rbvd[x]);
     }
    printf("\n");
    }
    dad=0;
    sad=2;
    lenr=sizeof(response);
    memset ( response, 0x00, 300 );
    CONNECT_TO_CT_DATA(ctn, dad, sad, 5, rbvd, lenr, response);
    antwort("Read Binary: Versichertenstatus");
    if ((mobil) && (nlib==2) && (response[lenr-2]==0x69) && (response[lenr-1]==0x00) ) {
       if (tst) {
           printf("Lesegeraet aufschliessen!\n");
       }
       fprintf(karte, "/nLesegeraet aufschliessen!");
       PROGRAM_ABORT;
    }	  
    if ( (response[lenr-2]==0x90) && (response[lenr-1]==0x00) ) {
      if (tst) {
      fprintf(stdout,"Command successful\n") ;
      }
     }
     else {
      fprintf(karte,"Fehler beim Auslesen der eGK");
      PROGRAM_ABORT;
     }
     for(x=0;x<lenr-2;x++){
        cst[nl++]=response[x];
      }
     if (tst) {
      fprintf(stdout,"\n");
      fprintf(stdout,"Ausgabe cst:\n");
      for(z=0;z<=lenr-3;z++){
        fprintf(stdout,"%02x ", cst[z]);
      }
      fprintf(stdout,"\n\n");
     }

    fz=fopen( "ef_st.txt" , "w+b" );
    if (fz==NULL) {
     fprintf(karte,"Fehler beim ™ffnen der Datei ef_st.txt");
     PROGRAM_ABORT;
     }
    if (fwrite(cst, lenr-3, 1, fz) != 1) {
      fprintf(karte, "Fehler beim Schreiben in Datei ef_st.txt!");
      PROGRAM_ABORT;
    }
    fclose(fz);  

    if (tst) {
    printf("\nPatientendaten:\n");
    }
    dad=0;
    sad=2;
    nl=0;
    for(y=1;y<11;y++) {
    lenegk=5;
    if (mobil) {
     lenegk=7;
    }
    if (tst) {
    printf("CMD: ");     //  Pers”nliche Daten
    for(x=0;x<lenegk;x++){
     printf("%02x ",rbegk[x]);
     }
    printf("\n");
    }
    lenr=sizeof(response);
    memset ( response, 0x00, lenr );
    CONNECT_TO_CT_DATA(ctn, dad, sad, lenegk, rbegk, lenr, response);
    antwort("Read Binary: Patientendaten");
    if ( (response[lenr-2]==0x90) && (response[lenr-1]==0x00) ) {
      if (tst) {
      fprintf(stdout,"Command successful\n") ;
      }
      if (y==1) {
        npd=response[0]*0x100+response[1];

        for(z=2;z<lenr-2;z++){
          cpd[nl++]=response[z];
          }
        } 
      else {
        for(z=0;z<lenr-2;z++){
          cpd[nl++]=response[z];
          }
        }
      }
     else {
      fprintf(karte,"Fehler beim Auslesen der eGK");
      PROGRAM_ABORT;
     }
    if (mobil) {
    if (tst) {
    printf("npd=%d\n",npd);
    printf("nl=%d\n",nl);
    }
    if (npd<=nl) {
    break;
    }
    else {
    rbegk[1]=0xB1;
    }
    }
    else {
    nm=256*y;
    if (nm>=npd){
    break;
    }
    else {
    rbegk[2]=y;
    dad=0;
    sad=2;
    }
    }
    }
    if (tst) {
    printf("\n");
    printf("Ausgabe cpd:\n");
    for(z=0;z<=npd-4;z++){
    printf("%02x ", cpd[z]);
    }
    printf("\n\n");
    }

    fz=fopen( "ef_pd.gz" , "w+b" );
    if (fz==NULL) {
     fprintf(karte,"Fehler beim ™ffnen der Datei ef_pd.gz");
     PROGRAM_ABORT;
     }
    if (fwrite(cpd, npd, 1, fz) != 1) {
      fprintf(karte, "Fehler beim Schreiben in Datei ef_pd.gz!");
      PROGRAM_ABORT;
    }
    fclose(fz);

    if (tst) {
    fprintf(stdout,"\nVersicherungsdaten:\n");
    }
    rbegk[1]=0xB0;
    rbegk[2]=0x82;
    nl=0;
    dad=0;
    sad=2;
    for(y=1;y<11;y++) {
    if (tst) {
    printf("CMD: ");     //  VD
    for(x=0;x<lenegk;x++){
     printf("%02x ",rbegk[x]);
     }
    printf("\n");
    }
    lenr=sizeof(response);
    memset ( response, 0x00, lenr);
    CONNECT_TO_CT_DATA(ctn, dad, sad, lenegk, rbegk, lenr, response);
    antwort("Read Binary: Versicherungsdaten");
    if ( (response[lenr-2]==0x90) && (response[lenr-1]==0x00) ) {
      if (tst) {
      fprintf(stdout,"Command successful\n") ;
      }
      if (y==1) {
        nvd1=response[2]*0x100+response[3];
        nvd2=response[6]*0x100+response[7];
        if (tst) {
        printf("nvd2=%d\n",nvd2);
        }
        if (nvd2==65535) {
          nvd2=nvd1;
         }
        if (tst) {
        printf("nvd1=%d\n",nvd1);
        printf("nvd2=%d\n",nvd2);
        }
        for(z=8;z<lenr-2;z++){
           cvd[nl++]=response[z];
           }
        }
      else {
        for(z=0;z<lenr-2;z++){
          cvd[nl++]=response[z];
          }
        }
        if (mobil) {
        if (tst) {
        printf("nvd2=%d\n",nvd2);
        printf("nl=%d\n",nl);
        }
        if (nvd2<=nl+7) {
          break;
        }
        else {
        rbegk[1]=0xB1;
        }
        }
      else {
      nm=256*y;
        if (tst) {
        printf("nvd2=%d\n",nvd2);
        printf("nm=%d\n",nl);
        }
      if (nm>=nvd2){
        break;
        }
      else {
        rbegk[2]=y;
        dad=0;
        sad=2;
        }
        }
      }
    else {
      if (tst) {
      fprintf(stdout, "Read Binary Fehler beim Auslesen der eGK\n");
      }
      fprintf(karte, "Read Binary Fehler beim Auslesen der eGK");
      PROGRAM_ABORT;
      }
    }
    if (tst) {
    printf("\n");
    printf("Ausgabe cvd:\n");
    for(z=0;z<=nvd2-4;z++){
    printf("%02x ", cvd[z]);
    }
    printf("\n");
    printf("nvd2 = %d\n",nvd2);
    }
    fz=fopen( "ef_vd.gz" , "w+b" );
    if (fz==NULL) {
      fprintf(karte,"Fehler beim Oeffnen der Datei ef_vd.gz");
      PROGRAM_ABORT;
      }
    if (fwrite(cvd, nvd2, 1, fz) != 1) {
      fprintf(karte, "Fehler beim Schreiben in Datei ef_vd.gz!");
      PROGRAM_ABORT;
    }
    fclose(fz);
    }
}

  /***Eject ICC*/
  if ( !(mobil) && !(argc>3) ) {
  dad=1;
  sad=2;
  ejecticc[0]=0x20;
  if (tst) {
  printf("Eject ICC\n");
  printf("CMD: ");
  for(x=0;x<sizeof(ejecticc);x++){
     printf("%02x ",ejecticc[x]);
     }
  printf("\n");
  }
  lenr=sizeof(response);
  memset ( response, 0x00, lenr );
  CONNECT_TO_CT_DATA(ctn, dad, sad, sizeof(ejecticc), ejecticc, lenr, response);
  antwort("Eject ICC");
  if ( ( (response[lenr-2]==0x90) && (response[lenr-1]==0x00) ) || ( (response[lenr-2]==0x90) && (response[lenr-1]==0x01) ) || ( (response[lenr-2]==0x62) && (response[lenr-1]==0x00) )) {
     if (tst) {
     fprintf(stdout, "Command successful\n");
     }
  }
  else {
    if (tst) {
    fprintf(stdout, "Fehler bei Eject ICC\n");
    }
    }
  }
     
  if ( (argc>3) &&  ( (nlib==8) || ( (nlib==3) && (ctn>10)  ) ) ) {
  /***Erase Binary */
  dad=0;
  sad=2;
  if (tst) {
  printf("Erase Binary\n");
  printf("CMD: ");
  for(x=0;x<4;x++){
     printf("%02x ",erase[x]);
     }
    printf("\n");
  }
  lenr=sizeof(response);
  memset ( response, 0x00, lenr );
  CONNECT_TO_CT_DATA(ctn, dad, sad, 4, erase, lenr, response);
  antwort("Erase Binary");
   if ( (response[lenr-2]==0x69) && (response[lenr-1]==0x86) ) {
     if (tst) {
      fprintf(stdout, "keine Daten zum Loeschen ausgew„hlt");
     }
     fprintf(stdout, "Loeschen fehlerhaft\n");
     PROGRAM_ABORT;
   }
   if ( (response[lenr-2]==0x65) && (response[lenr-1]==0x00) ) {
     if (tst) {
       fprintf(stdout, "Loeschen fehlgeschlagen");
     }
     fprintf(stdout, "Erasure failed\n");
     PROGRAM_ABORT;
   }
   if ( (response[lenr-2]==0x6B) && (response[lenr-1]==0x00) ) {
     if (tst) {
       fprintf(stdout, "Falscher Parameter");
     }
     fprintf(stdout, "Wrong parameter\n");
     PROGRAM_ABORT;
   }
   if ( (response[lenr-2]==0x90) && (response[lenr-1]==0x00) ) {
          if (tst) {
           fprintf(stdout, "Command successful\n");
       }
   }
   else {
     if (tst) {
       fprintf(stdout, "Loeschen misslungen");
     }
     PROGRAM_ABORT;
   }

  /***Eject ICC */
  dad=1;
  sad=2;
//  ejecticc[5]=0x01;
  if (tst) {
  printf("Eject ICC\n");
  printf("CMD: ");
  for(x=0;x<sizeof(ejecticc);x++){
     printf("%02x ",ejecticc[x]);
     }
  printf("\n");
  }
  lenr=sizeof(response);
  memset ( response, 0x00, lenr );
  CONNECT_TO_CT_DATA(ctn, dad, sad, sizeof(ejecticc), ejecticc, lenr, response);
  antwort("Eject ICC");
  if ( ( (response[lenr-2]==0x90) && (response[lenr-1]==0x00) ) || ( (response[lenr-2]==0x90) && (response[lenr-1]==0x01) ) || ( (response[lenr-2]==0x62) && (response[lenr-1]==0x00) )) {
    if (tst) {
     fprintf(stdout, "Command successful\n");
     }
    }
    else {
      if (tst) {
        fprintf(stdout, "Fehler bei Eject ICC\n");
      }
    }
 }

 PROGRAM_TERMINATE;

 return 0;
}

