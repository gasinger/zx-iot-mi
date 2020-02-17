/* ZX Server

Controls communication to the ZX computer by listening to signal_from and 
sending data via signal_to modules.

Works asynchroously, thus communication is done via queues

*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"

#include "zx_server.h"
#include "zx_file_img.h"
#include "signal_to_zx.h"

static const char *TAG = "zx_server";


static QueueHandle_t msg_queue=NULL;



//static const char menufile[]={0x00,0x00,0x00,0xf7,0x40,0xf8,0x40,0x17,0x41,0x00,0x00,0x18,0x41,0x17,0x41,0x00,0x00,0x18,0x41,0x18,0x41,0x00,0x5d,0x40,0x00,0x02,0x01,0x00,0xff,0xff,0xff,0x37,0xb7,0x40,0x00,0x00,0x00,0x00,0x00,0x8d,0x0c,0x00,0x00,0xff,0xff,0x00,0x00,0xbc,0x21,0x18,0x40,0x0e,0xfe,0x06,0x08,0xdb,0xfe,0x17,0x30,0xfb,0x10,0xfe,0x06,0x08,0xed,0x50,0xcb,0x12,0x17,0x16,0x04,0x15,0x20,0xfd,0x10,0xf4,0x77,0xcd,0xfc,0x01,0x18,0xe1,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0xea,0xcd,0xe7,0x02,0xcd,0x46,0x0f,0x06,0xa0,0xc5,0x06,0x00,0x10,0xfe,0xc1,0x10,0xf8,0x1e,0x49,0xcd,0x1f,0x03,0x1e,0x01,0xcd,0x1f,0x03,0x1e,0x00,0xcd,0x1f,0x03,0xe1,0xe1,0xe1,0xe1,0xe1,0xe1,0xe1,0x21,0x76,0x06,0xe3,0x21,0x15,0x40,0x34,0x21,0x09,0x40,0xc3,0x3c,0x40,0x76,0x00,0x0a,0x06,0x00,0xf1,0x26,0x0d,0x14,0x41,0x76,0x00,0x14,0x11,0x00,0xfa,0x26,0x0d,0x14,0x0b,0x0b,0xde,0xec,0x1d,0x1c,0x7e,0x84,0x20,0x00,0x00,0x00,0x76,0x00,0x1e,0x0e,0x00,0xf4,0xc5,0x0b,0x1d,0x22,0x21,0x1f,0x22,0x0b,0x1a,0xc4,0x26,0x0d,0x76,0x00,0x32,0x0b,0x00,0xf5,0xd4,0xc5,0x0b,0x1d,0x22,0x21,0x1d,0x20,0x0b,0x76,0x76,0x3f,0x3d,0x00,0x00,0x2e,0x34,0x39,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x80};

// demo p file
//static const uint8_t pfile[]={        0x0,0xa,0x0,0x88,0x40,0x89,0x40,0xa1,0x40,0x0,0x0,0xa2,0x40,0xaa,0x40,0x0,0x0,0xab,0x40,0xab,0x40,0x0,0x5d,0x40,0x0,0x2,0x0,0x0,0xbf,0xfd,0xff,0x37,0x88,0x40,0x0,0x0,0x0,0x0,0x0,0x8d,0xc,0x0,0x0,0xa2,0xf8,0x0,0x0,0xbc,0x21,0x18,0x40,  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0, 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0, 0x0,0x0,0x0,0x0,0x0,0x0,0x0, 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x76,0x0,0x0,0x0,0x0,0x0,0x0,0x84,0x0,0x0,0x0,0x84,0xa0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xa,0x7,0x0,0xea,0x2d,0x2a,0x31,0x31,0x34,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x80};

// c
//static const char ldrfilec[]= { 0xA6, 0x0,0x0,0x0,0xc0,0x40,0xc1,0x40,0xde,0x40,0x0,0x0,0xdf,0x40,0xde,0x40,0x0,0x0,0xdf,0x40,0xdf,0x40,0x0,0x5d,0x40,0x0,0x2,0x1,0x0,0xff,0xff,0xff,0x37,0xb1,0x40,0x0,0x0,0x0,0x0,0x0,0x8d,0xc,0x0,0x0,0xff,0xff,0x0,0x0,0xbc,0x21,0x18,0x40,0x2b,0x3e,0x3d,0xd3,0xef,0xdb,0x6f,0xe6,0x1,0x28,0xa,0x3e,0x38,0xd3,0xef,0xdb,0x6f,0x23,0x77,0x18,0xec,0xaf,0xd3,0xef,0xcd,0xfc,0x1,0x18,0xe3,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x30,0x0,0xea,0xcd,0xe7,0x2,0x21,0x76,0x6,0xe3,0x21,0x15,0x40,0x34,0x21,0x9,0x40,0xe,0xfe,0x16,0x9,0xdb,0xfe,0x17,0x38,0xfb,0x42,0x10,0xfe,0x6,0x8,0xed,0x50,0xcb,0x12,0xcb,0x1f,0xed,0x5f,0x77,0x16,0x3,0x15,0x20,0xfd,0x10,0xf0,0x18,0xe0,0x76,0x0,0xa,0xb,0x0,0xf5,0xd4,0xc5,0xb,0x1d,0x22,0x21,0x1d,0x20,0xb,0x76,0x76,0x3f,0x3d,0x0,0x35,0x2e,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x80};

// nc
//static const char ldrfile_printfirstbyte[]=  { 0xA6, 0x0,0x0,0x0,0xcd,0x40,0xce,0x40,0xeb,0x40,0x0,0x0,0xec,0x40,0xeb,0x40,0x0,0x0,0xec,0x40,0xec,0x40,0x0,0x5d,0x40,0x0,0x2,0x1,0x0,0xff,0xff,0xff,0x37,0xbe,0x40,0x0,0x0,0x0,0x0,0x0,0x8d,0xc,0x0,0x0,0xff,0xff,0x0,0x0,0xbc,0x21,0x18,0x40,0xe,0xfe,0x6,0x8,0xdb,0xfe,0x17,0x30,0xfb,0x10,0xfe,0x6,0x8,0xed,0x50,0xcb,0x12,0x1f,0x16,0x4,0x15,0x20,0xfd,0x10,0xf4,0x77,0xcd,0xfc,0x1,0x18,0xe1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x3d,0x0,0xea,0xcd,0xe7,0x2,0x21,0x76,0x6,0x21,0x9a,0x40,0x11,0x3c,0x40,0x1,0x20,0x0,0xed,0xb0,0x21,0x15,0x40,0x34,0x21,0x9,0x40,0xe,0xfe,0x6,0x8,0xdb,0xfe,0x17,0x30,0xfb,0x10,0xfe,0x6,0x8,0xed,0x50,0xcb,0x12,0x1f,0x16,0x4,0x15,0x20,0xfd,0x10,0xf4,0x4f,0x6,0x0,0xc9,0x77,0xcd,0xfc,0x1,0x18,0xdd,0x76,0x0,0xa,0xb,0x0,0xf5,0xd4,0xc5,0xb,0x1d,0x22,0x21,0x1d,0x20,0xb,0x76,0x76,0x3f,0x3d,0x0,0x35,0x2e,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x80};


//static const char ldrfile[]=  { 0xA6, 0x00,0x00,0x00,0xc0,0x40,0xc1,0x40,0xe0,0x40,0x00,0x00,0xe1,0x40,0xe0,0x40,0x00,0x00,0xe1,0x40,0xe1,0x40,0x00,0x5d,0x40,0x00,0x02,0x01,0x00,0xff,0xff,0xff,0x37,0xb1,0x40,0x00,0x00,0x00,0x00,0x00,0x8d,0x0c,0x00,0x00,0xff,0xff,0x00,0x00,0xbc,0x21,0x18,0x40,0x0e,0xfe,0x06,0x08,0xdb,0xfe,0x17,0x30,0xfb,0x10,0xfe,0x06,0x08,0xed,0x50,0xcb,0x12,0x17,0x16,0x04,0x15,0x20,0xfd,0x10,0xf4,0x77,0xcd,0xfc,0x01,0x18,0xe1,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0xea,0xcd,0xe7,0x02,0xcd,0x46,0x0f,0x06,0xc8,0xc5,0x06,0x00,0x10,0xfe,0xc1,0x10,0xf8,0x1e,0x46,0xcd,0x1f,0x03,0x21,0x05,0x40,0xcd,0x1e,0x03,0x21,0x76,0x06,0x1e,0x00,0xcd,0x1f,0x03,0xe3,0x21,0x15,0x40,0x34,0x21,0x09,0x40,0xc3,0x3c,0x40,0x76,0x00,0x0a,0x0b,0x00,0xf5,0xd4,0xc5,0x0b,0x1d,0x22,0x21,0x1d,0x20,0x0b,0x76,0x76,0x3f,0x3d,0x00,0x2e,0x34,0x00,0x39,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x80};

//const char ldrfile[]=  { 0xA6, 0x0,0x0,0x0,0xa3,0x40,0xa4,0x40,0xc3,0x40,0x0,0x0,0xc4,0x40,0xc3,0x40,0x0,0x0,0xc4,0x40,0xc4,0x40,0x0,0x5d,0x40,0x0,0x2,0x1,0x0,0xff,0xff,0xff,0x37,0x94,0x40,0x0,0x0,0x0,0x0,0x0,0x8d,0xc,0x0,0x0,0xff,0xff,0x0,0x0,0xbc,0x21,0x18,0x40,0xe,0xfe,0x6,0x8,0xdb,0xfe,0x17,0x30,0xfb,0x10,0xfe,0x6,0x8,0xed,0x50,0xcb,0x12,0x17,0x16,0x4,0x15,0x20,0xfd,0x10,0xf4,0x77,0xcd,0xfc,0x1,0x18,0xe1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x13,0x0,0xea,0xcd,0xe7,0x2,0x21,0x76,0x6,0xe3,0x21,0x15,0x40,0x34,0x21,0x9,0x40,0xc3,0x3c,0x40,0x76,0x0,0xa,0xb,0x0,0xf5,0xd4,0xc5,0xb,0x1d,0x22,0x21,0x1d,0x20,0xb,0x76,0x76,0x3f,0x3d,0x0,0x2e,0x34,0x0,0x39,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x80};




static const char* CODETABLE="#_~\"@$:?()><=+-*/;,.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";	/* ZX81 code table, starting at 28 */

