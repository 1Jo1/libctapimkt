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
     Kernel > 2.6.8
     tested on various card readers that are certified from the KBV with KVK,
     eGK tested on Celectronic CardStar medic2.      

   Report bugs or comments to:
        dr.claudia.neumann@gmx.de

   Known Bugs:
      none (hopefully) ;-)

*/

/* CT-API Errors */
#define OK          		0
#define ERR_INVALID   -1
#define ERR_CT            -8
#define ERR_TRANS     -10
#define ERR_MEMORY  -11
#define ERR_HOST       -127
#define ERR_HTSI        -128

/* Prototypes */

char CT_init (  unsigned short  ctn,
                unsigned short  pn);

char CT_data ( unsigned short   ctn,
               unsigned char  * dad,
               unsigned char  * sad,
               unsigned short   lenc,
      	       unsigned char  * command,
               unsigned short * lenr,
               unsigned char  * response );

char CT_close ( unsigned short  ctn );

