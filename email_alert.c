#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void init(){
    system("rpm -qa | grep postfix > installchk");
    char temp[256];
    FILE *fp = fopen("installchk","r");
    fgets(temp,sizeof(temp),fp);
    fclose(fp);
    system("rm installchk");
    if(strcmp(temp,"")==0){
        system("yum install postfix");
    }
    FILE *fp2 = fopen("/etc/postfix/main.cf","a+");
    int i;
    int chk = 0;
    while(!feof(fp2)){
        fgets(temp,sizeof(temp),fp2);
        if(strstr(temp,"[smtp.gmail.com]:587")){
            chk = 1;
        }
    }
    if(!chk){
        printf("FIX /etc/postfix/main.cf");
        char  maincf[6][100] = {"relayhost = [smtp.gmail.com]:587\n",
            "smtp_use_tls = yes\n", 
            "smtp_sasl_auth_enable = yes\n", 
            "smtp_sasl_security_options = noanonymous\n",
            "smtp_tls_CAfile = /etc/postfix/cacert.pem\n",
            "smtp_sasl_password_maps = hash:/etc/postfix/gmail\n"};
        for(i = 0; i < 6; i++){
            fputs(maincf[i], fp2);
        }
    }
    fclose(fp2);
    if(access("/etc/postfix/gmail",00) == -1){
        char ID[30] = {};
        char PW[30] = {};
        temp[0] = 0;
        strcat(temp,"[smtp.gmail.com]:587 ");
        printf("Google 계정 등록\n");
        printf("Email 주소: ");
        scanf("%s",ID);
        strcat(temp,ID);
        strcat(temp,":");
        printf("비밀번호: ");
        scanf("%s",PW);
        strcat(temp,PW);

        FILE *fp3 = fopen("/etc/postfix/gmail","w");
        fputs(temp,fp3);
        fclose(fp3);
    }
    system("chmod 600 /etc/postfix/gmail");
    system("postmap /etc/postfix/gmail");
    if(access("/etc/pki/tls/certs/hostname.pem",00) == -1){
        printf("Error : 인증서 누락\n");
        printf("경로: \"/etc/pki/tls/certs/\"로 이동 후 \"sudo make hostname.pem\"를 실행 후 다시 프로그램을 실행시켜주십시오\n");
        exit(1);
    } 
    system("cp /etc/pki/tls/certs/hostname.pem /etc/postfix/cacert.pem");
    system("service postfix restart");
    
    temp[0]=0;
    system("rpm -qa | grep mailx > mailxtest");
    FILE *fp4 = fopen("mailxtest","r");
    fgets(temp,sizeof(temp),fp4);
    fclose(fp4);
    system("rm mailxtest");
    if(strcmp(temp,"")==0){
        system("yum install mailx");
    }
    
    if(access("addr",00)==-1){
        FILE *fp6 = fopen("addr","w");
        char to[30];
        printf("==========이메일 주소 초기설정==========\n");
        printf("수신할 이메일 주소를 입력하십시오 : ");
        scanf("%s",to);
        fputs(to,fp6);
        fclose(fp6);
    }
    
    FILE *fp7 = fopen("addr","r");
    char To[30];
    fgets(To,sizeof(To),fp7);
    send_mail(To,"Mail alert Just Started","Mail alert service is successfully Registed\nAlert mail will arrive when Server overload");
}

void send_mail(char *addr, char *title, char *message){
    char buf[1000] = {};
    strcat(buf,"mail -s \"");
    strcat(buf,title);
    strcat(buf,"\" ");
    strcat(buf, addr);
    strcat(buf, " <<< \'");
    strcat(buf, message);
    strcat(buf, "\'");
    system(buf);
}

void chkstatus(void){
    system("/usr/local/bin/sar 1 1 -r -u > statusnow");
    FILE *fp1 = fopen("statusnow","r");
    int count = 0,i,j;
    char buffer[100], cpu[10], ram[10];
    while( !feof( fp1 ) ){
        fgets( buffer, sizeof(buffer), fp1 );
        count ++;

        if(count == 4){
            j=0;
            for(i = -21; i < -14; i ++){
                cpu[j] = buffer[sizeof(buffer)+i];
                j++;
            }
            cpu[j]=0;
        }
        else if(count == 8){
            j=0;
            for(i = 20; i<28; i++){
                ram[j] = buffer[i];
                j++;
            }
            ram[j]=0;
        }
    }
    fclose(fp1);
    system("rm statusnow");    
    
    float cpu_free = atof(cpu);
    int ram_free = atoi(ram);
    int cpu_alert = 0, ram_alert = 0;
    char *msg[2][2] = {{"YELLOW LIGHT! One of CPU or MEMORY are Overloading!", "MEMORY is overloading now!!!"},
                        {"CPU is overloading now!!!", "RED LIGHT!! Both of CPU and MEMORY are Overloading!!"}};
    FILE *fp2 = fopen("addr","r");
    char To[30], Message[200]="";
    fgets(To,sizeof(To),fp2);
    fclose(fp2);
    printf("\n\n\n\n==========================================================\n");
    printf("CPU STATUS: ");
    if(cpu_free < 20){
        cpu_alert = 1;
        printf(ANSI_COLOR_RED"DANGER    "ANSI_COLOR_RESET);
    }
    else{printf(ANSI_COLOR_GREEN"SAFE      "ANSI_COLOR_RESET);}
    
    printf("MEMORY STATUS: ");
    if(ram_free < 100000){
        ram_alert = 1;
        printf(ANSI_COLOR_RED"DANGER"ANSI_COLOR_RESET);
    }
    else{printf(ANSI_COLOR_GREEN"SAFE"ANSI_COLOR_RESET);}
    printf("\n==========================================================\n");

    if(cpu_alert + ram_alert > 0){
        if(cpu_alert + ram_alert == 1){
            strcat(Message,msg[0][0]);
            strcat(Message,"\n");
        }
        strcat(Message,msg[cpu_alert][ram_alert]);
        send_mail(To,"[SYSSTAT]Check Your Server!",Message);
    }
}

void print_Ascii_Art(FILE *fptr)
{
    char read_string[500];

    while(fgets(read_string,sizeof(read_string),fptr) != NULL)
        printf(ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET,read_string);
}

void main(){
    init();
    int t;
    printf("몇 분 주기로 측정하시겠습니까? : ");
    scanf("%d",&t);
    
    time_t ct;
    struct tm tm;

    while(1){
        system("clear");
        FILE *img = fopen("logo","r");
        print_Ascii_Art(img);
        chkstatus();
        ct = time(NULL);
        tm = *localtime(&ct);
        printf("recent Check: %d-%d-%d %d:%d:%d\n",
         tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
         tm.tm_hour, tm.tm_min, tm.tm_sec);
        printf("\nPress");
        printf( ANSI_COLOR_MAGENTA "CTRL" ANSI_COLOR_RESET);
        printf(" + ");
        printf( ANSI_COLOR_MAGENTA "C"  ANSI_COLOR_RESET);
        printf(" to turn off...\n");
        sleep(t*60);
    }
}
