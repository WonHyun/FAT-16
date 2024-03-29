/***********************************************************/
// 다음은 사용자 함수를 호출하는 루틴임 지우지 말것!
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

/*===== << 여기부터 사용자 프로그램을 작성함 >> =====*/

/***********************************************************/
// [1] : SD Access
/***********************************************************/

#if 0

/* 512B 단위의 섹터를 인쇄해주는 디버깅용 함수 */

static void sector_printf(void* buf)
{
	int i, j, k;

	for (i = 0; i < (128 / 8); i++)
	{
		printf("[%3d]", i * 32);

		for (j = 0; j < 8; j++)
		{
			for (k = 0; k < 4; k++)
			{
				printf("%.2X", ((char*)buf)[i * 32 + j * 4 + k]);
			}
			printf(" ");
		}
		printf("\n");
	}
}

void main(void)
{
	int i, k;
	int buf[512 / 4];

	printf("SD Card Access Test\n");
	if (SD_Check_Card() != SD_SUCCESS) printf("SD Card를 삽입해 주십시오\n");

	for (;;)
	{
		if (SD_Check_Card() == SD_SUCCESS) break;
	}

	printf("SD_Check_Card() Passed!\n");

	i = SD_Init();
	printf("SD_Init() Passed!\n");
	if (i != SD_SUCCESS) printf("Init Err : [%d]\n", i);

	Key_Poll_Init();

	for (k = 0; k < 2; k++)
	{
		printf("아무 KEY를 누르면 시작합니다\n");

		Key_Wait_Key_Pressed();
		Key_Wait_Key_Released();

		SD_Read_Sector(k, 1, (U8*)buf);
		printf("\n[%d]번 Sector Read!\n", k);
		sector_printf(buf);
	}
}

#endif

/***********************************************************/
// [2] : BMP Display 함수
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

	if ((add <= 0x30000000) || (add >= 0x32000000))
	{
		add = 0x31000000;
	}

	printf("%#x\n", add);
	Lcd_Draw_BMP_File_24bpp(0, 0, (void*)add);
}

#endif

/***********************************************************/
// [3] : FAT16 구현
/***********************************************************/

#if 01

#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#pragma pack(push, 1)

/* MBR을 모델링하기 위한 구조체 타입 선언 */
typedef struct {
	unsigned char rsvd[8];
	unsigned int LBA_Start;
	unsigned int Size;
}PARTITION;

typedef struct {
	unsigned char rsvd[446];
	PARTITION part[4];
	unsigned short signature;
}MBR;
/* BR을 모델링하기 위한 구조체 타입 선언 */
typedef struct {
	unsigned char boot[11];
	unsigned short bytepersector;
	unsigned char sectorpercluster;
	unsigned short rsvdsectorcnt;
	unsigned char nooffats;
	unsigned short rootenterycnt;
	unsigned char rsvd1[3];
	unsigned short fat16size;
	unsigned char rsvd2[8];
	unsigned int totalsector;
}BR;

/* File Entry 분석을 위한 구조체, 공용체 타입 선언 */
typedef struct {
	unsigned char r : 1;
	unsigned char h : 1;
	unsigned char s : 1;
	unsigned char v : 1;
	unsigned char d : 1;
	unsigned char a : 1;
}fat_file;
typedef struct {
	unsigned char ln : 4;
}long_file;
typedef union {
	fat_file n;
	long_file l;
	unsigned char c;
}FATATTR;
/* 시간 포맷을 위한 비트필드 구조체 선언 */
typedef struct {
	unsigned short sec : 5;
	unsigned short min : 6;
	unsigned short hour : 5;
}FATTIME;
/* 날짜 포맷을 위한 비트필드 구조체 선언 */
typedef struct {
	unsigned short day : 5;
	unsigned short mon : 4;
	unsigned short year : 7;
}FATDATE;
/* 하나의 32B Entry 분석을 위한 구조체 */
typedef struct
{/* Entry의 각 멤버를 설계한다 */
	unsigned char name[11];
	FATATTR attr;
	unsigned char rsvd;
	unsigned char creationtimetenth;
	FATTIME creationtime;
	FATDATE creationdate;
	FATDATE lastaccessdate;
	unsigned short firstclusterhigh;
	FATTIME lastwritetime;
	FATDATE lastwritedate;
	unsigned short firstclusterlow;
	unsigned int filesize;
}ENTRY;

#pragma pack(pop)

/* MBR, BR 분석을 통하여 획득하여 저장해야 하는 정보들 */

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

/* 하부 설계되는 함수 목록 */

static void listing_file(void);
static ENTRY* search_file(int id);
static int check_file_type(ENTRY* file);
static void read_file(ENTRY* file, void* data);
static void sector_printf(void* buf);

/* 512B 단위의 섹터를 읽어서 저장하기 위한 임시 버퍼 */

static unsigned int buf[512 / 4];

/* 512B 단위의 섹터를 인쇄해주는 디버깅용 함수 */

