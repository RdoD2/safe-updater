#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <crypt.h>
#include <time.h>
#include <unistd.h>

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}


int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;
 
    // Create and initialise the context
    if (!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();
 
    // Initialise the encryption operation
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv))
        handleErrors();
 
    // Provide the message to be encrypted, and obtain the encrypted output
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;
 
    // Finalise the encryption
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;
 
    // Clean up
    EVP_CIPHER_CTX_free(ctx);
 
    return ciphertext_len;
}




void file_encrypt(char *firmware_path, unsigned char* key, unsigned char* iv)
{
	FILE *fp;	
	unsigned char buffer[255];
	unsigned char encrypt_buffer[255];

	int file_size=0;
	int block_cnt = 0;
	int i=0;
	fp = fopen(firmware_path, "rb+");
	fseek(fp, 0, SEEK_END);
	file_size=ftell(fp);
	fseek(fp,0,SEEK_SET);
	block_cnt = file_size/255;
	for(i=0;i<block_cnt;i++)
	{
		fseek(fp, 255*i, SEEK_SET);
		fread(buffer, 255, 1, fp);
		encrypt(buffer, sizeof(buffer), key, iv, encrypt_buffer);
		fseek(fp, 255*i, SEEK_SET);
		fwrite(encrypt_buffer, 255, 1, fp);
		memset(buffer, 0, sizeof(buffer));
		memset(encrypt_buffer, 0, sizeof(encrypt_buffer));
	}
	fseek(fp, 255*block_cnt, SEEK_SET);
	fread(buffer, file_size%255, 1, fp);
	encrypt(buffer, file_size%255, key, iv, encrypt_buffer);
	fseek(fp, 255*block_cnt, SEEK_SET);
	fwrite(encrypt_buffer, file_size%255, 1, fp);
	fclose(fp);
	memset(buffer, 0, sizeof(buffer));
	memset(encrypt_buffer, 0, sizeof(encrypt_buffer));
}






#define TAR_FILE_CNT 3
char temp_tar_name_list[TAR_FILE_CNT][255];

int file_extract(char* path)
{
    FILE* fp;
    FILE* ftemp;
    int fsize = 0, fnameSize = 0;
    int i = 0, j = 0;
    int block_cnt = 0;
    char buf[255];
    char temp_tar_name[255];
    
    fp = fopen(path, "rb");
    printf("%s\n", path);
    for (i = 0; i < TAR_FILE_CNT; i++)
    {
        printf("start\n");
        fread(&fnameSize, 4, 1, fp);
        memset(temp_tar_name, 0, 255);
        fread(temp_tar_name, fnameSize, 1, fp);
        printf("tar name : %s \n" , temp_tar_name);
        fread(&fsize, 4, 1, fp);
        block_cnt = fsize / 255;
        ftemp = fopen(temp_tar_name, "wb");
        for (j = 0; j < block_cnt; j++)
        {
            fread(buf, 255, 1, fp);
            fwrite(buf, 255, 1, ftemp);
            memset(buf, 0, strlen(buf));
        }
        fread(buf, fsize % 255, 1, fp);
        fwrite(buf, fsize % 255, 1, ftemp);
        memset(buf, 0, strlen(buf));
        fclose(ftemp);
        strcpy(temp_tar_name_list[i], temp_tar_name);
    }
    fclose(fp);
    return 0;
}

int tar_extract()
{
    FILE* stream;
    char line[1024];
    char tar_fn[255] = { 0x00, };
    char cmd[1024] = { 0x00, };
    int i = 0;
    
    for (i = 0; i < TAR_FILE_CNT; i++)
    {

        
        strcpy(tar_fn, temp_tar_name_list[i]);

        if (access(tar_fn, 0) == 0) {
            rmdir("./boot");
            rmdir("./documentation");
            rmdir("./opt");
        }

        sprintf(cmd, "pv %s | tar xf - ", tar_fn);

        stream = popen(cmd, "r");
        if (stream == NULL) {
            remove(tar_fn);
            return -1;
        }
        pclose(stream);
        remove(tar_fn);
    }

    return 1;
}





int main(int argc, char* argv[])
{
 
    char firmware_path[255];
    int ret = 0;
    unsigned char* key;
    key = (unsigned char*) malloc(32);
    memcpy(key, "12341234912345678911234567832345", 32);
    unsigned char* iv;
    iv = (unsigned char*) malloc(16);
    memcpy(iv, "1234567891234567", 16);
    

	printf("파일을 암호화하고 있습니다!\n");
    
    
 
    if (argc != 2) {
        printf("usage: %s [firmware path]\n", argv[0]);
        exit(0);
    }
    sprintf(firmware_path,"%s", argv[1]);
    file_encrypt(firmware_path, key, iv);


  



    printf("[Success] 파일암호화에 성공하였습니다.\n");
}

