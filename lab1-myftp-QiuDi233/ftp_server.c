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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#define byte unsigned char
#define MAGIC_NUMBER_LENGTH 6
#define type unsigned char
#define status unsigned char

#define LISTENQ 128
char buf[2100];
char payload[1050010];
char filename[2048];

struct myftpHeader{
    byte m_protocol[MAGIC_NUMBER_LENGTH]; /* protocol magic number (6 bytes) */
    type m_type;                          /* type (1 byte) */
    status m_status;                      /* status (1 byte) */
    uint32_t m_length;                    /* length (4 bytes) in Big endian*/
} __attribute__ ((packed));
struct myftpHeader OPEN_CONN_REPLY;
struct myftpHeader AUTH_REPLY;
struct myftpHeader LIST_REPLY;
struct myftpHeader GET_REPLY;
struct myftpHeader PUT_REPLY;
struct myftpHeader QUIT_REPLY;
struct myftpHeader FILE_DATA;

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
        //printf("recv error:%s\n",strerror(errno));
        //printf("%ld\n",b);
        //if (b == 0) printf("socket Closed"); // 当连接断开
        if (b < 0) printf("Error ?"); // 这里可能发生了一些意料之外的情况
        ret += b; // 成功将b个byte塞进了缓冲区
        //printf("%ld\n",ret);
    }
}

void SEND(int sock,char*buffer,int len,int flags){
    size_t ret = 0;
    while (ret < len) {
        size_t b = send(sock, buffer + ret, len - ret, 0);
        printf("send b:%ld\n",b);
        if (b == 0) printf("socket Closed"); // 当连接断开
        if (b < 0) printf("Error ?"); // 这里可能发生了一些意料之外的情况
        ret += b; // 成功将b个byte塞进了缓冲区
    }
}

