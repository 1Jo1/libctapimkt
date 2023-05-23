/*  ctapi-mkt
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

   This is a CT API for MKT Version 1.0 for reading the german "Krankenversichertenkarte" 
   (KVK)  und the german "elektronische Gesundheitskarte" (eGK) with card readers on
   the serial port.

   See documentations under http://www.kbv.de/ita/register_G.html and 
   http://www.gematik.de/upload/gematik_Qop_eGK_Spezifikation_Teil1_V1_1_0_Kommentare_4_1652.pdf 

   It works with:
     Kernel > 2.6.0
     tested on various card readers that are certified from the KBV with KVK,
     KVK and eGK tested on Celectronic CardStar medic2, Thales mediCompact.

   Report bugs or comments to:
        dr.claudia.neumann@gmx.de

   Known Bugs:
      none (hopefully) ;-)

*/

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>

#include "ctapimkt/ctapi.h"


unsigned char ask_resync[] = {0x12, 0xC0, 0x00, 0xD2};
unsigned char ask_resync_medMobile[] = {0x16, 0x00, 0x05, 0x20, 0x11, 0x00, 0x00, 0x00, 0x22};
unsigned char frage[300];
struct termios tios;
int wtx=0;
int pcb=0;
int maxbytes[255];  

int ctn2fd[255]=
       {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


unsigned char fxor(const unsigned char *s,int l){
  unsigned char r=0;
  while(l--)r^=s[l];
  return(r);
}

int sendblock( int port, unsigned char *block, unsigned short lenc ){
  int retval,i;

  #ifdef DEBUG
  fprintf(stdout,"block[1]=%02x\n",block[1]);
  fprintf(stdout,"lenc=%d\n",lenc);
  for (i=0; i <lenc; i++) {
    fprintf(stdout, "%02x ", block[i]);
  }
  printf("\n");
  #endif

  retval = write(port, block, lenc);
  if (retval < 0)return retval;
  if (retval != lenc) return ERR_INVALID;
  return(retval);
}

int readblock( int port, unsigned short len) {
  int retval,i;

  memset( frage, 0x00, sizeof(frage));
  if(tcgetattr(port,&tios)==-1)err(1,NULL);
  tios.c_cc[VMIN]=len;
  if(tcsetattr(port,TCSANOW,&tios)==-1)err(1,NULL);
  retval=read(port,frage,len);
  if (retval < 0)return retval;

  #ifdef DEBUG
/*  fprintf(stdout,"retval=%d\n",retval); */
  for(i=0;i<retval;i++){
  fprintf(stdout," %02x",frage[i]);
  }
  fprintf(stdout,"\n");
  #endif
return(retval);
}
  


char CT_init (  unsigned short  ctn, unsigned short  pn ) {
 int port, i, fd, flags, retval, lenc;
 unsigned char apdu[1250];
 unsigned char device[20];
 maxbytes[ctn] = 32;

 switch(ctn) {
  case 0: strcpy(device,"/dev/ttyS0"); break;   /* COM1 mit 9600 Baud */
  case 1: strcpy(device,"/dev/ttyS0"); break;   /* COM1 mit 9600 Baud */
  case 2: strcpy(device,"/dev/ttyS1"); break;   /* COM2 mit 9600 Baud */
  case 3: strcpy(device,"/dev/ttyS2"); break;   /* COM3 mit 9600 Baud */
  case 4: strcpy(device,"/dev/ttyS0"); break;   /* COM1 mit 115200 Baud */
  case 5: strcpy(device,"/dev/ttyS1"); break;   /* COM2 mit 115200 Baud */
  case 6: strcpy(device,"/dev/ttyACM0"); break; /* USB1 ber ACM0 */
  case 7: strcpy(device,"/dev/ttyACM1"); break; /* USB2 ber ACM1 */
  case 8: strcpy(device,"/dev/ttyS0"); break;   /* USB1 ber ACM0 mit Verz”gerung fr VML GK1 */
  case 9: strcpy(device,"/dev/ttyUSB0"); break; /* USB ber USB-seriell-Adapter mit 9600 Baud */
  case 10: strcpy(device,"/dev/ttyUSB0"); break; /* USB ber USB-seriell-Adapter mit 115200 Baud */
  case 11: strcpy(device,"/dev/ttyACM0"); break; /* USB1 ber ACM0 fr Hypercom medMobile und SCM eHealth500 */
  case 12: strcpy(device,"/dev/ttyACM1"); break; /* USB2 ber ACM1 fr Hypercom medMobile und SCM eHealth500 */
  default:  strcpy(device,"/dev/ttyS0"); break;
  }

  if (ctn2fd[ctn] == 0){
    port = open (device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (port < 0) return ERR_INVALID;
    if(tcgetattr(port,&tios)==-1)err(1,NULL);
    tios.c_iflag&=~(IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK|ISTRIP|INLCR|IGNCR|ICRNL|IUCLC|IXON|IXANY|IXOFF|IMAXBEL);
    tios.c_oflag&=~OPOST;
    tios.c_cflag&=~(CSTOPB|PARODD|CRTSCTS);
    tios.c_cflag|=PARENB|CLOCAL;
    tios.c_lflag&=~(ISIG|ICANON|XCASE|ECHO);
    tios.c_cc[VTIME]=0;
    if ( (ctn==4) || (ctn==5) || (ctn==10) ) {
      cfsetspeed(&tios,B115200);
    }
    else {
      cfsetspeed(&tios,B9600);
    }
    if(tcsetattr(port,TCSANOW,&tios)==-1)err(1,NULL);
    if(tcflush(port,TCIOFLUSH)==-1)err(1,NULL);
    flags=fcntl(port,F_GETFL);
    if(flags==-1)err(1,NULL);
    if(fcntl(port,F_SETFL,flags&~O_NONBLOCK)==-1)err(1,NULL);
    if(flags==-1)err(1,NULL);
  }

  if (ctn==8){
    usleep(50000);
  }
  if ( (ctn==11) || (ctn==12) ){
    lenc=sizeof(ask_resync_medMobile);
    #ifdef DEBUG
    fprintf(stdout,"CMD: ");
    for (i=0; i <lenc; i++) {
      fprintf(stdout, "%02x ", ask_resync_medMobile[i]);
    }
    printf("\n");
    #endif
    retval = write(port, ask_resync_medMobile, lenc);
    if (retval < 0)return retval;
    if(tcgetattr(port,&tios)==-1)err(1,NULL);
    tios.c_cc[VMIN]=6;
    if(tcsetattr(port,TCSANOW,&tios)==-1)err(1,NULL); /* could miss some errors */
    retval=read(port,apdu,6);
    #ifdef DEBUG
    fprintf(stdout,"RSP: ");
    for (i=0; i <6; i++) {
      fprintf(stdout, "%02x ", apdu[i]);
    }
    printf("\n");
    #endif

    if(retval < 0)return ERR_INVALID;
    if ((apdu[0] != 0x61) || (apdu[1] != 0x00) )return ERR_INVALID;
  }
  else {
    lenc=sizeof(ask_resync);
    #ifdef DEBUG
    fprintf(stdout,"CMD: ");
    for (i=0; i <lenc; i++) {
      fprintf(stdout, "%02x ", ask_resync[i]);
    }
    printf("\n");
    #endif
    retval = write(port, ask_resync, lenc);
    if (retval < 0)return ERR_INVALID;
    if(tcgetattr(port,&tios)==-1)err(1,NULL);
    tios.c_cc[VMIN]=4;
    if(tcsetattr(port,TCSANOW,&tios)==-1)err(1,NULL); /* could miss some errors */
  
    retval=read(port,apdu,4);
    #ifdef DEBUG
    fprintf(stdout,"RSP: ");
    for (i=0; i <4; i++) {
      fprintf(stdout, "%02x ", apdu[i]);
    }
    printf("\n");
    #endif

    if(retval < 0)return retval;
    if ((apdu[0] != 0x21) || (apdu[1] != 0xe0) )return ERR_INVALID;
  }
  pcb=0;
  ctn2fd[ctn] = port;
  return OK;
} 


char CT_data ( unsigned short   ctn,
               unsigned char  * dad,
               unsigned char  * sad,
               unsigned short   lenc,
      	       unsigned char  * command,
               unsigned short * lenr,
               unsigned char  * response )
{

int port, retval, i, j, lenblock,lenresp,lenf,fehler;
unsigned char block[300];
unsigned char len;
unsigned char apdu[300];
int error_status;

if ((port = ctn2fd[ctn]) < 1){
  *lenr = 0;
  return ERR_CT;
}
len = *lenr;
lenresp = 0;
fehler = 0;

block[0] = (*dad <<4)| *sad;

if (lenc == 0 ){
  if ( wtx == 0) {
    block[1] = 0x80;
    wtx=1;
  }
  else {
    block[1] = 0x90;
    wtx=0;
  }
}
else {
  if (pcb==0) {
    block[1] = 0x00;
    pcb=1;
    wtx=0;
  }
  else{
    block[1] = 0x40;
    pcb=0;
    wtx=0;
  }
}

block[2] = lenc;
memcpy(&block[3], command, lenc);

block[lenc+3]=fxor(block,lenc+3);
lenc=lenc+4;

j=0;
while (j<10){
  printf("counter: %d\n", j);
  retval=sendblock(port, block, lenc);
  if (retval < 0)return retval;
  printf("0\n");
  retval=readblock(port, 3);
  if (retval < 0)return retval;

  printf("1\n");
  memset( apdu, 0x00, sizeof(apdu));

			  printf("%s\n", frage);
  for (i=0; i<3 ; i++){
    apdu[i]=frage[i];
  }
		  printf("1.0.1\n");
  len=frage[retval-1]+1;
	  printf("1.0\n");
  if ( (frage[1]==0x81) || (frage[1]==0x91) || (frage[1]==0x82) || (frage[1]==0x92) ){
    #ifdef DEBUG
    fprintf(stdout,"\ntransmission error\n");
    #endif
    
	  printf("1.1\n");
    retval=readblock(port, 1);
	  	  printf("1.2\n");

    if (retval < 0)return retval; 
	  	  	  printf("1.3\n");

    printf("2\n");
    apdu[3]=frage[0];
    if (apdu[3]!=fxor(apdu,3)) return ERR_INVALID;

    printf("3\n");
    fehler++;
    if (fehler==2){
    retval = sendblock(port, ask_resync, sizeof(ask_resync));
    if (retval < 0)return retval;
    
    printf("4\n");

    retval=readblock(port,4);
    if (retval < 0)return retval;

    printf("5\n");
    if (frage[3]!=fxor(frage,3)) return ERR_INVALID;
				
    printf("6\n");
    if (frage[1] != 0xe0) return ERR_INVALID;

    printf("7\n");
    block[1] = 0x00;
    pcb=1;
    wtx=0;
    }
  }
  else if (frage[1]==0xC0) {
	  	  printf("2.1\n");
    #ifdef DEBUG
    fprintf(stdout,"\nResynch request\n");
    #endif
    
    retval=readblock(port, 1);
    if (retval < 0)return retval;
	  
	  	  	  	  printf("2.1.1\n");

    apdu[3]=frage[0];
    if (apdu[3]!=fxor(apdu,3)) return ERR_INVALID;
    fehler=0;
    apdu[1]=0xE0;
    apdu[3]=fxor(apdu,3);
    retval = sendblock(port, apdu, 4);
    if (retval < 0)return retval;
    block[1] = 0x00;
    pcb=1;
    wtx=0;
  }
  else if (frage[1]==0xC3) {
	  	  	  printf("2.2\n");
    #ifdef DEBUG
    fprintf(stdout,"WTX-Request\n");
    #endif
    if(j==10)errx(1,"WTX-Fehler\n");
    retval=readblock(port, len);
	  	  	  	  printf("2.2.1\n");
    if (retval < 0)return retval;

    lenblock=len+3;
    for(i=0;i<lenblock;i++) {
      apdu[i+3]=frage[i];
    }
    if (apdu[lenblock-1]!=fxor(apdu,lenblock-1)) return ERR_INVALID;
    for(i=0;i<lenblock;i++) {
      block[i]=apdu[i];
    }
    block[0]=(block[0] <<4)| *sad;
    block[1]=0xE3;
    block[lenblock-1]=fxor(block,lenblock-1);
    lenc=lenblock;
    j++;
  }
  else {
	  	  	  	  printf("2.3\n");
    retval=readblock(port,len);
    if (retval < 0)return retval;
	  	  	  printf("2.3.1\n");
    lenblock=len+3;
    for(i=0;i<lenblock;i++) {
      apdu[i+3]=frage[i];
      response[lenresp+i]=frage[i];
    }
    if (apdu[lenblock-1]!=fxor(apdu,lenblock-1)) return ERR_INVALID;
   
    lenblock=len+3;
    lenresp=lenresp+len-1;
    *lenr=lenresp;
	
    #ifdef DEBUG
/*    fprintf(stdout,"\n"); */
    #endif

    if(apdu[lenblock-1]!=fxor(apdu,lenblock-1)){
      for (i=0; i<lenblock+1; i++) {
      #ifdef DEBUG
      fprintf(stdout, " %02x", apdu[i]);
      #endif
      }
      errx(1,"transmission error: fxor");
    }
    if((apdu[1]==0x60) || (apdu[1]==0x20)) {
      #ifdef DEBUG
      fprintf(stdout,"Subsequent block follows\n");
      #endif
      if(j==10)errx(1,"Subsequent block-Fehler\n");
      block[0]=0x02;
      if (apdu[1]==0x60) {
        block[1]=0x80;
      }
      else {
        block[1]=0x90;
      }
      block[2]=0x00;
      lenblock=4;
      block[lenblock-1]=fxor(block,lenblock-1);
      lenc=lenblock;
      j++;
    }
    else {
      break;
    }
  }
}
return OK;
}

char CT_close ( unsigned short  ctn ){
  int port;

  if ((port = ctn2fd[ctn]) < 1) return ERR_INVALID;
  close(port);
  ctn2fd[ctn] = 0;
  return OK;
}




