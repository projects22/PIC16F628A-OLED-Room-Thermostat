//OLED SSD1306 ROOM THERMOSTAT PIC16F628A
//MPLAB X v3.10 xc8 v1.45
//moty22.co.uk

#include <htc.h>
#include "oled_font.c"

#pragma config WDTE=OFF, MCLRE=OFF, BOREN=OFF, FOSC=INTOSCIO, CP=OFF, CPD=OFF, LVP=OFF

#define relay RA1 //pin18, increas setting
#define inc RB1 //pin7, increas setting
#define dec RB2 //pin8, decreas setting
#define SCL RA3 //pin2
#define SDA RA2 //pin1
#define si RB5		//sensor pin
#define si_in TRISB5=1		//sensor is input
#define si_out TRISB5=0		//sensor is output
#define _XTAL_FREQ 4000000

// prototypes
void command( unsigned char comm);
void oled_init();
void clrScreen();
void sendData(unsigned char dataB);
void startBit(void);
void stopBit(void);
void clock(void);
void drawChar2(char fig, unsigned char y, unsigned char x);
unsigned char reply_in (void);
void cmnd_w_in(unsigned char cmnd);
unsigned char sensor_rst_in(void);
void temp(void);
void setting (void);

unsigned char addr=0x78;  //0b1111000
int set=336, tempS;

void main(void) {
    	// PIC I/O init
    CMCON = 0b111;		//comparator off
    TRISA = 0b110001;   //SCL, SDA, relay outputs
	TRISB = 0b111111;		 //
    nRBPU = 0;  //pullup
	relay=0;
    SCL=1;
    SDA=1;
    
    __delay_ms(1000);
    oled_init();

    clrScreen();       // clear screen
    drawChar2(2, 0, 4);  //.
    drawChar2(19, 0, 7);  //° 
    drawChar2(13, 0, 8);  //C
    drawChar2(17, 6, 1);  //S
    drawChar2(16, 6, 2);  //E 
    drawChar2(18, 6, 3);  //T
    drawChar2(2, 6, 7);  //.
    setting();
    
    while (1) {
           //setting
        while(!inc){ 
            set +=8;
            if(set > 800) set=800;
            setting();
            __delay_ms(500);
        }
        while(!dec){
            set -=8;
            if(set < 0) set=0;
            setting();
            __delay_ms(500);
        }
        
        temp();
            //set relay
        if(tempS > 0){
            if(tempS < (set-4)){
               drawChar2(3, 3, 4);  //O
               drawChar2(15, 3, 5);  //N
               drawChar2(0, 3, 6);  //
               relay=1;
            }
            if(tempS > (set+4)){
               drawChar2(3, 3, 4);  //O
               drawChar2(14, 3, 5);  //F
               drawChar2(14, 3, 6);  //F
               relay=0;
            }       
        }else{
            drawChar2(3, 3, 4);  //O
            drawChar2(14, 3, 5);  //F
            drawChar2(14, 3, 6);  //F
            relay=0;
        }
        __delay_ms(1000);

	}
}

void setting (void){    //display setting
	unsigned char d[3];
	int Sdec;

    d[2]=((set & 15)*10)/16;
    Sdec=set/16;
    d[0]=(Sdec/10) %10; 	//digit on left
    d[1]=Sdec %10;	
    drawChar2(d[0]+3, 6, 5); 
    drawChar2(d[1]+3, 6, 6);
    drawChar2(d[2]+3, 6, 8); 
}

void temp(void){    //display temp
    int tempL, tempH;
    unsigned int temp;
	unsigned char dp[3], d[3], minus;//    
    int tDec;	//
        //read sensor
	sensor_rst_in();	//sensor init
	cmnd_w_in(0xCC);	//skip ROM command
	cmnd_w_in(0xBE);	//read pad command
	tempL=reply_in();	//LSB of temp
	tempH=reply_in();	//MSB of temp
	sensor_rst_in();
	sensor_rst_in();
	cmnd_w_in(0xCC);	//skip ROM command
	cmnd_w_in(0x44);	//start conversion command
    
    tempS = tempL + (tempH << 8);	//calculate temp

    if(tempS<0){
        minus=1; 
        temp=65536-tempS;   //convert to unsigned integer
    }else{
        minus=0;
        temp=(unsigned int)tempS;
    }
    
    d[2]=((temp & 15)*10)/16;
    tDec=temp/16;
    d[0]=(tDec/10) %10; 	//digit on left
    d[1]=tDec %10;	

 	if(minus){drawChar2(1, 0, 1);}else{drawChar2(0, 0, 1);}	//- sign
    drawChar2(d[0]+3, 0, 2); 
	drawChar2(d[1]+3, 0, 3);
	drawChar2(d[2]+3, 0, 5);
}