/* Helper for file name conversion, will set the provided error flag if the input is invalid */
static uint8_t convert_ascii_to_zx_code(int ascii_char)
{
	uint8_t zx_code=0;
	int upper_ascii_c=toupper(ascii_char);

    if(ascii_char==' ') return 0;
	while(CODETABLE[zx_code])
	{
		if(CODETABLE[zx_code]==upper_ascii_c) return 8+zx_code;		/* Exit with result on match */
		zx_code++;
	}
	/* return a default */
	return 4;
}

static void zx_string_to_ascii(const uint8_t* zxstr, size_t len,  char* buf_for_ascii)
{
    int i;
    uint8_t z;
    char c;
    for(i=0;i<len;i++) {
        c='?';
        z=zxstr[i] & 0x7f; // ignore invert
        if(z>=8 && z<64) c=CODETABLE[z-8];
        *buf_for_ascii++=c;
    }
    *buf_for_ascii=0;   // string end
    return;
}


static char txt_buf[33];



#define FILFB_SIZE 32
#define FILENAME_SIZE 24

static uint8_t file_first_bytes[FILFB_SIZE]; // storage for analyzing the file name or commands etc
static uint8_t file_name_len=0; 

typedef void (*resp_func) (const char*, int);

typedef struct {
    uint8_t zxkey;  
    resp_func func;
    char func_arg_s[FILENAME_SIZE];
    int func_arg_i;  
} zxsrv_menu_response_entry;

