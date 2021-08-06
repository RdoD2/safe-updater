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


int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext)

{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits */
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv))
        handleErrors();

    /* Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /* Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;


}




void file_decrypt(char *firmware_path, unsigned char* key, unsigned char* iv)
{
	FILE *fp;	
	unsigned char buffer[255];
	unsigned char decrypt_buffer[255];
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
		decrypt(buffer, sizeof(buffer), key, iv, decrypt_buffer);
		fseek(fp, 255*i, SEEK_SET);
		fwrite(decrypt_buffer, 255, 1, fp);
		memset(buffer, 0, sizeof(buffer));
		memset(decrypt_buffer, 0, sizeof(decrypt_buffer));
	}
	fseek(fp, 255*block_cnt, SEEK_SET);
	fread(buffer, file_size%255, 1, fp);
	decrypt(buffer, file_size%255, key, iv, decrypt_buffer);
	fseek(fp, 255*block_cnt, SEEK_SET);
	fwrite(decrypt_buffer, file_size%255, 1, fp);
	fclose(fp);
	memset(buffer, 0, sizeof(buffer));
	memset(decrypt_buffer, 0, sizeof(decrypt_buffer));
}






#define TAR_FILE_CNT 3
char temp_tar_name_list[TAR_FILE_CNT][255];

int file_extract(char* path)
{
    FILE* fp;
    FILE* ftemp;
    int fsize = 0, fnameSize = 0;
    char decrypt_buf[255];
    int i = 0, j = 0;
    int block_cnt = 0;
    char buf[255];
    char temp_tar_name[255];
    
    fp = fopen(path, "rb");
    printf("%s\n", path);
    for(i=0;i<3;i++)
    {	
        printf("시작중\n");
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
            memset(decrypt_buf, 0, strlen(decrypt_buf));
        }
        fread(buf, fsize % 255, 1, fp);
        fwrite(buf, fsize % 255, 1, ftemp);
        memset(buf, 0, strlen(buf));
        memset(decrypt_buf, 0, strlen(decrypt_buf));
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
    
    for(i=0; i<3; i++)
    {


        strcpy(tar_fn, temp_tar_name_list[i]);

        if (access(tar_fn, 0) == 0) {
            rmdir("./boot");
            rmdir("./documentation");
            rmdir("./opt");
        }

        sprintf(cmd, "pv %s |  tar xf - -C /var/update_test", tar_fn);

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
    int i=0;
    unsigned char* key;
    key = (unsigned char*) malloc(32);
    memcpy(key, "12341234912345678911234567832345", 32);
    unsigned char* iv;
    iv = (unsigned char*) malloc(16);
    memcpy(iv, "1234567891234567", 16);


    char hashbuffer[500];
    FILE* fp = fopen("hash.txt", "r"); 
    fread(hashbuffer, 1, 500, fp); 
    char myhash[500] = "2668d2a81c8b702cec79397a9ad2d02f7d16ede40d592d024d6c437c76c3156d";
    
    
    if(strcmp(hashbuffer,myhash) == 0)
    {
        printf("[완료] 사전에 확인된 파일의 해시와 동일합니다! \n");
        printf("[완료] 복호화가 시작됩니다 잠시만 기다려주세요! \n");
    }
    else
    {
        printf("[경고] 파일의 해시가 다릅니다!\n");
        printf("[경고] 펌웨어가 종료됩니다...\n");
        exit(0); 


    }
	
    
    sprintf(firmware_path, "%s", argv[1]);
    printf("[WAIT] 파일을 복호화합니다. 이작업은 약 10초 정도 걸릴 수 있습니다.\n");
    file_extract(firmware_path);
    
    sleep(5);
    

    printf("[WAIT] 잠시만 더 기다려주세요.\n");
  
    


    if (argc != 2) {
        printf("usage: %s [firmware path]\n", argv[0]);
        exit(0);
    }


    printf("[펌웨어를 추출하고 있습니다.] \n");
    if ((ret = file_extract(firmware_path)) < 0) {
        printf("File system extraction failed.(%d)", ret);
        exit(0);
    }
  
    for(i=0;i<3;i++)
    {
    if(strcmp(temp_tar_name_list[i], "documentation.tar")!=0)
		{
	file_decrypt(temp_tar_name_list[i], key, iv);
	
	}
    }
    tar_extract();


    printf("OK\n");
    printf("[Firmware Update] 펌웨어 업데이터가 성공적으로 되었습니다.\n");
    return 0;
}

