/*
 ============================================================================
 Project Name: ulc_example
 Name        : main.c
 Author      : d-logic (http://www.d-logic.net/nfc-rfid-reader-sdk/)
 Version     :
 Copyright   : 2017.
 Description : Project in C (Language standard: c99)
 Dependencies: uFR firmware - min. version x.y.z {define in ini.h}
               uFRCoder library - min. version x.y.z {define in ini.h}
 ============================================================================
 */

/* includes:
 * stdio.h & stdlib.h are included by default (for printf and LARGE_INTEGER.QuadPart (long long) use %lld or %016llx).
 * inttypes.h, stdbool.h & string.h included for various type support and utilities.
 * conio.h is included for windows(dos) console input functions.
 * windows.h is needed for various timer functions (GetTickCount(), QueryPerformanceFrequency(), QueryPerformanceCounter())
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#if __WIN32 || __WIN64
#	include <conio.h>
#	include <windows.h>
#elif linux || __linux__ || __APPLE__
#	define __USE_MISC
#	include <unistd.h>
#	include <termios.h>
#	undef __USE_MISC
#	include "conio_gnu.h"
#else
#	error "Unknown build platform."
#endif
#include <uFCoder.h>
#include "ini.h"
#include "uFR.h"
#include "utils.h"
//------------------------------------------------------------------------------
void usage(void);
void menu(char key);
UFR_STATUS NewCardInField(uint8_t sak, uint8_t *uid, uint8_t uid_size);
void authentication_3des(void);
void enable_card_halt(void);
void page_read(void);
void page_write(void);
void linear_read(void);
void linear_write(void);
void key_write(void);
bool open_sam(void);
void SamStoreKey(void);
//------------------------------------------------------------------------------

bool card_transceive_mode = false;
bool sam_used = false;

int main(void)
{
	char key;
	bool card_in_field = false;
	uint8_t old_sak = 0, old_uid_size = 0, old_uid[10];
	uint8_t sak, uid_size, uid[10];
	UFR_STATUS status;

	int mode = 0;
	printf("\nSelect reader opening mode:\n");
	printf(" (1) - Simple Reader Open\n");
	printf(" (2) - Advanced Reader Open\n");
	scanf("%d", &mode);
	fflush(stdin);
	if (mode == 1){
	status = ReaderOpen();
	} else if (mode == 2){
	   uint32_t reader_type = 1;
	   char port_name[1024] = "";
	   uint32_t port_interface = 2;
	   char open_args[1024] = "";
	   char str_interface[2] = "";

	   printf("Enter reader type:\n");
	   scanf("%d", &reader_type);
	   fflush(stdin);

	   printf("Enter port name:\n");
	   scanf("%s", port_name);
	   fflush(stdin);

	   printf("Enter port interface:\n");
	   scanf("%s", str_interface);
	   if (str_interface[0] == 'U'){
		   port_interface = 85;
	   } else if (str_interface[0] == 'T'){
		   port_interface = 84;
	   } else{
		   port_interface = atoi(str_interface);
	   }

	   fflush(stdin);

	   printf("Enter additional argument:\n");
	   scanf("%s", open_args);
	   fflush(stdin);

	   status = ReaderOpenEx(reader_type, port_name, port_interface, open_args);


	}
	else
	{
		printf("Invalid input. Press any key to quit the application...");
		getchar();
		return EXIT_FAILURE;
	}

#if __WIN32 || __WIN64
	Sleep(500);
#else // if linux || __linux__ || __APPLE__
	usleep(500000);
#endif

	if (!CheckDependencies())
	{
		ReaderClose();
		getchar();
		return EXIT_FAILURE;
	}

	printf(" --------------------------------------------------\n");
	printf("        uFR NFC reader successfully opened.\n");
	printf(" --------------------------------------------------\n");

	usage();
#if linux || __linux__ || __APPLE__
	_initTermios(0);
#endif
	do
	{
		while (!_kbhit())
		{
			if(card_transceive_mode)
				continue;

			status = GetCardIdEx(&sak, uid, &uid_size);
			switch (status)
			{
				case UFR_OK:
					if (card_in_field)
					{
						if (old_sak != sak || old_uid_size != uid_size || memcmp(old_uid, uid, uid_size))
						{
							old_sak = sak;
							old_uid_size = uid_size;
							memcpy(old_uid, uid, uid_size);
							NewCardInField(sak, uid, uid_size);
						}
					}
					else
					{
						old_sak = sak;
						old_uid_size = uid_size;
						memcpy(old_uid, uid, uid_size);
						NewCardInField(sak, uid, uid_size);
						card_in_field = true;
					}
					break;
				case UFR_NO_CARD:
					card_in_field = false;
					status = UFR_OK;
					break;
				default:
					ReaderClose();
					printf(" Fatal error while trying to read card, status is: 0x%08X\n", status);
					getchar();
#if linux || __linux__ || __APPLE__
					_resetTermios();
					tcflush(0, TCIFLUSH); // Clear stdin to prevent characters appearing on prompt
#endif
					return EXIT_FAILURE;
			}
#if __WIN32 || __WIN64
			Sleep(300);
#else // if linux || __linux__ || __APPLE__
			usleep(300000);
#endif
		}

		key = _getch();
		menu(key);
	}
	while (key != '\x1b');

	ReaderClose();
#if linux || __linux__ || __APPLE__
	_resetTermios();
	tcflush(0, TCIFLUSH); // Clear stdin to prevent characters appearing on prompt
#endif
	return EXIT_SUCCESS;
}
//------------------------------------------------------------------------------
void menu(char key)
{

	switch (key)
	{
		case '1':
			authentication_3des();
			break;

		case '2':
			enable_card_halt();
			break;

		case '3':
			page_read();
			break;

		case '4':
			page_write();
			break;

		case '5':
			linear_read();
			break;

		case '6':
			linear_write();
			break;

		case '7':
			key_write();
			break;

		case '8':
			SamStoreKey();
			break;

		case '\x1b':
			break;

		default:
			usage();
			break;
	}
}
//------------------------------------------------------------------------------
void usage(void)
{
		printf(" +------------------------------------------------+\n"
			   " |             ULTRALIGHT C SUPPORT               |\n"
			   " |          3DES AUTHENTICATION EXAMPLE           |\n"
			   " |              version "APP_VERSION"             |\n"
			   " +------------------------------------------------+\n"
			   "                              For exit, hit escape.\n");
		printf(" --------------------------------------------------\n");
		printf("  (1) - External authentication with 3DES key (card halt disable)\n"
			   "  (2) - Card halt enable\n"
			   "  (3) - Read Page\n"
			   "  (4) - Write Page\n"
			   "  (5) - Linear Read\n"
			   "  (6) - Linear Write\n"
			   "  (7) - Enter 3DES key into card\n"
			   "  (8) - Writing keys into SAM\n");
}
//------------------------------------------------------------------------------
UFR_STATUS NewCardInField(uint8_t sak, uint8_t *uid, uint8_t uid_size)
{
	UFR_STATUS status;
	uint8_t dl_card_type;

	status = GetDlogicCardType(&dl_card_type);
	if (status != UFR_OK)
		return status;

	printf(" \a-------------------------------------------------------------------\n");
	printf(" Card type: %s, sak = 0x%02X, uid[%d] = ", GetDlTypeName(dl_card_type), sak, uid_size);
	print_hex_ln(uid, uid_size, ":");
	printf(" -------------------------------------------------------------------\n");

	return UFR_OK;
}
//------------------------------------------------------------------------------
bool Enter3DesKey(uint8_t *key_3des)
{
	char str[100];
	size_t str_len;

	scanf("%[^\n]%*c", str);
	str_len = hex2bin(key_3des, str);
	if(str_len != 16)
	{
		printf("\nYou need to enter 16 hexadecimal numbers with or without spaces or with : as delimiter\n");
		scanf("%[^\n]%*c", str);
		str_len = hex2bin(key_3des, str);
		if(str_len != 16)
			return false;
	}

	return true;
}
//------------------------------------------------------------------------------
bool EnterPageData(uint8_t *page_data)
{
	char str[100];
	size_t str_len;
	char key;

	printf(" (1) - ASCI\n"
		   " (2) - HEX\n");

	while (!_kbhit())
		;
	key = _getch();

	if(key == '1')
	{
		printf("Enter 4 ASCI characters\n");
		scanf("%[^\n]%*c", str);
		str_len = strlen(str);
		if(str_len != 4)
		{
			printf("\nYou need to enter 4 characters\n");
			scanf("%[^\n]%*c", str);
			str_len = strlen(str);
			if(str_len != 4)
				return false;
		}
		memcpy(page_data, str, 4);
		return true;
	}
	else if(key == '2')
	{
		printf("Enter 4 hexadecimal bytes\n");
		scanf("%[^\n]%*c", str);
		str_len = hex2bin(page_data, str);
		if(str_len != 4)
		{
			printf("\nYou need to enter 4 hexadecimal numbers with or without spaces or with : as delimiter\n");
			scanf("%[^\n]%*c", str);
			str_len = hex2bin(page_data, str);
			if(str_len != 4)
				return false;
		}
		return true;
	}
	else
		return false;
}
//------------------------------------------------------------------------------
bool EnterLinearData(uint8_t *linear_data, uint16_t *linear_len)
{
	char str[3440];
	size_t str_len;
	char key;

	*linear_len = 0;

	printf(" (1) - ASCI\n"
		   " (2) - HEX\n");

	while (!_kbhit())
		;
	key = _getch();

	if(key == '1')
	{
		printf("Enter ASCI text\n");
		scanf("%[^\n]%*c", str);
		str_len = strlen(str);
		*linear_len = str_len;
		memcpy(linear_data, str, *linear_len);
		return true;
	}
	else if(key == '2')
	{
		printf("Enter hexadecimal bytes\n");
		scanf("%[^\n]%*c", str);
		str_len = hex2bin(linear_data, str);
		*linear_len = str_len;
		return true;
	}
	else
		return false;
}
//----------------------------------------------------------------------------------

void authentication_3des(void)
{
	UFR_STATUS status;
	uint8_t key_3des[16];

	printf(" -------------------------------------------------------------------\n");
	printf("                        Authentication with 3DES key                    \n");
	printf(" -------------------------------------------------------------------\n");

	printf("\nEnter 3DES key (16 hexadecimal numbers)\n");
	if(!Enter3DesKey(key_3des))
	{
		printf("\nError while key entry\n");
		return;
	}

	status = ULC_ExternalAuth_PK(key_3des);

	if(status)
	{
		printf("\nCard does not authenticated\n");
		printf("Error code = %02X\n", status);
	}
	else
		printf("\nCard is authenticated\n");
}
//------------------------------------------------------------------------------
void enable_card_halt(void)
{
	UFR_STATUS status;

	printf(" -------------------------------------------------------------------\n");
	printf("                      Card halt enabled                             \n");
	printf(" -------------------------------------------------------------------\n");

	status = card_halt_enable();

	if(status)
	{
		printf("\nCard halt operation does not enabled\n");
		printf("Error code = %02X\n", status);
	}
	else
		printf("\nCard halt operation is enabled\n");
}
//------------------------------------------------------------------------------
void page_read(void)
{
	UFR_STATUS status;
	int page_nr_int, key_index_int;
	uint8_t page_nr, key_index;
	uint8_t data[16];
	char key;

	printf(" -------------------------------------------------------------------\n");
	printf("                        Page data read                              \n");
	printf(" -------------------------------------------------------------------\n");

	printf("\nEnter authentication mode\n"
			" (1) - No authentication\n"
			" (2) - SAM AES key authentication\n"
			);

	while (!_kbhit())
			;
	key = _getch();

	if(key == '1')
	{
		printf("\nEnter page number (0 - 43)\n");
		scanf("%d%*c", &page_nr_int);
		page_nr = page_nr_int;

		status = BlockRead(data, page_nr, T2T_WITHOUT_PWD_AUTH, 0);

		if(status)
		{
			printf("\nPage read failed\n");
			printf("Error code = %02X\n", status);
		}
		else
		{
			printf("\nPage read successful\n");
			printf("Data = ");
			print_hex_ln(data, 4, " ");
			printf("\n");
		}
	}
	else if(key == '2')
	{
		if(!sam_used)
		{
			if(open_sam())
				sam_used = true;
			else
			{
				printf("Error during SAM opened \n");
				return;
			}
		}

		printf("\nEnter page number (0 - 43)\n");
		scanf("%d%*c", &page_nr_int);
		page_nr = page_nr_int;

		printf("\nEnter SAM AES key number (0 - 127)\n");
		scanf("%d%*c", &key_index_int);
		key_index = key_index_int;

		status = BlockReadSamKey(data, page_nr, T2T_WITH_PWD_AUTH, key_index);

		if(status)
		{
			printf("\nPage read failed\n");
			printf("Error code = %02X\n", status);
		}
		else
		{
			printf("\nPage read successful\n");
			printf("Data = ");
			print_hex_ln(data, 4, " ");
			printf("\n");
		}
	}
	else
		printf("Wrong choice\n");
}
//-----------------------------------------------------------------------------

void page_write(void)
{
	UFR_STATUS status;
	int page_nr_int, key_index_int;
	uint8_t page_nr, key_index;
	uint8_t data[16];
	char key;

	printf(" -------------------------------------------------------------------\n");
	printf("                        Page data write                             \n");
	printf(" -------------------------------------------------------------------\n");

	printf("\nEnter authentication mode\n"
			" (1) - No authentication\n"
			" (2) - SAM AES key authentication\n"
			);

	while (!_kbhit())
			;
	key = _getch();

	if(key == '1')
	{
		printf("\nEnter page number (2 - 47)\n");
		scanf("%d%*c", &page_nr_int);
		page_nr = page_nr_int;

		printf("\nEnter page data 4 bytes or characters\n");
		if(!EnterPageData(data))
		{
			printf("\nError while data entry\n");
			return;
		}

		status = BlockWrite(data, page_nr, T2T_WITHOUT_PWD_AUTH, 0);

		if(status)
		{
			printf("\nPage write failed\n");
			printf("Error code = %02X\n", status);
		}
		else
			printf("\nPage write successful\n");
	}
	else
	{
		if(!sam_used)
		{
			if(open_sam())
				sam_used = true;
			else
			{
				printf("Error during SAM opened \n");
				return;
			}
		}

		printf("\nEnter page number (2 - 47)\n");
		scanf("%d%*c", &page_nr_int);
		page_nr = page_nr_int;

		printf("\nEnter page data 4 bytes or characters\n");
		if(!EnterPageData(data))
		{
			printf("\nError while data entry\n");
			return;
		}

		printf("\nEnter SAM AES key number (0 - 127)\n");
		scanf("%d%*c", &key_index_int);
		key_index = key_index_int;

		status = BlockWriteSamKey(data, page_nr, T2T_WITH_PWD_AUTH, key_index);

		if(status)
		{
			printf("\nPage write failed\n");
			printf("Error code = %02X\n", status);
		}
		else
			printf("\nPage write successful\n");
	}
}
//---------------------------------------------------------------------------
void linear_read(void)
{
	UFR_STATUS status;
	uint16_t lin_addr, lin_len, ret_bytes;
	uint8_t key_index;
	int lin_addr_int, lin_len_int, key_index_int;
	uint8_t data[200];
	char key;

	printf(" -------------------------------------------------------------------\n");
	printf("                        Linear read                                 \n");
	printf(" -------------------------------------------------------------------\n");

	printf("\nEnter authentication mode\n"
				" (1) - No authentication\n"
				" (2) - SAM AES key authentication\n"
				);

	while (!_kbhit())
			;
	key = _getch();

	if(key == '1')
	{
		printf("\nEnter linear address (0 - 143)\n");
		scanf("%d%*c", &lin_addr_int);
		lin_addr = lin_addr_int;

		printf("\nEnter number of bytes for read\n");
		scanf("%d%*c", &lin_len_int);
		lin_len = lin_len_int;

		status = LinearRead(data, lin_addr, lin_len, &ret_bytes, T2T_WITHOUT_PWD_AUTH, 0);

		if(status)
		{
			printf("\nLinear read failed\n");
			printf("Error code = %02X\n", status);
		}
		else
		{
			printf("\nLinear read successful\n");
			printf("Data = ");
			print_hex_ln(data, ret_bytes, " ");
			printf("ASCI = %s\n", data);
		}
	}
	else if(key == '2')
	{
		if(!sam_used)
		{
			if(open_sam())
				sam_used = true;
			else
			{
				printf("Error during SAM opened \n");
				return;
			}
		}

		printf("\nEnter linear address (0 - 143)\n");
		scanf("%d%*c", &lin_addr_int);
		lin_addr = lin_addr_int;

		printf("\nEnter number of bytes for read\n");
		scanf("%d%*c", &lin_len_int);
		lin_len = lin_len_int;

		printf("\nEnter SAM AES key number (0 - 127)\n");
		scanf("%d%*c", &key_index_int);
		key_index = key_index_int;

		status = LinearReadSamKey(data, lin_addr, lin_len, &ret_bytes, T2T_WITH_PWD_AUTH, key_index);

		if(status)
		{
			printf("\nLinear read failed\n");
			printf("Error code = %02X\n", status);
		}
		else
		{
			printf("\nLinear read successful\n");
			printf("Data = ");
			print_hex_ln(data, ret_bytes, " ");
			printf("ASCI = %s\n", data);
		}
	}
	else
		printf("Wrong choice\n");
}
//----------------------------------------------------------------------------------
void linear_write(void)
{
	UFR_STATUS status;
	uint16_t lin_addr, lin_len, ret_bytes;
	uint8_t key_index;
	int lin_addr_int, key_index_int;
	uint8_t data[200];
	char key;

	printf(" -------------------------------------------------------------------\n");
	printf("                        Linear write                                 \n");
	printf(" -------------------------------------------------------------------\n");

	printf("\nEnter authentication mode\n"
				" (1) - No authentication\n"
				" (2) - SAM AES key authentication\n"
				);

	while (!_kbhit())
			;
	key = _getch();

	if(key == '1')
	{
		printf("\nEnter linear address (0 - 143)\n");
		scanf("%d%*c", &lin_addr_int);
		lin_addr = lin_addr_int;

		printf("\nEnter linear data\n");
		if(!EnterLinearData(data, &lin_len))
		{
			printf("\nError while data entry\n");
			return;
		}

		status = LinearWrite(data, lin_addr, lin_len, &ret_bytes, T2T_WITHOUT_PWD_AUTH, 0);

		if(status)
		{
			printf("\nLinear write failed\n");
			printf("Error code = %02X\n", status);
		}
		else
			printf("\nLinear write successful\n");
	}
	else if(key == '2')
	{
		if(!sam_used)
		{
			if(open_sam())
				sam_used = true;
			else
			{
				printf("Error during SAM opened \n");
				return;
			}
		}

		printf("\nEnter linear address (0 - 143)\n");
		scanf("%d%*c", &lin_addr_int);
		lin_addr = lin_addr_int;

		printf("\nEnter linear data\n");
		if(!EnterLinearData(data, &lin_len))
		{
			printf("\nError while data entry\n");
			return;
		}

		printf("\nEnter SAM AES key number (0 - 127)\n");
		scanf("%d%*c", &key_index_int);
		key_index = key_index_int;

		status = LinearWriteSamKey(data, lin_addr, lin_len, &ret_bytes, T2T_WITH_PWD_AUTH, key_index);

		if(status)
		{
			printf("\nLinear write failed\n");
			printf("Error code = %02X\n", status);
		}
		else
			printf("\nLinear write successful\n");
	}
	else
		printf("Wrong choice\n");
}
//----------------------------------------------------------------------------------
void key_write(void)
{
	UFR_STATUS status;
	char key;
	uint8_t new_key[16], old_key[16];

	printf(" -------------------------------------------------------------------\n");
	printf("                 Enter 3DES key into card                           \n");
	printf(" -------------------------------------------------------------------\n");

	printf("\nSelect method for key entering into card\n");
	printf(" (1) - Current key does not exist (authentication for access to the 3DES key pages is not required)\n"
		   " (2) - Current key is factory (authentication for access to the 3DES key pages with factory key)\n"
		   " (3) - Current key exist (authentication for access to the 3DESS key pages with current key\n");

	while (!_kbhit())
		;
	key = _getch();

	switch(key)
	{
	case '1':
		printf("\nEnter 3DES key (16 hexadecimal numbers)\n");
		if(!Enter3DesKey(new_key))
		{
			printf("\nError while key entry\n");
			return;
		}

		status = ULC_write_3des_key_no_auth(new_key);

		if(status)
		{
			printf("\nKey write failed\n");
			printf("Error code = %02X\n", status);
		}
		else
			printf("\nKey write successful\n");
		break;
	case '2':
		printf("\nEnter 3DES key (16 hexadecimal numbers)\n");
		if(!Enter3DesKey(new_key))
		{
			printf("\nError while key entry\n");
			return;
		}

		status = ULC_write_3des_key_factory_key(new_key);

		if(status)
		{
			printf("\nKey write failed\n");
			printf("Error code = %02X\n", status);
		}
		else
			printf("\nKey write successful\n");
		break;
	case '3':
		printf("\nEnter current 3DES key (16 hexadecimal numbers)\n");
		if(!Enter3DesKey(old_key))
		{
			printf("\nError while key entry\n");
			return;
		}

		printf("\nEnter new 3DES key (16 hexadecimal numbers)\n");
		if(!Enter3DesKey(new_key))
		{
			printf("\nError while key entry\n");
			return;
		}

		status = ULC_write_3des_key(new_key, old_key);

		if(status)
		{
			printf("\nKey write failed\n");
			printf("Error code = %02X\n", status);
		}
		else
			printf("\nKey write successful\n");
		break;
	}
}

bool open_sam(void)
{
	UFR_STATUS status;
	uint8_t atr_data[50];
	uint8_t len = 50;

	status = open_ISO7816_interface(atr_data, &len);
	if(status)
	{
		printf("Error code = %02x\n", status);
		return false;
	}
	else
	{
		printf("SAM opened\n");
		return true;
	}
}

void SamStoreKey(void)
{
    UFR_STATUS status;
    int key_no_int, key_ver_int;
    unsigned char key_no;
    unsigned char key_a[16];
    unsigned char master_key[16], master_key_ver;
    unsigned char apdu_sw[2];
    unsigned char auth_key_no;

    printf(" -------------------------------------------------------------------\n");
   	printf("                         WRITING KEY INTO SAM                       \n");
   	printf(" -------------------------------------------------------------------\n");

   	if(!sam_used)
	{
		if(open_sam())
			sam_used = true;
		else
		{
			printf("Error during SAM opened \n");
			return;
		}
	}

	printf("Enter 2K3DES key (16 bytes hexadecimal)\n");
	if(!Enter3DesKey(key_a))
	{
		printf("\nError while key entry\n");
		return;
	}

	printf("Enter SAM ordinal key number (0 - 127):\n");
	scanf("%d%*c", &key_no_int);
	key_no = key_no_int & 0xFF;

	printf("Enter SAM key for host authentication ordinal number: \n");
	scanf("%d%*c", &key_no_int);
	auth_key_no = key_no_int & 0x7F;

	printf("Enter version of host authentication key (0 - 255): \n");
	scanf("%d%*c", &key_ver_int);
	master_key_ver = key_ver_int & 0xFF;

	printf("Enter host authentication key: \n");
	if(!Enter3DesKey(master_key))
	{
		printf("\nError while key entry\n");
		return;
	}

	status = SAM_authenticate_host_AV2_plain(master_key, auth_key_no, master_key_ver, apdu_sw);

	if(status)
	{
		printf("\nHost authentication error\n");
		printf("Error code = %02X\n", status);
		return;
	}
	else
		printf("\nHost authentication is OK\n");


	status = SAM_change_key_entry_2K3DES_ULC_AV2_plain_one_key(key_no, key_a, auth_key_no, master_key_ver, 0xFF, apdu_sw);

	if(status)
	{
		printf("\nChange key entry error\n");
		printf("Error code = %02X\n", status);
		return;
	}
	else
		printf("\n2K3DES key stored successfully\n");

}