static zxsrv_menu_response_entry menu_response[30];
static uint8_t menu_resp_size=0;
//static char curr_dir[];

static void clear_mrespond_entries(){
    menu_resp_size=0;
}
static void create_mrespond_entry(uint8_t zxkey, resp_func func, const char* func_arg_s,  int func_arg_i){
    menu_response[menu_resp_size].zxkey=zxkey;
    menu_response[menu_resp_size].func=func;
    strlcpy(menu_response[menu_resp_size].func_arg_s, func_arg_s, FILENAME_SIZE);
    menu_response[menu_resp_size].func_arg_i=func_arg_i;
    menu_resp_size++;
}

static void menu_respond_from_key(uint8_t zxkey){
    uint8_t e=0;
    if (menu_resp_size==0) return;
    for (e=0; e<menu_resp_size; e++){
        if( menu_response[e].zxkey == zxkey || e+1==menu_resp_size ){
            (*menu_response[e].func)(menu_response[e].func_arg_s, menu_response[e].func_arg_i);
        }
    }
}

static void zxsrv_respond_filemenu(const char *dirpath, int); // fwd declare

static void send_zxf_loader_uncompressed(){
    uint16_t imgsize;
    ESP_LOGI(TAG,"Send (uncompressed) loader \n");
    zxfimg_create(ZXFI_LOADER);
    imgsize=zxfimg_get_size();
    stzx_send_cmd(STZX_FILE_START,FILE_TAG_NORMAL);
    stzx_send_cmd(STZX_FILE_DATA,0xA6);       /* one byte file name */             
    for(size_t i=0;i<imgsize;i++){
        stzx_send_cmd(STZX_FILE_DATA,zxfimg_get_img(i));                    
    }
    stzx_send_cmd(STZX_FILE_END,0);
    zxfimg_delete();
    // next - main menu
    clear_mrespond_entries();
    create_mrespond_entry(0, zxsrv_respond_filemenu, "/spiffs/", 0 );

}