int main(int argc, char ** argv) {
    char*ip=argv[1];
    int port=atoi(argv[2]);


    /*声明服务器地址和客户链接地址*/  
    struct sockaddr_in servaddr , cliaddr;  
    bzero(&servaddr, sizeof(servaddr)); 
    /*声明服务器监听套接字和客户端链接套接字*/  
    int listenfd , connfd;  
    listenfd = socket(AF_INET , SOCK_STREAM , 0);

    socklen_t clilen; 

    /*设置服务器sockaddr_in结构*/  
    servaddr.sin_port = htons(port); // 在21端口监听 htons是host to network (short)的简称，表示进行大小端表示法转换，网络中一般使用大端法
    servaddr.sin_family = AF_INET; // 表示使用AF_INET地址族
    inet_pton(AF_INET, ip, &servaddr.sin_addr);
    

    /*绑定套接字和端口*/
    if((bind(listenfd , (struct sockaddr*)&servaddr , sizeof(servaddr)))<0){
        printf("bind error\n");
    }

    /*监听客户请求*/
    if((listen(listenfd , LISTENQ))<0){
        printf("listen error\n");
    }

    clilen = sizeof(cliaddr);
    //printf("line77\n");

    int connected=0;
    int auth=0;
    memset(buf,0,sizeof(buf));
    while(1){
        if(connected==0){
            //printf("not connected\n");
            if((connfd=accept(listenfd , (struct sockaddr *)&cliaddr , &clilen))>=0){
                printf("connect\n");
                connected=1;
                
                RECV(connfd,buf,12,0);
                
                //printf("check\n");
                if(buf[6]==(char)0xA1){//open
                    //printf("rev!\n");
                
                    char*s="\xe3myftp";
                    memcpy(OPEN_CONN_REPLY.m_protocol,s,6);
                    OPEN_CONN_REPLY.m_type=0xA2;
                    OPEN_CONN_REPLY.m_status=1;
                    OPEN_CONN_REPLY.m_length=htonl(12);
                    memcpy(buf,&OPEN_CONN_REPLY,sizeof(OPEN_CONN_REPLY));
                    SEND(connfd, buf, 12, 0);
                }
                else{
                    printf("recv failure\n");
                }
                //continue;
            }
            else{
                printf("connect error\n");
                //continue;
            }
        }
        if(connected){
            printf("already connected,waiting to RECV\n");
            RECV(connfd,buf,12,0);
            printf("RECV messages\n");
                if(auth==0&&buf[6]==(char)0xA3){//auth
                    auth=1;
                    printf("start to auth\n");
                    RECV(connfd,buf+12,12,0);//receive payload
                    /*-----------auth reply--------------*/
                    char*s="\xe3myftp";
                    memcpy(AUTH_REPLY.m_protocol,s,6);
                    AUTH_REPLY.m_type=(char)0xA4;
                    
                    AUTH_REPLY.m_length=htonl(12);
                    memcpy(buf,&AUTH_REPLY,sizeof(AUTH_REPLY));

                    if(strncmp(buf+12,"user",4)==0&&strncmp(buf+17,"123123",6)==0){
                        AUTH_REPLY.m_status=1;
                        memcpy(buf,&AUTH_REPLY,sizeof(AUTH_REPLY));
                        SEND(connfd, buf, 12, 0);
                    }
                    else{
                        AUTH_REPLY.m_status=0;
                        memcpy(buf,&AUTH_REPLY,sizeof(AUTH_REPLY));
                        SEND(connfd, buf, 12, 0);
                    }
                    printf("auth finish\n");
                    //continue;
                }
                /*-------------list----------------*/
                if(auth==1&&buf[6]==(char)0xA5){
                    printf("start to list\n");
                    
                    memset(filename,0,sizeof(filename));
                    char tmp[100];
                    FILE *fp = NULL;
                    fp = popen("ls", "r");
                    int cnt=0;
                    while(memset(tmp, 0, sizeof(tmp)), fgets(tmp, sizeof(tmp), fp) != 0 ) {
                        //printf("tmp:%s",tmp);
                        //printf("cnt:%d\n",cnt++);
                        memcpy(filename+strlen(filename),tmp,strlen(tmp));
                        //printf("filename:%s\n",filename);
                    }
                    int len=strlen(filename);
                    filename[len]='\0';
                    
                    printf("files:%s\n",filename);
                    printf("len:%d\n",len);
                    pclose(fp);
                    
                    /*---------list reply-------*/
                    char*s="\xe3myftp";
                    memcpy(LIST_REPLY.m_protocol,s,6);
                    LIST_REPLY.m_type=(char)0xA6;
                    LIST_REPLY.m_length=htonl(12+len+1);
                    memcpy(buf,&LIST_REPLY,sizeof(LIST_REPLY));
                    memcpy(buf+sizeof(LIST_REPLY),filename,len+1);
                    printf("buf:%ld\n",sizeof(LIST_REPLY)+strlen(filename)+1);

                    /*---------send-------------*/
                    SEND(connfd,buf,12+len+1,0);

                }

                /*-----------get--------------*/
                if(auth==1&&buf[6]==(char)0xA7){
                    int length=ntohl(*((unsigned int*)(buf+8)))-12;
                    char filename[30];
                    memset(filename,0,sizeof(filename));
                    RECV(connfd,filename,length,0);
                    printf("filename:%s\n",filename);
                    /*--------set the get reply--------*/
                    char*s="\xe3myftp";
                    memcpy(GET_REPLY.m_protocol,s,6);
                    GET_REPLY.m_type=(char)0xA8;
                    GET_REPLY.m_length=htonl(12);

                    /*check if there is the file------*/
                    int filesize=file_size(filename);
                    char path[30];
                    memset(path,0,sizeof(path));
                    memcpy(path,"./",2);
                    memcpy(path+2,filename,strlen(filename));
                    int fd=open(path,O_RDONLY);
                    if(fd<0){
                        GET_REPLY.m_status=0;
                        memcpy(buf,&GET_REPLY,sizeof(GET_REPLY));
                        SEND(connfd,buf,12,0);
                        continue;
                    }
                    GET_REPLY.m_status=1;
                    memcpy(buf,&GET_REPLY,sizeof(GET_REPLY));
                    SEND(connfd,buf,12,0);//send the header

                    //char payload[10];
                    read(fd,payload,filesize);

                    memcpy(FILE_DATA.m_protocol,s,6);
                    FILE_DATA.m_type=0xFF;
                    FILE_DATA.m_length=htonl(12+filesize);
                    memcpy(buf,&FILE_DATA,sizeof(FILE_DATA));
                    memcpy(buf+sizeof(FILE_DATA),payload,filesize);

                    SEND(connfd,buf,12+filesize,0);//send the data
                }

                /*-----------put-----------------*/
                if(auth==1&&buf[6]==(char)0xA9){
                    /*--------recv put request payload-----*/
                    char Filename[30];
                    int len=ntohl(*((unsigned int*)(buf+8)))-12;
                    memset(Filename,0,sizeof(Filename));
                    RECV(connfd,Filename,len,0);
                    printf("Filename:%s\n",Filename);

                    printf("server put reply\n");
                    char*s="\xe3myftp";
                    memcpy(PUT_REPLY.m_protocol,s,6);
                    PUT_REPLY.m_type=(char)0xAA;
                    PUT_REPLY.m_length=htonl(12);
                    memcpy(buf,&PUT_REPLY,sizeof(PUT_REPLY));
                    SEND(connfd,buf,12,0);

                    /*recv file data header*/
                    printf("start to recv data header\n");
                    RECV(connfd,buf,12,0);
                    int filesize=ntohl(*((unsigned int*)(buf+8)))-12;
                    printf("filesize:%d\n",filesize);
                    char path[30];
                    memset(path,0,sizeof(path));
                    memcpy(path,"./",2);
                    memcpy(path+2,Filename,strlen(Filename));
                    printf("%s\n",path);
                    int fd=open(path, O_WRONLY|O_CREAT|O_TRUNC,0644);
                    /*recv payload*/
                    //char payload[10];
                    RECV(connfd,payload,filesize,0);
                    write(fd,payload,filesize);
                    printf("Server:File downloaded.\n");
                }


                /*----------quit---------------*/
                if(auth==1&&buf[6]==(char)0xAB){
                    char*s="\xe3myftp";
                    memcpy(QUIT_REPLY.m_protocol,s,6);
                    QUIT_REPLY.m_type=(char)0xAC;
                    QUIT_REPLY.m_length=htonl(12);
                    memcpy(buf,&QUIT_REPLY,sizeof(QUIT_REPLY));

                    SEND(connfd,buf,12,0);
                }
        }
    }
    
    return 0;
}