unsigned char reply_in (void){	//reply from sensor
	unsigned char ret=0,i;
	
	for(i=0;i<8;i++){	//read 8 bits
		si_out; si=0; __delay_us(2); si_in; __delay_us(6);
		if(si){ret += 1 << i;} __delay_us(80);	//output high=bit is high
	}
	si_in;
	return ret;
}

void cmnd_w_in(unsigned char cmnd){	//send command temperature sensor
	unsigned char i;
	
	for(i=0;i<8;i++){	//8 bits 
		if(cmnd & (1 << i)){si_out; si=0; __delay_us(2); si_in; __delay_us(80);}
		else{si_out; si=0; __delay_us(80); si_in; __delay_us(2);}	//hold output low if bit is low 
	}
	si_in;
	
}

unsigned char sensor_rst_in(void){		//reset the temperature
	
	si_out;
	si=0; __delay_us(600);
	si_in; __delay_us(100);
	__delay_us(600);
	return si;	//return 0 for sensor present
	
}

    //size 2 chars
void drawChar2(char fig, unsigned char y, unsigned char x)
{
    unsigned char i, line, btm, top;    //
    
    command(0x20);    // vert mode
    command(0x01);

    command(0x21);     //col addr
    command(13 * x); //col start
    command(13 * x + 9);  //col end
    command(0x22);    //0x22
    command(y); // Page start
    command(y+1); // Page end
    
        startBit();
        sendData(addr);            // address
        sendData(0x40);
        
        for (i = 0; i < 5; i++){
            line=font[5*(fig)+i];
            btm=0; top=0;
                // expend char    
            if(line & 64) {btm +=192;}
            if(line & 32) {btm +=48;}
            if(line & 16) {btm +=12;}           
            if(line & 8) {btm +=3;}
            
            if(line & 4) {top +=192;}
            if(line & 2) {top +=48;}
            if(line & 1) {top +=12;}        

             sendData(top); //top page
             sendData(btm);  //second page
             sendData(top);
             sendData(btm);
        }
        stopBit();
        
    command(0x20);      // horizontal mode
    command(0x00);    
        
}

void clrScreen()    //fill screen with 0
{
    unsigned char y, i;
    
    for ( y = 0; y < 8; y++ ) {
        command(0x21);     //col addr
        command(0); //col start
        command(127);  //col end
        command(0x22);    //0x22
        command(y); // Page start
        command(y+1); // Page end    
        startBit();
        sendData(addr);            // address
        sendData(0x40);
        for (i = 0; i < 128; i++){
             sendData(0x00);
        }
        stopBit();
    }    
}

//Software I2C
void sendData(unsigned char dataB)
{
    for(unsigned char b=0;b<8;b++){
       SDA=(dataB >> (7-b)) % 2;
       clock();
    }
    TRISA2=1;   //SDA input
    clock();
    __delay_us(5);
    TRISA2=0;   //SDA output

}

void clock(void)
{
   __delay_us(1);
   SCL=1;
   __delay_us(5);
   SCL=0;
   __delay_us(1);
}

void startBit(void)
{
    SDA=0;
    __delay_us(5);
    SCL=0;

}

void stopBit(void)
{
    SCL=1;
    __delay_us(5);
    SDA=1;

}

void command( unsigned char comm){
    
    startBit();
    sendData(addr);            // address
    sendData(0x00);
    sendData(comm);             // command code
    stopBit();
}

void oled_init() {
    
    command(0xAE);   // DISPLAYOFF
    command(0x8D);         // CHARGEPUMP *
    command(0x14);     //0x14-pump on
    command(0x20);         // MEMORYMODE
    command(0x0);      //0x0=horizontal, 0x01=vertical, 0x02=page
    command(0xA1);        //SEGREMAP * A0/A1=top/bottom 
    command(0xC8);     //COMSCANDEC * C0/C8=left/right
    command(0xDA);         // SETCOMPINS *
    command(0x12);   //0x22=4rows, 0x12=8rows
    command(0x81);        // SETCONTRAST
    command(0xFF);     //0x8F
    //next settings are set by default
//    command(0xD5);  //SETDISPLAYCLOCKDIV 
//    command(0x80);  
//    command(0xA8);       // SETMULTIPLEX
//    command(0x3F);     //0x1F
//    command(0xD3);   // SETDISPLAYOFFSET
//    command(0x0);  
//    command(0x40); // SETSTARTLINE  
//    command(0xD9);       // SETPRECHARGE
//    command(0xF1);
//    command(0xDB);      // SETVCOMDETECT
//    command(0x40);
//    command(0xA4);     // DISPLAYALLON_RESUME
 //   command(0xA6);      // NORMALDISPLAY
    command(0xAF);          //DISPLAYON

}