static void send_zxf_image_compr(){
    uint16_t imgsize=zxfimg_get_size();
    stzx_send_cmd(STZX_FILE_START,FILE_TAG_COMPRESSED);
    for(uint16_t i=0;i<imgsize;i++){
        stzx_send_cmd(STZX_FILE_DATA, zxfimg_get_img(i) );                    
    }
   stzx_send_cmd(STZX_FILE_END,0);
}

static void zxsrv_respond_fileload(const char *filepath, int dummy){
    int rdbyte;
    uint16_t fpos=0;
    FILE *fd = NULL;

    ESP_LOGI(TAG, "FILELOAD : %s", filepath);

    clear_mrespond_entries();

    fd = fopen(filepath, "rb");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        return;
    }
    while(1) {
        rdbyte=fgetc(fd);
        if (rdbyte==EOF) break;
        zxfimg_set_img(fpos++, rdbyte);
    } 
    /* Close file after read complete */
    fclose(fd);
    send_zxf_image_compr();
    zxfimg_delete();

    // next - main menu
    clear_mrespond_entries();
    create_mrespond_entry(0, zxsrv_respond_filemenu, "/spiffs/", 0 );

}

static void zxsrv_respond_inpstr(const char *question, int offset){

   // const char *dirpath="/spiffs/";
    zxfimg_create(ZXFI_STR_INP);
    sprintf(txt_buf,"[ INPUT STRING ] ");
    zxfimg_print_video(1,txt_buf);

    sprintf(txt_buf,"[ INPUT STRING ] ");
    zxfimg_print_video(3,question);

    clear_mrespond_entries();
    /* append default entry */
    create_mrespond_entry(0, zxsrv_respond_filemenu, "/spiffs/", 0 );
    send_zxf_image_compr();
    zxfimg_delete();
}


static void zxsrv_respond_filemenu(const char *dirpath, int offset){

   // const char *dirpath="/spiffs/";
    char entrypath[ESP_VFS_PATH_MAX+17];
    char entrysize[16];
    const char *entrytype;
    uint8_t entry_num=0;
    struct dirent *entry;
    struct stat entry_stat;
    DIR *dir = opendir(dirpath);
    const size_t dirpath_len = strlen(dirpath);


    ESP_LOGI(TAG, "FILEMENU : %s", dirpath);
    /* Retrieve the base path of file storage to construct the full path */
    strlcpy(entrypath, dirpath, sizeof(entrypath));

    if (!dir) {
        ESP_LOGE(TAG, "Failed to stat dir : %s", dirpath);
    //    /* Respond with 404 Not Found */
    //    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Directory does not exist");
        return;
    }
    zxfimg_create(ZXFI_MENU_KEY);
    sprintf(txt_buf,"[ FILE MENU ]: (%s) ",dirpath);
    zxfimg_print_video(1,txt_buf);

    clear_mrespond_entries();
    /* Iterate over all files / folders and fetch their names and sizes */
    while ((entry = readdir(dir)) != NULL && entry_num<16) {
        entrytype = (entry->d_type == DT_DIR ? "(DIR)" : "");

        strlcpy(entrypath + dirpath_len, entry->d_name, sizeof(entrypath) - dirpath_len);
        if (stat(entrypath, &entry_stat) == -1) {
            ESP_LOGE(TAG, "Failed to stat %s : %s", entrytype, entry->d_name);
            continue;
        }
        sprintf(entrysize, "%ld", entry_stat.st_size);
        ESP_LOGI(TAG, "Found %s : %s (%s bytes)", entrytype, entry->d_name , entrysize);
        create_mrespond_entry(entry_num+0x1c, zxsrv_respond_fileload,  entrypath, 0 );

        sprintf(txt_buf," [%X] %s %s",entry_num,entry->d_name,entrytype);
        zxfimg_print_video(entry_num+3,txt_buf);
        entry_num++;
    }
    /* append default entry */
    create_mrespond_entry(55, zxsrv_respond_inpstr, "INP-QU", 0 ); // "R"
    create_mrespond_entry(0, zxsrv_respond_filemenu, "/spiffs/", 0 );
    closedir(dir);
    send_zxf_image_compr();
    zxfimg_delete();
}


