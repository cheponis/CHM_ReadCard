// ReadCard.cpp - Read Cards from the Card Reader  Copyright (C) 1999 Mike Cheponis
//
// 3 Dec 99 - First real version w/Interrupt-driven receiver routines.
//
//@rem something weird happens w/Interrupts with /G3 even on Pentium; use /G2
//Microsoft C++ Compiler for DOS
//cl /W4 /nologo /Ox /G2drf readcard.cpp
//
//
//
//
//
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include <signal.h>
#include <time.h>

// Register definitions
#include "c.h"

// conveinent typedefs
#include "cwi.h"

void (cdecl interrupt far *int0C)();// Holds what is in int 0x0c when we start
void cdecl far interrupt rs232_interrupt(void);

void (cdecl interrupt far *int1C)();// Holds what is in int 0x1c when we start
void cdecl far interrupt timertick(void);//Routine we insert at 0x1c interrupt

void __cdecl Control_C_Handler( int sig );

void putchar_rs232(char c);
char getchar_rs232();
void  Enable_RS232();

///////////////////////////// Timer-Related ////////////////////////////////
u32 timer_tick_counter = 0UL;

#define	colorburst (3579545.0)	// Freq in Hertz
// This is where the 18.2 tick per second comes in...
// Also note that colorburst * 4 = 14.whatever used for CGA adaptor
//  ms_per_tick = 1.0 / (colorburst/3.0/65536.0); // This is in ms/tick, tho
/////////////////////////////////////////////////////////////////////////////

#define	COM1	(0x3f8)	// Port address for Com1
#define PIC0	(0x20)  // Primary Programmable Interrupt Controller
#define	EOI	(0x20)	// EOI End of Interrupt cmd for PIC

// Circular receive buffer.  200 characters.  Size is pretty arbitrary.

#define  MAX_RS232_RX_BUFFER_SIZE  (200)
u8 rs232_rx_buffer[MAX_RS232_RX_BUFFER_SIZE];
volatile u16 put_index_rs232_rx_buffer=0; // Sometimes called "head" index
volatile u16 get_index_rs232_rx_buffer=0; // Sometimes called "tail" index


i32   Read_Cards(FILE* fp);
void  Setup_Hardware();
FILE *Grab_Filename_and_Open_File();

void Dump_File_Header_Parameters(FILE* fp, int argc, char *argv[]);
void Dump_File_Trailer_And_Close_File(FILE* fp, i32 n_cards);
void Get_Ready_To_Read_Cards();


#define	FILENAME_SIZE (99)
char Filename[FILENAME_SIZE];
#pragma warning(disable:4702)

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
/////////////////////////void cdecl main()/////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void cdecl main(int argc, char* argv[]){
FILE* fp;
i32 n_cards;

  printf("\nThe Computer Museum History Center Card Reader Program\n\n");

  Setup_Hardware();

  for(;;){                  // Loop forever, leave program by typing ^C

    if( (fp=Grab_Filename_and_Open_File()) == NULL) continue;

    Dump_File_Header_Parameters(fp,argc,argv);

    Get_Ready_To_Read_Cards();

    n_cards = Read_Cards(fp);

    Dump_File_Trailer_And_Close_File(fp,n_cards);

  } // Matches for(;;)

// We know we'll never reach here, so tell compiler to not complain about that
//#pragma warning(disable:4702)    (which is actually located above)
}


/////////////////////////////////////////////////////////////////////////////
////////////////////// i32 Read_Cards(FILE* fp) ///////////////////////////
/////////////////////////////////////////////////////////////////////////////

//Baud rate is 19200.  Switches are irrelevant, or, at least, are not correct.
//^B does put it in Column Binary, ^F does cause next card to read.
//When you send ^F and no card is there, it sends 'error' then 0x0a.
//Each 160 bytes of column-binary have a 0x0a afterwards.
//Column binary is offset 0x40 per character.
//
//            LSBit             LSBit
//12 11 0 1 2 3       4 5 6 7 8 9

void Reset_RX_Buffer_Pointers();

