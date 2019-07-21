#include "p24FJ64GA004.h"
#include "spi.h"
#include "pps.h"

// �R���t�B�M�����[�V���� �r�b�g�̐ݒ�
_CONFIG1( JTAGEN_OFF & GCP_OFF & GWRP_OFF &	BKBUG_ON
	 & COE_OFF & ICS_PGx1& FWDTEN_OFF ) 
_CONFIG2( IESO_OFF & FNOSC_FRCPLL & FCKSM_CSDCMD
	 & OSCIOFNC_OFF & IOL1WAY_OFF & I2C1SEL_PRI & POSCMOD_NONE)
	 
// ADXL-345���W�X�^�A�h���X
#define	BW_RATE		0x2C	//Data rate and power mode control
#define POWER_CTL	0x2D	//Power Control Register
#define	DATA_FORMAT	0x31	//Data format control
#define DATAX0		0x32	//X-Axis Data 0

// SPI1�ʐM�Ŏ�M�����f�[�^�o�b�t�@
static char Adxl345_buf[10];

// �x���֐�	 
void Delay(unsigned int CNT)
{
	unsigned int i,j;
	for(i=0; i<CNT; i++)
		for(j=0; j<100; j++);
}

// SPI��������
void WriteRegister(char registerAddress, char value)
{
	unsigned char dummy = 0;

	// SPI�J�n����LOW
	_RC4		= 0;

	// �������݃A�h���X�w��
	putsSPI1(1, &registerAddress);
	Delay(5);
	dummy = getcSPI1();

	// ��������
	putsSPI1(1, &value);
	Delay(5);
	dummy = getcSPI1();	

	// SPI�I������HIGH
	_RC4		= 1;
}

// SPI�ǂݍ���
void ReadRegister(char registerAddress, int numBytes)
{
	char dat[10];
	unsigned int i = 0;
	unsigned char dummy = 0x00;
	char dummyDat;

	// �����o�C�g�ǂݏo�����̂���@
	char address = 0x80 | registerAddress;
	if(numBytes > 1)address = address | 0x40;
	
	// SPI�J�n����LOW
	_RC4		= 0;

	// �ǂݏo����A�h���X�̎w��
	putsSPI1(1, &address);
	Delay(5);
	dummyDat = getcSPI1();

	Delay(5);

	// �l�̓ǂݏo��
	for(i = 0; i < numBytes; i++)
	{
		putsSPI1(1, &dummy);
		Delay(5);
		dat[i] = getcSPI1();
		Delay(5);
	}

	// SPI�I������HIGH
	_RC4		= 1;

	for(i = 0; i < numBytes; i++)
	{
		Adxl345_buf[i] = dat[i];
	}
}

int main(void)
{
	// �S�|�[�g�f�B�W�^���Ɏw��
	AD1PCFG = 0xFFFF;
	
	// LED�|�[�g�ݒ�
	_TRISB10	= 0;	// RB10 �o��
	
	// �|�[�g�̐ݒ�
	PPSInput (PPS_SDI1	,PPS_RP5);
	PPSOutput(PPS_RP6	,PPS_SDO1);
	PPSOutput(PPS_RP21	,PPS_SCK1OUT);
	PPSOutput(PPS_RP20	,PPS_SS1OUT);
	
	// SPI�̏�����
	_TRISC4		= 0;	// RP20/RC4 �o��
	_RC4		= 1;	// CS�𗧂Ă�
	
	CloseSPI1();	// �O�̂���
	
	OpenSPI1(
			// config1
			ENABLE_SCK_PIN &		// SPICLK��L���ɂ���
			ENABLE_SDO_PIN &		// SDO�s�������W���[���Ő��䂷��
			SPI_MODE8_ON &			// �f�[�^����8bit�Ƃ���
			SPI_SMP_ON &			// �f�[�^�̃G�b�W�ŃT���v�����O����
			SPI_CKE_OFF &			// Idle->Acrive�J�ڎ��Ƀf�[�^��ς���
			SLAVE_ENABLE_OFF &		// SS�����W���[���Ő��䂵�Ȃ�
			CLK_POL_ACTIVE_LOW &	// CLK��Active = Low�Ƃ���
			MASTER_ENABLE_ON &		// Master�Ƃ��Ďg��
			SEC_PRESCAL_2_1 &
			PRI_PRESCAL_1_1,

			// config2
			FRAME_ENABLE_OFF,
			
			// config3
			SPI_ENABLE &
			SPI_IDLE_CON &
			SPI_RX_OVFLOW_CLR
		);
		
	// ���荞�݋����Ȃ�
	ConfigIntSPI1(SPI_INT_DIS & SPI_INT_PRI_6);	
	
	// ADXL-345������
	// �����x�Z���T���W�X�^�ݒ�
	WriteRegister(DATA_FORMAT, 0x03);  // +-16g 10bit
	WriteRegister(BW_RATE, 0x19);
	WriteRegister(POWER_CTL, 0x08);  //Measurement mode
	
	int Adxl345_x, Adxl345_y, Adxl345_z;
	double Adxl345_xg, Adxl345_yg, Adxl345_zg;
	
	// ���C�����[�v
	while(1)
	{
		// �����x��M
		ReadRegister(DATAX0, 6);
		
		// �����x�v�Z
	  	Adxl345_x = ((int)Adxl345_buf[1]<<8)|(int)Adxl345_buf[0];
	  	Adxl345_y = ((int)Adxl345_buf[3]<<8)|(int)Adxl345_buf[2];
	  	Adxl345_z = ((int)Adxl345_buf[5]<<8)|(int)Adxl345_buf[4];
	   
	  	Adxl345_xg = Adxl345_x * 0.03125;
	  	Adxl345_yg = Adxl345_y * 0.03125;
	  	Adxl345_zg = Adxl345_z * 0.03125;
	  	
	  	// Z������5g�ȏ�̉����x�����m������LED�_��
	  	if(Adxl345_zg >= 5.0)
	  	{
			LATBbits.LATB10 = 0;
		}
	}
	
	CloseSPI1();
	
	return 0;
}

