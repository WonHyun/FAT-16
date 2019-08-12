/***********************************************************/
// ������ ����� �Լ��� ȣ���ϴ� ��ƾ�� ������ ����!
/***********************************************************/

#include "device_driver.h"
#define printf		Uart_Printf
#define main		User_Main

void User_Main(void);

void Main(void)
{
	MMU_Init();
	Uart_Init(115200);	
	printf("\n=================================\n");
	main();
	printf("=================================\n");
}

/*===== << ������� ����� ���α׷��� �ۼ��� >> =====*/

/***********************************************************/
// [1] : SD Access
/***********************************************************/

#if 0

/* 512B ������ ���͸� �μ����ִ� ������ �Լ� */

static void sector_printf(void * buf)
{
	int i, j, k;

	for(i=0; i<(128/8); i++)
	{
		printf("[%3d]", i*32);

		for(j=0; j<8; j++)
		{
			for(k=0; k<4; k++)
			{
				printf("%.2X", ((char *)buf)[i*32+j*4+k]);
			}
			printf(" ");
		}
		printf("\n");
	}
}

void main(void)
{
	int i, k;
	int buf[512/4];

	printf("SD Card Access Test\n");
	if(SD_Check_Card() != SD_SUCCESS) printf("SD Card�� ������ �ֽʽÿ�\n");

	for(;;)
	{
		if(SD_Check_Card() == SD_SUCCESS) break;
	}

	printf("SD_Check_Card() Passed!\n");

	i=SD_Init();
	printf("SD_Init() Passed!\n");
	if(i != SD_SUCCESS) printf("Init Err : [%d]\n", i);

	Key_Poll_Init();

	for(k=0;k<2;k++)
	{
		printf("�ƹ� KEY�� ������ �����մϴ�\n");

		Key_Wait_Key_Pressed();
		Key_Wait_Key_Released();

		SD_Read_Sector(k, 1, (U8 *)buf);
		printf("\n[%d]�� Sector Read!\n", k);
		sector_printf(buf);
	}
}

#endif

/***********************************************************/
// [2] : BMP Display �Լ�
/***********************************************************/

#if 0

void main(void)
{
	int add;
	Lcd_Graphic_Init();
	Lcd_Clr_Screen(0xf800);

	printf("24bpp BMP Display\n");

	printf("Address of BMP file, Default=0x31000000 => ");

	add = Uart_GetIntNum();

	if((add <= 0x30000000) || (add >= 0x32000000))
	{
		add = 0x31000000;
	}

	printf("%#x\n", add);
	Lcd_Draw_BMP_File_24bpp(0,0,(void *)add);
}

#endif

/***********************************************************/
// [3] : FAT16 ����
/***********************************************************/

#if 01

#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#pragma pack(push, 1)

/* MBR�� �𵨸��ϱ� ���� ����ü Ÿ�� ���� */

typedef struct 
{
	unsigned char rsvd[8];
	unsigned int LBA_Start;
	unsigned int Size;
} PARTITION;

typedef struct {
	unsigned char rsvd[446];
	PARTITION part[4];
	unsigned short signature;
} MBR;

/* BR�� �𵨸��ϱ� ���� ����ü Ÿ�� ���� */


/* File Entry �м��� ���� ����ü, ����ü Ÿ�� ���� */

/* �ð� ������ ���� ��Ʈ�ʵ� ����ü ���� */
/* ��¥ ������ ���� ��Ʈ�ʵ� ����ü ���� */

/* �ϳ��� 32B Entry �м��� ���� ����ü */

typedef struct
{
	/* Entry�� �� ����� �����Ѵ� */


}ENTRY;

#pragma pack(pop)

/* MBR, BR �м��� ���Ͽ� ȹ���Ͽ� �����ؾ� �ϴ� ������ */

static struct _parameter
{
	unsigned int lba_start;
	unsigned short byte_per_sector;
	unsigned char sector_per_cluster;
	unsigned int root_sector_count;
	unsigned int fat0_start;
	unsigned int root_start;
	unsigned int file_start;
}parameter;

/* �Ϻ� ����Ǵ� �Լ� ��� */

static void listing_file(void);
static ENTRY * search_file(int id);
static int check_file_type(ENTRY * file);
static void read_file(ENTRY * file, void * data);
static void sector_printf(void * buf);

/* 512B ������ ���͸� �о �����ϱ� ���� �ӽ� ���� */

static unsigned int buf[512/4];

/* 512B ������ ���͸� �μ����ִ� ������ �Լ� */

static void sector_printf(void * buf)
{
	int i, j, k;

	for(i=0; i<(128/8); i++)
	{
		printf("[%3d]", i*32);

		for(j=0; j<8; j++)
		{
			for(k=0; k<4; k++)
			{
				printf("%.2X", ((char *)buf)[i*32+j*4+k]);
			}
			printf(" ");
		}
		printf("\n");
	}
}