/////////////////////////////////////////////////////////////////////////////
i32
Read_Cards(FILE* fp){
  i32 n_cards; // For counting number of cards read.
  int i,ic;
  char c;
  char card_image[163];  // 160 data chars, \n, and room for \0

  n_cards = 0L;
  putchar_rs232('B' & 0xf);	// Send "Control-B" to put it in Binary mode

  for(;;){
  top:
    Reset_RX_Buffer_Pointers();	// Need to clear out garbage received stuff
    putchar_rs232('F' & 0xf);	// Send Control-F to read this card

    for(i=0;i<160;i++){
      while( (c=getchar_rs232()) < 15); // Spin until we see a REAL char

      card_image[i] = c;  // Save this char away

//      if(i==0) printf("<<=");
//      if(i<5) printf("(%c)",c);
//      if(i==4) printf("=>>\n\n");


      // got 5 chars (i==4) and the first 5 are "error" then...
      if((i==4) && (strncmp(card_image,"error",5) == 0)){
        printf("More cards? (y/n)");  fflush(stdout);
        ic = getchar(); fflush(stdin);
        if(ic == 'n' || ic == 'N')return n_cards;
	else goto top;
      }
    }
    card_image[160] = 0; // Null-terminate this string, make the \r a null
    fprintf(fp,"%s\n",card_image);
    n_cards++;
    printf("n_cards = %lu\n",n_cards);
  }
}

/////////////////////////////////////////////////////////////////////////////
////////////////////// void Get_Ready_To_Read_Cards() ///////////////////////
/////////////////////////////////////////////////////////////////////////////
void Get_Ready_To_Read_Cards(){
  printf("Press \"Enter\" when the Card Reader is ready: ");
  getchar();
  fflush(stdin);
}


/////////////////////////////////////////////////////////////////////////////
//////////////////// Dump_File_Trailer_And_Close_File(n_cards) //////////////
/////////////////////////////////////////////////////////////////////////////

void Dump_File_Trailer_And_Close_File(FILE* fp, i32 n_cards){

  fprintf(fp,"$NUMBER_OF_CARDS %ld\r\n",n_cards);

  fprintf(fp,"$END_OF FILE \"%s\"\r\n",Filename);

  fclose(fp);
  printf("Closed \"%s\"\n",Filename);
}


/////////////////////////////////////////////////////////////////////////////
////////////////// void Dump_File_Header_Parameters(argc,argv) //////////////
/////////////////////////////////////////////////////////////////////////////
void Dump_File_Header_Parameters(FILE* fp,int argc,char *argv[]){

struct tm *newtime;
time_t aclock;
int i;

  if(argc>1){	// If indeed there -are- any parameters...
    fprintf(fp,"$PARAMETERS");
    for(i=1; i<argc; i++) fprintf(fp," %s",argv[i]);
    fprintf(fp,"\r\n");
  }

  fprintf(fp,"$FILENAME \"%s\"\r\n",Filename);

  time (&aclock); // Get time in seconds
  newtime = localtime(&aclock); // Convert time to struct tm form
  fprintf(fp,"$DATE %s\r",asctime(newtime)); // Save as ascii in file

  fprintf(fp,"$COLUMN_BINARY\r\n");

}

/////////////////////////////////////////////////////////////////////////////
////////////////////// FILE *Grab_Filename_and_Open_File() //////////////////
/////////////////////////////////////////////////////////////////////////////