static void sector_printf(void* buf)
{
	int i, j, k;

	for (i = 0; i < (128 / 8); i++)
	{
		printf("[%3d]", i * 32);

		for (j = 0; j < 8; j++)
		{
			for (k = 0; k < 4; k++)
			{
				printf("%.2X", ((char*)buf)[i * 32 + j * 4 + k]);
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

	if (SD_Check_Card() != SD_SUCCESS) printf("Insert SD Card Please!\n");

	for (;;)
	{
		if (SD_Check_Card() == SD_SUCCESS) break;
	}

	i = SD_Init();
	if (i != SD_SUCCESS) printf("Init Err : [%d]\n", i);

	/* 사용자 코드 */

	{

#if 01
		SD_Read_Sector(0, 1, (U8*)buf);
		//sector_printf(buf);
		/* MBR에서 parameter.lba_start 값을 읽어서 저장한다 */
		parameter.lba_start = ((MBR*)buf)->part[0].LBA_Start;
		//parameter.lba_start = ((char*)buf)[454] + (((char*)buf)[455]<<8)
		//		+ (((char*)buf)[456]<<16) + (((char*)buf)[457]<<24);

		printf("LBA_Start = %d\n", parameter.lba_start);

#endif

#if 01
		SD_Read_Sector(parameter.lba_start, 1, (U8*)buf);
		//sector_printf(buf);
		/* BR에서 정보들을 읽어서 parameter 구조체 멤버들에 저장한다 */
		parameter.byte_per_sector = ((BR*)buf)->bytepersector;
		parameter.sector_per_cluster = ((BR*)buf)->sectorpercluster;
		parameter.fat0_start = parameter.lba_start + ((BR*)buf)->rsvdsectorcnt;
		parameter.root_start = parameter.fat0_start +
			((BR*)buf)->fat16size * ((BR*)buf)->nooffats;
		parameter.root_sector_count = ((BR*)buf)->rootenterycnt * 32 / parameter.byte_per_sector;
		parameter.file_start = parameter.root_start + parameter.root_sector_count;

		printf("byte_per_sector = %d\n", parameter.byte_per_sector);
		printf("sector_per_cluster = %d\n", parameter.sector_per_cluster);
		printf("fat0_start = %d\n", parameter.fat0_start);
		printf("root_start = %d\n", parameter.root_start);
		printf("file_start = %d\n", parameter.file_start);

#endif
		for (;;)
		{
			ENTRY* file;
			int num;
			int r;
#if 01
			/* ROOT 분석 & 파일 인쇄 */
			listing_file();

			printf("File Number(Exit => 999)? ");
			num = Uart_GetIntNum();
#endif

#if 01
			if (num == 999) break;

			/* 원하는 번호의 파일 탐색 */
			file = search_file(num);
			if ((file == (ENTRY*)0))
			{
				printf("Not Supported File\n");
				continue;
			}

			/* 제대로 탐색되었는지 확인하기 위한 이름 인쇄 */
			for (i = 0; i < 8; i++) printf("%c", ((char*)file)[i]);
			printf(".");
			for (i = 8; i < (8 + 3); i++) printf("%c", ((char*)file)[i]);
			printf("\n");

#endif

#if 01
			/* C, TXT, BMP 파일인지 확인 */
			r = check_file_type(file);
			printf("File Type = %d\n", r);
#endif

#if 01
			if (r != 0)
			{
				char* data;
				int size = file->filesize;

				/* 파일 크기를 초과하는 섹터 단위의 메모리로 할당을 받아야 한다 */
				num = size + parameter.byte_per_sector;
				//num = (size + parameter.byte_per_sector-1) & ~(parameter.byte_per_sector-1);
				data = malloc(num);

				/* 파일 데이터 읽기 */
				read_file(file, data);

				switch (r)
				{
				case 1:
				case 2:
					for (i = 0; i < size; i++) printf("%c", ((char*)data)[i]);
					printf("\n");
					break;
				case 3:
					Lcd_Clr_Screen(0xf800);
					Lcd_Draw_BMP_File_24bpp(0, 0, (void*)data);
					break;
				default:
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

static void entryprint(int num, ENTRY* buf) {
	int i;
	printf("[%03d] ", num);
	for (i = 0; i < 8; i++) printf("%c", buf->name[i]);
	printf(".");
	for (i = 8; i < 11; i++) printf("%c", buf->name[i]);
	printf(" 0x%x ", buf->attr.c);
	printf("%02d/%02d/%02d ", buf->creationdate.year + 1980,
		buf->creationdate.mon, buf->creationdate.day);
	printf("%02d:%02d:%02d ", buf->creationtime.hour,
		buf->creationtime.min, buf->creationtime.sec * 2);
	printf("%5d %10d\n", buf->firstclusterlow, buf->filesize);
}
static void listing_file(void) {
	int i, j, num = 1;
	/* 타이틀 인쇄 => 임의 변경 가능 */
	printf("[NUM] [NAME   .EXT] [AT] [  DATE  ] [TIME] [CLUST] [ SIZE ]\n");
	printf("=============================================================\n");

	for (i = 0; i < parameter.root_sector_count; i++)
	{
		SD_Read_Sector(parameter.root_start + i, 1, (U8*)buf);

		for (j = 0; j < (parameter.byte_per_sector / 32); j++)
		{
			/* Name[0]가 0x0이면 인쇄 종료, 0x05, 0xE5이면 삭제파일 Skip */
			if (((ENTRY*)buf)[j].name[0] == 0x00) goto EXIT;//인쇄 종료
			if (((ENTRY*)buf)[j].name[0] == 0x05) continue;//삭제파일 Skip
			if (((ENTRY*)buf)[j].name[0] == 0xE5) continue;//삭제파일 Skip
			/* 파일 속성이 0x3F 또는 0x0F long file name 이므로 Skip */
			if (((ENTRY*)buf)[j].attr.l.ln == 0xf) continue;
			/* Entry 정보 인쇄 */
			entryprint(num, &((ENTRY*)buf)[j]);
			/* 인쇄되는 파일 또는 폴더 마다 맨 앞에 1번부터 1씩 증가하며 번호를 인쇄한다 */
			num++;
		}
	}
EXIT:
	printf("=============================================================\n");
}

static ENTRY* search_file(int id)
{
	int i, j, num = 1;

	for (i = 0; i < parameter.root_sector_count; i++)
	{
		SD_Read_Sector(parameter.root_start + i, 1, (U8*)buf);

		for (j = 0; j < (parameter.byte_per_sector / 32); j++)
		{
			/* Name[0]가 0x0이면 NULL 포인터 리턴, 0x05, 0xE5이면 Skip */
			if (((ENTRY*)buf)[j].name[0] == 0x00) return (ENTRY*)0;//인쇄 종료
			if (((ENTRY*)buf)[j].name[0] == 0x05) continue;//삭제파일 Skip
			if (((ENTRY*)buf)[j].name[0] == 0xE5) continue;//삭제파일 Skip
			/* 파일 속성이 0x3F 또는 0x0F long file name 이므로 Skip */
			if (((ENTRY*)buf)[j].attr.l.ln == 0xf) continue;
			/* 정상 Entry(파일, 폴더)일 경우 num이 원하는 ID인지 확인 */
			/* num이 id보다 작으면 num 증가만 하고 계속 탐색 반복, 같으면 Entry 시작 주소 리턴 */
			if (num == id) return &((ENTRY*)buf)[j];
			num++;
		}
	}

	return (ENTRY*)0;
}

static int check_file_type(ENTRY* file)
{
	static char* cmp[] = { "C  ", "TXT", "BMP" };
	int i;
	/* 리턴 값 : C 파일 => 1, TXT 파일 => 2, BMP 파일 => 3, 그외 => 0 리턴 */
	if (file->attr.n.d == 1) return 0;//directory 임
	if (file->attr.n.a == 0) return 0;//file 아님
	for (i = 8; i < 11; i++) {
		if (('a' <= file->name[i]) && (file->name[i] <= 'z')) {
			file->name[i] = file->name[i] - 'a' + 'A';//소문자를 대문자로 변경
		}
	}
	for (i = 0; i < 3; i++) {
		if (strncmp((char*)& file->name[8], cmp[i], 3) == 0) return i + 1;
	}
	return 0;
}

static void read_file(ENTRY* file, void* data)
{/* 주어진 Entry의 실제 데이터를 읽어서 data 주소에 저장한다 */
	U16* fatbuf = malloc(parameter.byte_per_sector);
	int prefatsecoffset = -1;//이전에 읽은 fat 테이블 offset(초기값 올수없는 값, 최초는 읽어야 하므로)
	int curfatsecoffset;//현재 읽어야 하는 fat 테이블 offset
	int nextclusternum = file->firstclusterlow;//다음에 읽을 cluster 번호
	int curclusternum;//현재 읽어야할 cluster 번호
	int addrsec;
	int size = file->filesize;//읽어야할 파일 크기
	U8* databuf = data;//저장할 주소
	int i, cnt;
	while (nextclusternum != 0xFFFF) {
		curclusternum = nextclusternum;
		curfatsecoffset = curclusternum / (parameter.byte_per_sector / 2);//시작 클러스터 번호에 해당하는 섹터
		if (prefatsecoffset != curfatsecoffset) {//다른 섹터면 읽어야 함
			prefatsecoffset = curfatsecoffset;
			SD_Read_Sector(parameter.fat0_start + curfatsecoffset, 1, (U8*)fatbuf);
		}
		nextclusternum = fatbuf[curclusternum % (parameter.byte_per_sector / 2)];
		if (nextclusternum == 0xFFFF) {//마지막 cluster 는 잔량만 읽기
			cnt = (size + parameter.byte_per_sector - 1) / parameter.byte_per_sector;
		}
		else {
			cnt = parameter.sector_per_cluster;
		}
		addrsec = parameter.file_start + (parameter.sector_per_cluster * (curclusternum - 2));
		for (i = 0; i < cnt; i++) {
			SD_Read_Sector(addrsec + i, 1, databuf);
			databuf += parameter.byte_per_sector;//주소 증가
			size -= parameter.byte_per_sector;//크기 감소
		}
	}
	free(fatbuf);
}

#endif

