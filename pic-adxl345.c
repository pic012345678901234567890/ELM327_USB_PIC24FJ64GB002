#include "p24FJ64GA004.h"
#include "spi.h"
#include "pps.h"

// コンフィギュレーション ビットの設定
_CONFIG1( JTAGEN_OFF & GCP_OFF & GWRP_OFF &	BKBUG_ON
	 & COE_OFF & ICS_PGx1& FWDTEN_OFF ) 
_CONFIG2( IESO_OFF & FNOSC_FRCPLL & FCKSM_CSDCMD
	 & OSCIOFNC_OFF & IOL1WAY_OFF & I2C1SEL_PRI & POSCMOD_NONE)
	 
// ADXL-345レジスタアドレス
#define	BW_RATE		0x2C	//Data rate and power mode control
#define POWER_CTL	0x2D	//Power Control Register
#define	DATA_FORMAT	0x31	//Data format control
#define DATAX0		0x32	//X-Axis Data 0

// SPI1通信で受信したデータバッファ
static char Adxl345_buf[10];

// 遅延関数	 
void Delay(unsigned int CNT)
{
	unsigned int i,j;
	for(i=0; i<CNT; i++)
		for(j=0; j<100; j++);
}

// SPI書き込み
void WriteRegister(char registerAddress, char value)
{
	unsigned char dummy = 0;

	// SPI開始時にLOW
	_RC4		= 0;

	// 書き込みアドレス指定
	putsSPI1(1, &registerAddress);
	Delay(5);
	dummy = getcSPI1();

	// 書き込み
	putsSPI1(1, &value);
	Delay(5);
	dummy = getcSPI1();	

	// SPI終了時にHIGH
	_RC4		= 1;
}

// SPI読み込み
void ReadRegister(char registerAddress, int numBytes)
{
	char dat[10];
	unsigned int i = 0;
	unsigned char dummy = 0x00;
	char dummyDat;

	// 複数バイト読み出し時のお作法
	char address = 0x80 | registerAddress;
	if(numBytes > 1)address = address | 0x40;
	
	// SPI開始時にLOW
	_RC4		= 0;

	// 読み出し先アドレスの指定
	putsSPI1(1, &address);
	Delay(5);
	dummyDat = getcSPI1();

	Delay(5);

	// 値の読み出し
	for(i = 0; i < numBytes; i++)
	{
		putsSPI1(1, &dummy);
		Delay(5);
		dat[i] = getcSPI1();
		Delay(5);
	}

	// SPI終了時にHIGH
	_RC4		= 1;

	for(i = 0; i < numBytes; i++)
	{
		Adxl345_buf[i] = dat[i];
	}
}

int main(void)
{
	// 全ポートディジタルに指定
	AD1PCFG = 0xFFFF;
	
	// LEDポート設定
	_TRISB10	= 0;	// RB10 出力
	
	// ポートの設定
	PPSInput (PPS_SDI1	,PPS_RP5);
	PPSOutput(PPS_RP6	,PPS_SDO1);
	PPSOutput(PPS_RP21	,PPS_SCK1OUT);
	PPSOutput(PPS_RP20	,PPS_SS1OUT);
	
	// SPIの初期化
	_TRISC4		= 0;	// RP20/RC4 出力
	_RC4		= 1;	// CSを立てる
	
	CloseSPI1();	// 念のため
	
	OpenSPI1(
			// config1
			ENABLE_SCK_PIN &		// SPICLKを有効にする
			ENABLE_SDO_PIN &		// SDOピンをモジュールで制御する
			SPI_MODE8_ON &			// データ長を8bitとする
			SPI_SMP_ON &			// データのエッジでサンプリングする
			SPI_CKE_OFF &			// Idle->Acrive遷移時にデータを変える
			SLAVE_ENABLE_OFF &		// SSをモジュールで制御しない
			CLK_POL_ACTIVE_LOW &	// CLKをActive = Lowとする
			MASTER_ENABLE_ON &		// Masterとして使う
			SEC_PRESCAL_2_1 &
			PRI_PRESCAL_1_1,

			// config2
			FRAME_ENABLE_OFF,
			
			// config3
			SPI_ENABLE &
			SPI_IDLE_CON &
			SPI_RX_OVFLOW_CLR
		);
		
	// 割り込み許可しない
	ConfigIntSPI1(SPI_INT_DIS & SPI_INT_PRI_6);	
	
	// ADXL-345初期化
	// 加速度センサレジスタ設定
	WriteRegister(DATA_FORMAT, 0x03);  // +-16g 10bit
	WriteRegister(BW_RATE, 0x19);
	WriteRegister(POWER_CTL, 0x08);  //Measurement mode
	
	int Adxl345_x, Adxl345_y, Adxl345_z;
	double Adxl345_xg, Adxl345_yg, Adxl345_zg;
	
	// メインループ
	while(1)
	{
		// 加速度受信
		ReadRegister(DATAX0, 6);
		
		// 加速度計算
	  	Adxl345_x = ((int)Adxl345_buf[1]<<8)|(int)Adxl345_buf[0];
	  	Adxl345_y = ((int)Adxl345_buf[3]<<8)|(int)Adxl345_buf[2];
	  	Adxl345_z = ((int)Adxl345_buf[5]<<8)|(int)Adxl345_buf[4];
	   
	  	Adxl345_xg = Adxl345_x * 0.03125;
	  	Adxl345_yg = Adxl345_y * 0.03125;
	  	Adxl345_zg = Adxl345_z * 0.03125;
	  	
	  	// Z方向に5g以上の加速度を検知したらLED点灯
	  	if(Adxl345_zg >= 5.0)
	  	{
			LATBbits.LATB10 = 0;
		}
	}
	
	CloseSPI1();
	
	return 0;
}

