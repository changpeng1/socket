#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <stdlib.h>
using namespace std;
#define PORT 6000
#define MSG_SIZE 1024
char* work_mode;
class Group
{
    private:
        string Name;
        string Uuid;
        string Type;
        string Ip;
        unsigned int Port;
    public:
        Group(string &name,string &uuid,string &type,string &ip,unsigned int port):Name(name),Uuid(uuid),Type(type),Ip(ip),Port(port){};
        string get_ip(){return Ip;};
};
vector<class Group> g_group;
int sender(char * mode)
{
    string mesg;
    if(!strcmp(mode,"master"))
    {
        mesg="ACTION:SEARCH\r\n"
        "TYPE:Master\r\n"
        "UUID:9087392-wudjdk-eujdkxm\r\n"
        "NAME:test\r\n"
        "HOST:192.168.1.101\r\n"
        "PORT:6001\r\n";
    }
    else
    {
        mesg="ACTION:NOTIFY\r\n"
        "TYPE:Slave\r\n"
        "UUID:9087392-wudjdk-eujdkxm\r\n"
        "NAME:test\r\n"
        "HOST:192.168.1.101\r\n"
        "PORT:6001\r\n";
        work_mode="slave";
    }
    int sock = -1;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {   
        cout<<"socket error"<<endl; 
        return false;
    }   

    const int opt = 1;
    int nb = 0;
    nb = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));
    if(nb == -1)
    {
        cout<<"set socket error..."<<endl;
        return false;
    }

    struct sockaddr_in addrto;
    bzero(&addrto, sizeof(struct sockaddr_in));
    addrto.sin_family=AF_INET;
    addrto.sin_addr.s_addr=htonl(INADDR_BROADCAST);
    addrto.sin_port=htons(PORT);
    int nlen=sizeof(addrto);
    int i =2;
    while(i>0)
    {
        int ret=sendto(sock, mesg.c_str(),mesg.length(), 0, (sockaddr*)&addrto, nlen);
        if(ret<0)
        {
            cout<<"send error...."<<ret<<endl;
        }
        i-=1;
        sleep(1);
    }
    while(1) sleep(1);
    close(sock);
    return 0;
}
void * thread_func(void * data)
{
    if(data)
        sender((char*)data);
}
int main(int argc,char*argv[])
{
    if(argc<2)
    {
        printf("please input the mode,master or slave\n");
        return -1;
    }
    work_mode = argv[1];
    pthread_t tid;
    pthread_create(&tid,NULL,thread_func,(void*)work_mode);

    struct sockaddr_in addrto;
    bzero(&addrto, sizeof(struct sockaddr_in));
    addrto.sin_family = AF_INET;
    addrto.sin_addr.s_addr = htonl(INADDR_ANY);
    addrto.sin_port = htons(PORT);

    struct sockaddr_in from;
    bzero(&from, sizeof(struct sockaddr_in));
    from.sin_family = AF_INET;
    from.sin_addr.s_addr = htonl(INADDR_ANY);
    from.sin_port = htons(PORT);

    int sock = -1;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {   
        cout<<"socket error"<<endl; 
        return false;
    }   

    const int opt = 1;
    int nb = 0;
    nb = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));
    if(nb == -1)
    {
        cout<<"set socket error..."<<endl;
        close(sock);
        return false;
    }
    nb = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    if(nb == -1)
    {
        cout<<"set socket error..."<<endl;
        close(sock);
        return false;
    }
    if(bind(sock,(struct sockaddr *)&(addrto), sizeof(struct sockaddr_in)) == -1) 
    {   
        cout<<"bind error..."<<endl;
        close(sock);
        return false;
    }
    int len = sizeof(sockaddr_in);
    char smsg[MSG_SIZE] = {0};

    while(1)
    {
        int ret=recvfrom(sock, smsg, MSG_SIZE, 0, (struct sockaddr*)&from,(socklen_t*)&len);
        if(ret<=0)
        {
            cout<<"read error...."<<sock<<endl;
        }
        else
        {       
            printf("recieve from host %s\n%s\n", inet_ntoa( from.sin_addr),smsg);   
            string uuid,type,name,ip;
            unsigned int port;
            ip = inet_ntoa( from.sin_addr);
            bool has_entry= false;
            char *str1, *str2, *token, *subtoken;
            char *saveptr1, *saveptr2;
            int j;
            for(vector<class Group>::iterator i = g_group.begin();i!=g_group.end();i++)
            {
                if((*i).get_ip()==ip)
                {
                    has_entry = true;
                    break;
                }
            }

            for (j = 1, str1 = smsg; ; j++, str1 = NULL) {
                token = strtok_r(str1, "\r\n", &saveptr1);
                if (token == NULL)
                    break;
            //    printf("-- %s\n", token);
                string temp = token;
                string::size_type pos1=temp.find(":");
                string key = temp.substr(0,pos1);
                string value =temp.substr(pos1+1);
            //    cout<<key<<"*"<<value<<endl;
                if(key=="ACTION") 
                {
                    if(value=="SEARCH")
                    {
                        if(!strcmp(work_mode,"slave"))//此设备是从设备，收到search消息，需要发送notify，适用于从设备先启动，主设备后启动
                        {
                            sender("slave");
                            break;
                        }
                    }
                }
                if(!has_entry)
                {
                    if(key == "TYPE") type = value;
                    else if(key == "UUID") uuid=value;
                    else if(key == "NAME") name=value;
                    else if(key == "PORT") port = atoi(value.c_str());
                    Group group(name,uuid,type,ip,port);
                    g_group.push_back(group);
                }
            }
            for(vector<class Group>::iterator i = g_group.begin();i!=g_group.end();i++)
            {
        //        cout<<"Recv from host: "<<(*i).get_ip()<<endl;
            } 

        }

        sleep(1);
    }
    close(sock);
    return 0;
}
