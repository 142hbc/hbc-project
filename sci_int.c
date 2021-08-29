#include "DSP28x_Project.h"
#include "math.h"

void scic_echoback_init(void);
void scic_xmit(int a);
void scic_msg(unsigned char *msg);
void scic_fifo_init(void);
unsigned char LRC_Check(unsigned char *data,unsigned short length);
void Solve(void);
void ReadHoldRegisters(void);

unsigned char R[17];   //={0x3a,0x30,0x31,0x30,0x33,0x30,0x30,0x30,0x32,0x30,0x30,0x30,0x31,0x46,0x39,0x0d,0x0a};
unsigned char Cmd[14];
unsigned char Sdata[30];
unsigned char TS[30];
int RXflag=0,Txflag=0;
Uint16 i=0,a=0;
Uint16  ReceivedChar,Temp;

void main()
{


    InitSysCtrl();
    InitScicGpio();
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();
   // LoopCount = 0;
   // ErrorCount = 0;
   // scic_fifo_init();
    scic_echoback_init();
    for(i = 0; i <17; i++)
    {
            R[i] = 0;
            Cmd[i]= 0;
    }
    for(i=0;i<30;i++)
    {
      Sdata[i]=0;
      TS[i]=0;
     }
    i=0;
    for(;;)
   {

      while(ScicRegs.SCIRXST.bit.RXRDY != 1){}
      {

       ReceivedChar   = ScicRegs.SCIRXBUF.all;
       if(ReceivedChar == 0x3A)
       {
           RXflag=1;
       }
       if(RXflag == 1)
       R[i++] = ReceivedChar;
     }
      //scic_xmit(ReceivedChar);
      if(R[15] == 13 && R[16]== 10)
      Solve();
      if(Txflag == 1)
      {
    	  scic_msg(Sdata);
//      {
//    	  if(Sdata[a]!='\0')
//    		  Temp = Sdata[a++];
//    	  while (ScicRegs.SCICTL2.bit.TXRDY != 1){}
//    	  ScicRegs.SCITXBUF = Temp;
//      }
    	  Txflag = 0;
      }
      if(i==17)
      {
         for(i = 0; i <17; i++)
    	 R[i] = 0;
   	     i=0;
      }
  }

}

void scic_echoback_init(void)
{
    ScicRegs.SCICCR.all = 0x0007;
    ScicRegs.SCICTL1.all = 0x0003;
    ScicRegs.SCICTL2.all = 0x0003;
    ScicRegs.SCICTL2.bit.TXINTENA = 1;
    ScicRegs.SCICTL2.bit.RXBKINTENA = 1;

    ScicRegs.SCIHBAUD =0x0001;
    ScicRegs.SCILBAUD =0x00E7;

	ScibRegs.SCICTL2.bit.TXINTENA =1;
	ScibRegs.SCICTL2.bit.RXBKINTENA =1;
    ScicRegs.SCICTL1.all = 0x0023;
}
void scic_xmit(int a)
{
    //while (ScicRegs.SCIFFTX.bit.TXFFST != 0)
    while (ScicRegs.SCICTL2.bit.TXRDY != 1){}
    ScicRegs.SCITXBUF=a;

}
void scic_msg(unsigned char * msg)
{
    int k;
    k = 0;
    while(msg[k] != '\0')
    {
        scic_xmit(msg[k]);
        k++;
    }
}
//void Txdata(void)
//{
//    if(Txflag == 1)
//    scic_msg(Sdata);
//}
void Solve(void)
{   Uint16  a=0;
    for(a=1;a<14+1;a++)     //ASCII to 0x H
    {
       if(R[a]>='0'&& R[a]<='9')
       Cmd[a-1] = R[a]-0x30 ;
       else if(R[a]>='A'&& R[a]<='F')
       Cmd[a-1] = R[a] -0x41+ 0x0A;
       else
       Cmd[a-1] = R[a];
    }
    unsigned char lrc=0;
    lrc = LRC_Check(Cmd,12);
    switch (R[4])
    {
    case 0x33:
    {
        if(lrc ==(Cmd[12]*16+Cmd[13]))
        ReadHoldRegisters();
        break;
    }
    }
}
void ReadHoldRegisters(void)
{
    Uint16 j=0,f=0,LRC=0,len=0;
    Uint16 RigCount=0;
    RigCount =Cmd[10]*16+Cmd[11];

    for(j=0;j<5;j++)
    Sdata[j]=R[j];
    Sdata[5] = 0x30;
    Sdata[6] = 0x32;
    for(f=0;f<4*RigCount;f++)
    {Sdata[7+f]= 0x31;}
    len = 6+4*RigCount;
    for(f=1;f<7+4*RigCount;f++)     //ASCII to 0x H
        {
           if(Sdata[f]>='0'&& Sdata[f]<='9')
           TS[f-1] = Sdata[f]-0x30 ;
           else if(Sdata[f]>='A'&& Sdata[f]<='F')
           TS[f-1] = Sdata[f] -0x41+ 0x0A;
           else
           TS[f-1] = Sdata[f];
        }
    LRC = LRC_Check(TS,len);
    if((LRC/16)>9)
       Sdata[7+4*RigCount]  = (LRC/16-0x0A+0x41);
    else
        Sdata[7+4*RigCount] = (LRC/16+0x30);
    if((LRC%16)>9)
       Sdata[8+4*RigCount] = (LRC%16-0x0A+0x41);
    else
       Sdata[8+4*RigCount] = (LRC%16+0x30);
    Sdata[9+4*RigCount] = 0x0D;
    Sdata[10+4*RigCount] = 0x0A;
    //if (Sdata[10+4*RigCount] == )
    Txflag = 1;
}

unsigned char LRC_Check(unsigned char *data,unsigned short length)
{
    unsigned char g;
    Uint16 h=0;
    unsigned char result=0;
//    char lrcdata[length];
//    for(g=1;g<length+1;g++)
//    {
//        if(data[g]>0x40)
//            lrcdata[g-1]=data[g]-0x41+10;
//        else
//            lrcdata[g-1]=data[g]-0x30;
//    }
    for(g=0;g<length/2;g++)//数据（2n个字符）两两组成一个16进制的数值
    {
        h+=(data[2*g]*16+data[2*g+1]);
    }
//    h=h%256;
 //   h=0xFF-h;
    result=0xFF-h%256+1;
    return result;
}