FILE *Grab_Filename_and_Open_File(){
char s[FILENAME_SIZE];	// This should be long enough for any filename.
int ic, i;

    for(i=0;i<sizeof(s);i++)s[i]=0; // Clear our input buffer
    printf("\n\nFilename? ");

    if(fgets(s,sizeof(s),stdin) == NULL){// fgets saves room for \0 at end
      printf("Some error occured while trying to get the filename.\n");
      fflush(stdin);	// flush all I/O streams
      return NULL;
    }

    if(s[strlen(s)-1] != '\n'){
      printf("Name too long, I'm afraid.\n");
      fflush(stdin);	// flush all I/O streams
      return NULL;
    }

    s[strlen(s)-1] = 0; // Else convert the \n to a null

    FILE* fp=fopen(s,"r");
    if(fp != NULL){
      fclose(fp);	// Yikes, the file is already in existence!
      fflush(stdin);
      printf("\"%s\" already exists!  Do you want to overwrite it? (y/n) ",s);
      ic = getchar();
      fflush(stdin); // Flush the \n away
      if(ic == 'n' || ic == 'N') return NULL;
    }

    // Else, overwrite, so here goes!

    fp=fopen(s,"wb"); // Note, it's in Binary mode
    if(fp == NULL){
      printf("Error opening \"%s\" for writing!\n",s);
      fflush(stdin);
      return NULL;
    }
  strcpy(Filename, s);	// Save Filename for whatever purpose
  return fp;
}



///////////////////////////////////////////////////////////////////////////
/////////////////////// void  Setup_Hardware() ////////////////////////////
///////////////////////////////////////////////////////////////////////////
void  Setup_Hardware(){

  // Install signal handler to modify CTRL+C behavior
  if( signal( SIGINT, Control_C_Handler) == SIG_ERR ){
    fprintf( stderr, "Couldn't set SIGINT for Control-C Handler!\n" );
    abort();
  }

  int0C =_dos_getvect(0x0C);  _dos_setvect(0x0C, rs232_interrupt);
  int1C =_dos_getvect(0x1C);  _dos_setvect(0x1C, timertick);

  Enable_RS232();	// Sets up the regs for 19200,8,N,1
}


///////////////////////////////////////////////////////////////////////////
void __cdecl Control_C_Handler( int sig ){

  _dos_setvect(0x0C,int0C);	// Set the vector back to what it was, RS232
  _dos_setvect(0x1C,int1C);	// Set the vector back to what it was, timer

  u8 mcr = (u8)inp(MODEM_CONTROL_REGISTER+COM1);
  outp(MODEM_CONTROL_REGISTER+COM1, mcr & ~( MCR_DTR | MCR_RTS ) );
                                                         // resets RTS and DTR
  printf("\nExiting.\n");
  exit(sig);
}



/****************************************************************************/
/********************* void interrupt timertick(void) ***********************/
/****************************************************************************/
void cdecl far interrupt
timertick(void){
  timer_tick_counter++;	// We have one more tick, so count it!
  _chain_intr(int1C);   // Do whatever else is chained on this interrupt
}

/////////////////////////////////////////////////////////////////////////////
#define Enable_RS232_Interrupt  \
                  outp(INTERRUPT_ENABLE_REGISTER+COM1, IER_RX_DATA_READY)

#define Disable_RS232_Interrupt   outp(INTERRUPT_ENABLE_REGISTER+COM1, 0)
/////////////////////////////////////////////////////////////////////////////
void Enable_RS232(){

  printf("Seting up 8,N,1 at 19,200 baud on Com Port 1.\n");

  Disable_RS232_Interrupt;
  outp(FIFO_CONTROL_REGISTER+COM1,0);	// FIFOs turned off

  _disable();				// No ints while we have DLAB on
  outp(LINE_CONTROL_REGISTER+COM1, LCR_DLAB);  // DLAB on, ready to write
  outp(DIVISOR_LATCH_LOW +COM1, 6);
  outp(DIVISOR_LATCH_HIGH+COM1, 0);	// This is 19,200 baud
  outp(LINE_CONTROL_REGISTER+COM1,
  	  LCR_WORD_LENGTH_SELECT_0 | LCR_WORD_LENGTH_SELECT_1); // 8,N,1
  _enable();

  u8 was = (u8)inp(0x21);
  outp(0x21,  was & 0xef);	// 0x21 is the Interrupt Mask Reg
                                // Inside the interrupt controller
				// 0xEF  (1110 1111) enables INT4
				// Note here "enable" means make the bit a 0

  // Throw away the junk in the input reg...
  inp(RECEIVE_BUFFER_REGISTER+COM1);  inp(RECEIVE_BUFFER_REGISTER+COM1);
  inp(RECEIVE_BUFFER_REGISTER+COM1);  inp(RECEIVE_BUFFER_REGISTER+COM1);

  Enable_RS232_Interrupt;
  u8 mcr = (u8)inp(MODEM_CONTROL_REGISTER+COM1);
  outp(MODEM_CONTROL_REGISTER+COM1,(int)(mcr | MCR_OUT2 | MCR_DTR | MCR_RTS));
                                                       // RTS and DTR now on
}

