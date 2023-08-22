#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/socket.h>  
#include <sys/types.h>  
#include <unistd.h>  
#include <errno.h>  
#include <netinet/in.h>  
#include <netdb.h>  
#include <arpa/inet.h>  
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#define byte unsigned char
#define MAGIC_NUMBER_LENGTH 6
#define type unsigned char
#define status unsigned char


struct myftpHeader{
    byte m_protocol[MAGIC_NUMBER_LENGTH]; /* protocol magic number (6 bytes) */
    type m_type;                          /* type (1 byte) */
    status m_status;                      /* status (1 byte) */
    uint32_t m_length;                    /* length (4 bytes) in Big endian*/
} __attribute__ ((packed));
struct myftpHeader OPEN_CONN_REQUEST;
struct myftpHeader AUTH_REQUEST;
struct myftpHeader LIST_REQUEST;
struct myftpHeader GET_REQUEST;
struct myftpHeader PUT_REQUEST;
struct myftpHeader QUIT_REQUEST;
struct myftpHeader FILE_DATA;

char payload[1050010];
char buffer[2100];
int file_size(char* filename)//获取文件名为filename的文件大小。

{

    struct stat statbuf;

    int ret;

    ret = stat(filename,&statbuf);//调用stat函数

    if(ret != 0) return -1;//获取失败。

    return statbuf.st_size;//返回文件大小。

}

void RECV(int sock,char*buffer,int len,int flags){
    size_t ret = 0;
    //printf("RECV\n");
    while (ret < len) {
        //printf("start to recv\n");
        size_t b = recv(sock, buffer + ret, len - ret, 0);
        //printf("b:%d\n",b);
        if (b == 0) printf("socket Closed"); // 当连接断开
        if (b < 0) printf("Error ?"); // 这里可能发生了一些意料之外的情况
        ret += b; // 成功将b个byte塞进了缓冲区
        //printf("%d\n",ret);
    }
}