void main(void)
{
	int i;

	Lcd_Graphic_Init();
	Lcd_Clr_Screen(0xf800);

	if(SD_Check_Card() != SD_SUCCESS) printf("Insert SD Card Please!\n");

	for(;;)
	{
		if(SD_Check_Card() == SD_SUCCESS) break;
	}

	i=SD_Init();
	if(i != SD_SUCCESS) printf("Init Err : [%d]\n", i);

	/* ����� �ڵ� */

	{

#if 01
		SD_Read_Sector(0, 1, (U8 *)buf);

		/* MBR���� parameter.lba_start ���� �о �����Ѵ� */

		((MBR*)buf)->part[0].LBA_Start;

		printf("LBA_Start = %d\n", parameter.lba_start);

#endif

#if 0
		SD_Read_Sector(parameter.lba_start, 1, (U8 *)buf);

		/* BR���� �������� �о parameter ����ü ����鿡 �����Ѵ� */




		printf("byte_per_sector = %d\n", parameter.byte_per_sector);
		printf("sector_per_cluster = %d\n", parameter.sector_per_cluster);
		printf("fat0_start = %d\n", parameter.fat0_start);
		printf("root_start = %d\n", parameter.root_start);
		printf("file_start = %d\n", parameter.file_start);

#endif

		for(;;)
		{
			ENTRY * file;
			int num;
			int r;

#if 0
			/* ROOT �м� & ���� �μ� */

			listing_file();
			
			printf("File Number(Exit => 999)? ");
			num = Uart_GetIntNum();
#endif

#if 0
			if(num == 999) break;

			/* ���ϴ� ��ȣ�� ���� Ž�� */

			file = search_file(num);

			if((file == (ENTRY *)0))
			{
				printf("Not Supported File\n");
				continue;
			}

			/* ����� Ž���Ǿ����� Ȯ���ϱ� ���� �̸� �μ� */

			for(i = 0; i < 8; i++) printf("%c", ((char *)file)[i]);
			printf(".");
			for(i = 8; i < (8+3); i++) printf("%c", ((char *)file)[i]);
			printf("\n");

#endif

#if 0
			/* C, TXT, BMP �������� Ȯ�� */

			r = check_file_type(file);

			printf("File Type = %d\n", r);

#endif

#if 0
			if(r != 0)
			{
				char * data;
				int size = file->size;

				/* ���� ũ�⸦ �ʰ��ϴ� ���� ������ �޸𸮷� �Ҵ��� �޾ƾ� �Ѵ� */
				num = 
				data = malloc(num);
				
				/* ���� ������ �б� */

				read_file(file, data);

				switch(r)
				{
					case 1 :
					case 2 :
						for(i = 0; i < size; i++) printf("%c", ((char *)data)[i]);
						printf("\n");
						break;
					case 3 :
						Lcd_Clr_Screen(0xf800);
						Lcd_Draw_BMP_File_24bpp(0,0,(void *)data);
						break;
					default :
						break;
				}

				free(data);
			}

			else printf("Not Supported File\n");

#endif

		}

		printf("BYE!\n");
	}
}

static void listing_file(void)
{
	int i, j, num = 1;

	/* Ÿ��Ʋ �μ� => ���� ���� ���� */

	printf("[NUM] [NAME .EXT] [AT] [  DATE  ] [TIME] [CLUST] [ SIZE ]\n");
	printf("=============================================================\n");

	for(i = 0; i < parameter.root_sector_count; i++)
	{
		SD_Read_Sector(parameter.root_start + i, 1, (U8 *)buf);

		for(j = 0; j < (parameter.byte_per_sector / 32); j++)
		{
			/* Name[0]�� 0x0�̸� �μ� ����, 0x05, 0xE5�̸� �������� Skip */
			/* ���� �Ӽ��� 0x3F �Ǵ� 0x0F long file name �̹Ƿ� Skip */
			/* Entry ���� �μ� */
			/* �μ�Ǵ� ���� �Ǵ� ���� ���� �� �տ� 1������ 1�� �����ϸ� ��ȣ�� �μ��Ѵ� */
		}
	}

	printf("=============================================================\n");
}

static ENTRY * search_file(int id)
{
	int i, j, num = 1;

	for(i = 0; i < parameter.root_sector_count; i++)
	{
		SD_Read_Sector(parameter.root_start + i, 1, (U8 *)buf);

		for(j = 0; j < (parameter.byte_per_sector / 32); j++)
		{
			/* Name[0]�� 0x0�̸� NULL ������ ����, 0x05, 0xE5�̸� Skip */
			/* ���� �Ӽ��� 0x3F �Ǵ� 0x0F long file name �̹Ƿ� Skip */
			/* ���� Entry(����, ����)�� ��� num�� ���ϴ� ID���� Ȯ�� */
			/* num�� id���� ������ num ������ �ϰ� ��� Ž�� �ݺ�, ������ Entry ���� �ּ� ���� */
		}
	}

	return (ENTRY *)0;
}

static int check_file_type(ENTRY * file)
{
	/* ���� �� : C ���� => 1, TXT ���� => 2, BMP ���� => 3, �׿� => 0 ���� */

	return 0;
}

static void read_file(ENTRY * file, void * data)
{
	/* �־��� Entry�� ���� �����͸� �о data �ּҿ� �����Ѵ� */

}

#endif