/****************************************************************************/
/************* void cdecl far interrupt rs232_interrupt(void) ***************/
/****************************************************************************/
void cdecl far interrupt
rs232_interrupt(void){

#if 0

static  volatile u8 far  *far_pointer  =     (u8 far *)MK_FP(0xb800,0);
static  volatile u8 attribute = *(far_pointer + 1);

  if(inp(LINE_STATUS_REGISTER+COM1) & 0xe)	// Bad news some kind of error
    *far_pointer++ = '!'; // Framing, Parity, or Overrun error occurred
  else
    *far_pointer++ = '*';

  *far_pointer++ = attribute;

#endif
// #if 0


  // Get the character and put it on the buffer
  rs232_rx_buffer[put_index_rs232_rx_buffer++] =
                                       (u8)inp(RECEIVE_BUFFER_REGISTER+COM1);

  // Wrap the buffer if necessary, should probably check for overflow
  if(put_index_rs232_rx_buffer == MAX_RS232_RX_BUFFER_SIZE)
                                              put_index_rs232_rx_buffer = 0;
  outp(PIC0,EOI); // Make the PIC happy (Programmable Interrupt Controller)
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void
putchar_rs232(char c){

    while( (inp(LINE_STATUS_REGISTER+COM1) &
            LSR_TRANSMITTER_HOLDING_REGISTER_EMPTY) == 0); // Wait for ready

    outp(TRANSMIT_HOLDING_REGISTER+COM1, c);  // Blast it out!
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////// char getchar_rs232() ////////////////////////
//////////////////////////////////////////////////////////////////////////
// Returns 0 if no character is available.  This is OK if we never expect
// to receive data that has nulls in it, and we don't.

char
getchar_rs232(){

  char c = 0;

  // Make sure no one else is putting characters in the buffer while we
  // check and take characters off it.

  _disable();  // Global disable ints,

// NOTE!  Doing the next line results in jerky interrupt response (see below)
// Disable_RS232_Interrupt;

  if(put_index_rs232_rx_buffer != get_index_rs232_rx_buffer) //got some!
  {
    c = rs232_rx_buffer[ get_index_rs232_rx_buffer++ ]; // Snag next char

    if(get_index_rs232_rx_buffer == MAX_RS232_RX_BUFFER_SIZE)
      get_index_rs232_rx_buffer = 0;    // circularize the buffer if needed
  }


// NOTE!  Doing the next line results in jerky interrupt response (in concert
//  with Disable_RS232_Interrupt, above, and without the _enable(),_disable()

// Enable_RS232_Interrupt;

  _enable();	// This is the CORRECT thing to do.

// That's why we MUST use global interrupt enable/disable.  I could
// probably ALSO use the PIC's interrupt enable/disable, but doing it
// this way is very easy and it does work.

  return c;
}
/////////////////////////////////////////////////////////////////////////////
////////////////////// void Reset_RX_Buffer_Pointers() //////////////////////
/////////////////////////////////////////////////////////////////////////////
void Reset_RX_Buffer_Pointers(){  // Flush buffers (reset pointers
  _disable();			  // We could also just set the buffers
  get_index_rs232_rx_buffer = 0;  // to be equal to each other, but what
  put_index_rs232_rx_buffer = 0;  // the heck...
  _enable();
}

/**************************** Function Header ******************************/
/*                                                                         */
/*      Function Name    : crc                                             */
/*                                                                         */
/*      Input Parameters : input_string - string to calculate crc for      */
/*                         length - number of characters from string to use*/
/*      Return Value     : unsigned char - crc value                       */
/*      Description      : This routine computes the 8-bit CRC represented */
/*                         by the polynomial x**8 + x**4 + x**3 + x**2 + 1.*/
/*                         passed in.                                      */
/*                                                                         */
/*      Usage:  Assume you are transmitting, and have the bytes to be      */
/*              transmitted in a string called "s".                        */
/*                                                                         */
/*              Declaration are:                                           */
/*               unsigned char s[50];   // Long enough for longest command */
/*               unsigned short string_length;                             */
/*               unsigned char crc_value;                                  */
/*                                                                         */
/*              The CRC routine is called in this way:                     */
/*                crc_value = crc(s,string_length);                        */
/*                                                                         */
/*              We check the CRC by doing this (in this example):          */
/*                rx_string_length = 5;  (r is char[])                     */
/*                crc_check_value = crc(r,rx_string_length);               */
/*                if(crc_check_value != (unsigned char)0xff)               */
/*                  Call_An_Error_Routine();                               */
/*                                                                         */
/*      Created By       : Mike Cheponis                                   */
/*      Created In       : 3/98                                            */
/***************************************************************************/
unsigned char
crc(unsigned char *input_string, unsigned short length){

  static unsigned short crc_table[256] = {
    0x00,0x64, 0xc8,0xac, 0xe1,0x85, 0x29,0x4d, 0xb3,0xd7, 0x7b,0x1f,
    0x52,0x36, 0x9a,0xfe, 0x17,0x73, 0xdf,0xbb, 0xf6,0x92, 0x3e,0x5a,
    0xa4,0xc0, 0x6c,0x08, 0x45,0x21, 0x8d,0xe9, 0x2e,0x4a, 0xe6,0x82,
    0xcf,0xab, 0x07,0x63, 0x9d,0xf9, 0x55,0x31, 0x7c,0x18, 0xb4,0xd0,
    0x39,0x5d, 0xf1,0x95, 0xd8,0xbc, 0x10,0x74, 0x8a,0xee, 0x42,0x26,
    0x6b,0x0f, 0xa3,0xc7, 0x5c,0x38, 0x94,0xf0, 0xbd,0xd9, 0x75,0x11,
    0xef,0x8b, 0x27,0x43, 0x0e,0x6a, 0xc6,0xa2, 0x4b,0x2f, 0x83,0xe7,
    0xaa,0xce, 0x62,0x06, 0xf8,0x9c, 0x30,0x54, 0x19,0x7d, 0xd1,0xb5,
    0x72,0x16, 0xba,0xde, 0x93,0xf7, 0x5b,0x3f, 0xc1,0xa5, 0x09,0x6d,
    0x20,0x44, 0xe8,0x8c, 0x65,0x01, 0xad,0xc9, 0x84,0xe0, 0x4c,0x28,
    0xd6,0xb2, 0x1e,0x7a, 0x37,0x53, 0xff,0x9b, 0xb8,0xdc, 0x70,0x14,
    0x59,0x3d, 0x91,0xf5, 0x0b,0x6f, 0xc3,0xa7, 0xea,0x8e, 0x22,0x46,
    0xaf,0xcb, 0x67,0x03, 0x4e,0x2a, 0x86,0xe2, 0x1c,0x78, 0xd4,0xb0,
    0xfd,0x99, 0x35,0x51, 0x96,0xf2, 0x5e,0x3a, 0x77,0x13, 0xbf,0xdb,
    0x25,0x41, 0xed,0x89, 0xc4,0xa0, 0x0c,0x68, 0x81,0xe5, 0x49,0x2d,
    0x60,0x04, 0xa8,0xcc, 0x32,0x56, 0xfa,0x9e, 0xd3,0xb7, 0x1b,0x7f,
    0xe4,0x80, 0x2c,0x48, 0x05,0x61, 0xcd,0xa9, 0x57,0x33, 0x9f,0xfb,
    0xb6,0xd2, 0x7e,0x1a, 0xf3,0x97, 0x3b,0x5f, 0x12,0x76, 0xda,0xbe,
    0x40,0x24, 0x88,0xec, 0xa1,0xc5, 0x69,0x0d, 0xca,0xae, 0x02,0x66,
    0x2b,0x4f, 0xe3,0x87, 0x79,0x1d, 0xb1,0xd5, 0x98,0xfc, 0x50,0x34,
    0xdd,0xb9, 0x15,0x71, 0x3c,0x58, 0xf4,0x90, 0x6e,0x0a, 0xa6,0xc2,
    0x8f,0xeb, 0x47,0x23
  };

  unsigned char crc_value = 0xff;
  unsigned short index;

  for(index=0; index<length; index++)
    crc_value = (unsigned char) ~crc_table[crc_value ^ input_string[index]];

  return (unsigned char) crc_value;
}

/******************************************************************************
Baud Rate = 38,400 8,N,1
Power up: Ascii mode
send ctrl-B to switch to Binary mode
ctrl-A to go back to Ascii mode

SW1 should be OPEN to allow program control of when a card picks.

ctrl-F is the pick command

ctrl-s is "stop read" and ctrl-q is "start (actually, continue) read"
Should not be needed in normal case of interlocked (program-controlled) picks.

Hopper check, pick check, stack check, or read check causes "ERROR" to be
sent.

There is something about 3 indicator lights: RFE, ROR, RPE.  I don't see
how to get these unless the setup is wrong.

-----------------------------------------------------------------------------

Data gotten on 20 Nov 99, from REAL data...

Baud rate is 19200.  Switches are irrelevant, or, at least, are not correct.

^B does put it in Column Binary, ^F does cause next card to read.

When you send ^F and no card is there, it sends 'error' then 0x0a.

Each 160 bytes of column-binary have a 0x0a afterwards.

Column binary is offset 0x40 per character.

            LSBit             LSBit
12 11 0 1 2 3       4 5 6 7 8 9


I'm thinking of that "universal format" Dave mentioned, something like:

$BEGIN CU01-Diagnostic-Card-Deck.CARD
$TITLE CU01 Diagnostic
$MACHINE IBM 1620
$DATE-READ-IN 1999-Oct-27
$TIME-READ-IN 11:23 AM PT (GMT -8)
$DATE-ORIGINAL 1959-Jul-4
$DATA-FORMAT 80 Column Binary / Offset ASCII / Offset = octal 060
$DATA-CHARACTER-CODING 1620-BCD         <<- These types would be standardized
$BEGIN-PICTURE-OF-TOP-OF-CARD-DECK
  $PICTURE-FORMAT GIF/ASCII/base64
  $PICTURE-SIZE 12232 8-bit bytes
  $PICTURE-CRC-32 0x12345678
  $BEGIN-PICTURE-DATA
  abc1399adef .....
  $END-PICTURE-DATA
$END-PICTURE-OF-TOP-OF-CARD-DECK
$BEGIN-CARD-DATA                 <<- Note! Or Paper Tape or Mag Tape or ....
0a<=CDA.......
$END-CARD-DATA
$END CU01-Diagnostic-Card-Deck.CARD
$CRC-OF-THIS-FILE-TO-THIS-POINT 87654321
$EOF


Or something like that.  Something that's NOT binary (can be included in
emails seamlessly), where CR/LF are treated as nulls,  human readable,
and easy to machine parse.

We may want to see if we can extend or use MIME, too.

-Mike C


On Wed, 27 Oct 1999, Lee Courtney wrote:

> Mike,
>
> In addition to talking with Jim Alexander at Sequoia-Pacific, you might also
> contact Tom Benton of the Benton Company. Tom designed the interface box and
> was going to to send us a 'manual' a couple weeks ago. I left a message for
> him this morning gently reminding him we hadn't received it yet. I suspect
> the manual will be no more than a copy of his notes, but since he built the
> box it will probably be useful. He is in S. California and can be reached at
> 949-361-2751. Call or email me with any questions, comments, etc. Have fun!
>
> Lee C.



******************************************************************************/



// End of "ReadCard.cpp"