void SEND(int sock,char*buffer,int len,int flags){
    size_t ret = 0;
    while (ret < len) {
        size_t b = send(sock, buffer + ret, len - ret, 0);
        if (b == 0) printf("socket Closed"); // 当连接断开
        if (b < 0) printf("Error ?"); // 这里可能发生了一些意料之外的情况
        ret += b; // 成功将b个byte塞进了缓冲区
    }
    //printf("send success\n");
}
int main(int argc, char ** argv) {
    char cmd[20];
    memset(cmd,0,sizeof(cmd));
    int sock=0;
    while(1){
        printf("Client>");
        scanf("%s",cmd);
        if(strncmp(cmd,"open",4)==0){
            //printf("open!\n");
            char ip[30];
            memset(ip,0,sizeof(ip));
            int port=0;
            scanf("%s",ip);
            scanf("%d",&port);
            //printf("open222!\n");
            //printf("ip:%s,port:%d\n",ip,port);
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if(sock<0){
                printf("sock error\n");
            }
            struct sockaddr_in addr;
            bzero(&addr, sizeof(addr)); 
            addr.sin_port = htons(port);
            addr.sin_family = AF_INET;
            inet_pton(AF_INET, ip, &addr.sin_addr); // 表示我们要连接到服务器的ip:port
            if(connect(sock, (struct sockaddr*)&addr, sizeof(addr))==0){
                printf("connected successfully\n");
            }
            else{
                printf("Error:%d\n",connect);
            }
            /*----------set open_conn_request---------------*/
            char*s="\xe3myftp";
            memcpy(OPEN_CONN_REQUEST.m_protocol,s,6);
            OPEN_CONN_REQUEST.m_type=0xA1;
            OPEN_CONN_REQUEST.m_length=htonl(12);

            /*-----------------send--------------------------*/
           
            memset(buffer,0,sizeof(buffer));
            memcpy(buffer,&OPEN_CONN_REQUEST,sizeof(OPEN_CONN_REQUEST));
            //send OPEN_CONN_REQUEST
            //printf("send\n");
            SEND(sock, buffer, 12, 0);

            int len=ntohl(*((unsigned int*)(buffer+8)));
            printf("open request length:%ld\n",len);
            /*------------receive-------------------------*/
            //printf("recv\n");
            
            RECV(sock,buffer,12,0);
            //printf("%s\n",buffer);
            
            if(buffer[6]==(char)0xA2){
                printf("Server connection accepted.\n");
                continue;
            }
            
        }
        /*------------------------AUTH---------------------------*/
        if(strncmp(cmd,"auth",4)==0){
            char user[10];
            int pass;
            scanf("%s",user);
            scanf("%d",&pass);

            /*------------------set auth----------------*/
            char*s="\xe3myftp";
            char*data="user 123123\0";
            memcpy(AUTH_REQUEST.m_protocol,s,6);
            AUTH_REQUEST.m_type=0xA3;
            AUTH_REQUEST.m_length=htonl(12+strlen(data)+1);
            printf("%d\n",12+strlen(data)+1);

            /*-----------------send--------------------------*/
       
            memset(buffer,0,sizeof(buffer));
            memcpy(buffer,&AUTH_REQUEST,sizeof(AUTH_REQUEST));
            memcpy(buffer+sizeof(AUTH_REQUEST),data,strlen(data)+1);
            SEND(sock, buffer, 12+strlen(data)+1, 0);

            /*------------------recv--------------------------*/
            RECV(sock,buffer,12,0);
            //printf("%s\n",buffer);
            
            if(buffer[7]==(char)1){
                printf("Auth success!\n");
                continue;
            }
            if(buffer[7]==(char)0){
                printf("Auth failure!Closing the connection...\n");
                close(sock);//close connection
                continue;
            }
        }
        if(strncmp(cmd,"ls",2)==0){
            char*s="\xe3myftp";
            
            memcpy(LIST_REQUEST.m_protocol,s,6);
            LIST_REQUEST.m_type=0xA5;
            LIST_REQUEST.m_length=htonl(12);

            
            memset(buffer,0,sizeof(buffer));
            memcpy(buffer,&LIST_REQUEST,sizeof(LIST_REQUEST));
            SEND(sock, buffer, 12, 0);

            RECV(sock,buffer,12,0);//先收个报头
            if(buffer[6]==(char)0xA6){
                printf("start to recv data\n");
                int len=ntohl(*((unsigned int*)(buffer+8)))-12;//payload的长度
                printf("payload length:%ld\n",len);
                RECV(sock,buffer,len,0);//再收payload
                printf("-------file list start-------\n");

                printf("%s\n",buffer);

                printf("-------file list end---------\n");
            }
            else{
                printf("ls what a pity\n");
            }
            continue;
        }


        /*----------download file------------*/
        if(strncmp(cmd,"get",3)==0){
            char filename[20];//要获取的文件名称
            scanf("%s",filename);

        /*-----------get_request-------------*/
            char*s="\xe3myftp";
            
            memcpy(GET_REQUEST.m_protocol,s,6);
            GET_REQUEST.m_type=0xA7;
            GET_REQUEST.m_length=htonl(12+strlen(filename)+1);
            memcpy(buffer,&GET_REQUEST,sizeof(GET_REQUEST));
            memcpy(buffer+sizeof(GET_REQUEST),filename,strlen(filename)+1);
        
            /*----------send--------------------*/
            SEND(sock, buffer, 12+strlen(filename)+1, 0);
            
            /**-----------recv-----------------*/
            RECV(sock,buffer,12,0);//收GET_REPLY
            if(buffer[7]==(char)1){//文件存在
                printf("file exist!\n");
                RECV(sock,buffer,12,0);//收FILE_DATA Header
                if(buffer[6]!=(char)0xFF){
                    printf("error!It is not a filedata!\n");
                }
                int filesize=ntohl(*((unsigned int*)(buffer+8)))-12;
                printf("filesize:%d\n",filesize);
                char path[30];
                memset(path,0,sizeof(path));
                memcpy(path,"./",2);
                memcpy(path+2,filename,strlen(filename));
                printf("%s\n",path);
                int fd=open(path, O_WRONLY|O_CREAT|O_TRUNC,0644);
                //char payload[10];
                RECV(sock,payload,filesize,0);
                write(fd,payload,filesize);
                printf("File downloaded.\n");
            }
            else if(buffer[7]==(char)0){//文件不存在
                printf("file doesn't exist\n");
                continue;
            }
        }

        /*-------------put-------------*/
        if(strncmp(cmd,"put",3)==0){
            char filename[20];//要put的文件名称
            memset(filename,0,sizeof(filename));
            scanf("%s",filename);

            /*----------check if the file exist------------*/
            int filesize=file_size(filename);

            //char payload[10];
            char path[30];
            memset(path,0,sizeof(path));
            memcpy(path,"./",2);
            memcpy(path+2,filename,strlen(filename));
            int fd=open(path,O_RDONLY);
            if(fd<0){
                printf("the file dosn't exist!\n");
                continue;
            }
            read(fd,payload,filesize);
            /*------put request-----------*/
            char*s="\xe3myftp";
            memcpy(PUT_REQUEST.m_protocol,s,6);
            PUT_REQUEST.m_type=0xA9;
            PUT_REQUEST.m_length=htonl(12+strlen(filename)+1);
            memcpy(buffer,&PUT_REQUEST,sizeof(PUT_REQUEST));
            memcpy(buffer+sizeof(PUT_REQUEST),filename,strlen(filename)+1);
        
            /*----------send--------------------*/
            SEND(sock, buffer, 12+strlen(filename)+1, 0);

            RECV(sock,buffer,12,0);

            /*-----------set file data-----------*/
            memcpy(FILE_DATA.m_protocol,s,6);
            FILE_DATA.m_type=0xFF;
            FILE_DATA.m_length=htonl(12+filesize);
            memcpy(buffer,&FILE_DATA,sizeof(FILE_DATA));
            memcpy(buffer+sizeof(FILE_DATA),payload,filesize);

            SEND(sock,buffer,12+filesize,0);
            printf("File uploaded.\n");
            continue;
        }


        /*------------quit---------*/
        if(strncmp(cmd,"quit",4)==0){
            char*s="\xe3myftp";
            
            memcpy(QUIT_REQUEST.m_protocol,s,6);
            QUIT_REQUEST.m_type=0xAB;
            QUIT_REQUEST.m_length=htonl(12);

            memset(buffer,0,sizeof(buffer));
            memcpy(buffer,&QUIT_REQUEST,sizeof(QUIT_REQUEST));
            SEND(sock, buffer, 12, 0);

            RECV(sock,buffer,12,0);
            close(sock);
            printf("client exit\n");
            break;
        }
    }
    return 0;
}