static void zxsrv_filename_received(){
    char namebuf[FILFB_SIZE];
    zx_string_to_ascii(file_first_bytes,file_name_len,namebuf);
    ESP_LOGI(TAG,"SAVE file, name [%s]\n",namebuf);
}

static void save_received_zxfimg(){
    FILE *fd = NULL;
    uint16_t i;
    const char *dirpath="/spiffs/";
    char entrypath[ESP_VFS_PATH_MAX+17];
    char namebuf[FILFB_SIZE];
    zx_string_to_ascii(file_first_bytes,file_name_len,namebuf);
    if(NULL==strchr(namebuf,'.') ){ /* add extension if none provided */
        strlcpy(namebuf + strlen(namebuf), ".p", FILFB_SIZE - strlen(namebuf));
    }
    ESP_LOGI(TAG,"SAVE - file [%s]\n",namebuf);
    strlcpy(entrypath, dirpath, sizeof(entrypath));
    strlcpy(entrypath + strlen(dirpath), namebuf, sizeof(entrypath) - strlen(dirpath));
    fd = fopen(entrypath, "wb");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to write to file : %s", entrypath);
        return;
    }
    for(i=0; i<zxfimg_get_size(); i++) {
        fputc( zxfimg_get_img(i), fd );
    } 
    /* Close file after write complete */
    fclose(fd);
    ESP_LOGI(TAG,"SAVE - done\n");
}

static void zxsrv_task(void *arg)
{
    zxserv_event_t evt;
    ESP_LOGI(TAG,"sfzx_task START \n");
    while(true){
		if(pdTRUE ==  xQueueReceive( msg_queue, &evt, 10 / portTICK_RATE_MS ) ) {
			//ESP_LOGI(TAG,"Retrieved evt %d",evt.evt_type);
            if(evt.evt_type==ZXSG_HIGH){
                // Load
                send_zxf_loader_uncompressed();
            }else if(evt.evt_type==ZXSG_FILE_DATA){
                if(evt.addr<FILFB_SIZE){
                    file_first_bytes[evt.addr]=(uint8_t) evt.data;
                    // extract file name
                    if(evt.addr==0) file_name_len=0;
                    if(!file_name_len){
                        if( evt.data&0x80 ){
                            file_name_len=evt.addr+1;
                            zxsrv_filename_received();
                        }
                    }
                    if(file_first_bytes[0]==ZX_SAVE_TAG_MENU_RESPONSE+1 && evt.data==0x80){
                            // send compressed second stage
                            ESP_LOGI(TAG,"STRING INPUT addr %d \n",evt.addr); 
                            for(int i=0;i<=evt.addr;i++){
                                ESP_LOGI(TAG,"  STR field %d %02X \n",evt.addr,file_first_bytes[i] ); 
                            }
                    }

                    //
                    if(evt.addr==1){
                        if(file_first_bytes[0]==ZX_SAVE_TAG_LOADER_RESPONSE){
                            // send compressed second stage
                            ESP_LOGI(TAG,"Response from %dk ZX, send 2nd (compressed) stage \n",(evt.data-0x40)/4 );                        
                            menu_respond_from_key(0);
                        } else if(file_first_bytes[0]==ZX_SAVE_TAG_MENU_RESPONSE){
                            // send compressed second stage
                            ESP_LOGI(TAG,"MENU RESPONSE KEYPRESS code %02X \n",evt.data); 
                            menu_respond_from_key(evt.data);
                        }
                    }
                }
                if(file_name_len){
                    zxfimg_set_img(evt.addr-file_name_len,evt.data);
                    if(evt.addr>file_name_len+30 && zxfimg_get_size()==1+evt.addr-file_name_len ){
                        save_received_zxfimg();
                        file_name_len=0;
                    }
                }
			}
			else ESP_LOGW(TAG,"Unexpected evt %d",evt.evt_type);
		}
    }
}



void zxsrv_init()
{
    msg_queue=xQueueCreate(50,sizeof( zxserv_event_t ) );
    xTaskCreate(zxsrv_task, "zxsrv_task", 1024 * 3, NULL, 8, NULL);
}


void zxsrv_send_msg_to_srv( zxserv_evt_type_t msg, uint16_t addr, uint16_t data)
{
    zxserv_event_t evt;
	evt.evt_type=msg;
	evt.addr=addr;
	evt.data=data;
	if( xQueueSendToBack( msg_queue,  &evt, 100 / portTICK_RATE_MS ) != pdPASS )
	{
		// Failed to post the message, even after 100 ms.
		ESP_LOGE(TAG, "Server write queue blocked");
	}
}
