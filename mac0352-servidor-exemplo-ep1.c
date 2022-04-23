/* Por Prof. Daniel Batista <batista@ime.usp.br>
 * Em 16/3/2022
 * 
 * Um código simples de um servidor de eco a ser usado como base para
 * o EP1. Ele recebe uma linha de um cliente e devolve a mesma linha.
 * Teste ele assim depois de compilar:
 * 
 * ./ep1-servidor-exemplo 8000
 * 
 * Com este comando o servidor ficará escutando por conexões na porta
 * 8000 TCP (Se você quiser fazer o servidor escutar em uma porta
 * menor que 1024 você precisará ser root ou ter as permissões
 * necessárias para rodar o código com 'sudo').
 *
 * Depois conecte no servidor via telnet. Rode em outro terminal:
 * 
 * telnet 127.0.0.1 8000
 * 
 * Escreva sequências de caracteres seguidas de ENTER. Você verá que o
 * telnet exibe a mesma linha em seguida. Esta repetição da linha é
 * enviada pelo servidor. O servidor também exibe no terminal onde ele
 * estiver rodando as linhas enviadas pelos clientes.
 * 
 * Obs.: Você pode conectar no servidor remotamente também. Basta
 * saber o endereço IP remoto da máquina onde o servidor está rodando
 * e não pode haver nenhum firewall no meio do caminho bloqueando
 * conexões na porta escolhida.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

#include <pthread.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <dirent.h> 


#define LISTENQ 1
#define MAXDATASIZE 100
#define MAXLINE 4096

struct linkedList {
    pthread_t thread;
    int fileDescriptor;
    char * topic;
    struct linkedList * next;
};

typedef struct linkedList LinkedList;

typedef struct {
    unsigned char type;
    unsigned char fixedHeaderFlags;
    unsigned char remainingLength;
    unsigned char * variableHeader;
    unsigned char * payload;
} Packet;

#define NONE 0
#define CONNECT 1
#define CONNACK 2
#define PUBLISH 3
#define SUBSCRIBE 8
#define SUBACK 9
#define DISCONNECT 14
#define SUCCESS_SUBSCRIBE 0x00

Packet createPacket(char * recvline, int n) {
    Packet packet; 

    packet.type = ((unsigned char) recvline[0]) >> 4;
    packet.fixedHeaderFlags = ((unsigned char) recvline[0])  & 0b1111;
    packet.remainingLength = (unsigned char) recvline[1];

    if (packet.remainingLength > 0) {
        packet.variableHeader = malloc(packet.remainingLength * sizeof(unsigned char));

        for (int i = 0; i < packet.remainingLength; i++)
            packet.variableHeader[i] = (unsigned char) recvline[i + 2];
    }
    else  
        packet.variableHeader = NULL;


    packet.payload = NULL;

    return packet;
}

Packet createConnackPacket() {
    Packet connackPacket;

    connackPacket.type = CONNACK;
    connackPacket.fixedHeaderFlags = 0;
    connackPacket.remainingLength = 2;

    connackPacket.variableHeader = malloc(2 * sizeof(unsigned char));

    connackPacket.variableHeader[0] = 0x00;
    connackPacket.variableHeader[1] = 0x00;

    connackPacket.payload = NULL;

    return connackPacket;
}

LinkedList * getLastNode(LinkedList * listOfTopicsNode) {
    while (listOfTopicsNode->next != NULL) 
        listOfTopicsNode = listOfTopicsNode->next;

    return listOfTopicsNode;
}

void * thread(void * arguments) {
    ssize_t n;
    unsigned char recvline[MAXLINE + 1];

    int fileDescriptor = ((int *) arguments)[0];
    int connfd = ((int *) arguments)[1];

    printf("Entrei na thread\n");
        fflush(stdout);

    while ((n=read(fileDescriptor, recvline, MAXLINE)) > 0) {
        printf("PACKET %d\n\n", n);
        for (int i = 0; i < n; i++) printf("%02x ", recvline[i]);
        printf("\n\n");
        fflush(stdout);
        printf("Printei no %d \n\n", connfd);
        fflush(stdout);
        write(connfd, recvline, n);
    }
    return NULL;
}

void * addTopic(char * topic, LinkedList *listOfTopicsNode, int connfd) {
    int * arguments = malloc(2*(sizeof(int)));
    char path[MAXLINE + 1];
    char pid[MAXLINE + 1];
    path[0] = '\0';

    strcat(path, "/tmp/");
    strcat(path, "ep1");
    strcat(path, "/");
    mkdir(path, 0777);
    strcat(path, topic);
    mkdir(path, 0777);
    strcat(path, "/");
    sprintf(pid, "%d", getpid());
    strcat(path, pid);
    strcat(path, ".falcon");

    listOfTopicsNode->next = malloc(sizeof(LinkedList));
    listOfTopicsNode = listOfTopicsNode->next;

    if (mkfifo(path,0644) == -1) {
        perror("mkfifo :(\n");
    }

    // listOfTopicsNode->fileDescriptor = open(path, O_WRONLY);
    // unlink(path); ///TODO: verificar se esta certo esse unlinks
    // close(listOfTopicsNode->fileDescriptor);
    
    listOfTopicsNode->fileDescriptor = open(path, O_RDWR);//TODO: verificar se esta certo usar read E write
    //unlink(path); ///TODO: verificar se esta certo esse unlink
    arguments[0] = listOfTopicsNode->fileDescriptor;
    arguments[1] = connfd;

    pthread_create(&(listOfTopicsNode->thread), NULL, thread, arguments);

    listOfTopicsNode->topic = topic;
    listOfTopicsNode->next = NULL;
}

Packet createSubackPacket(Packet subscribePacket, LinkedList * listOfTopicsNode, int connfd) {
    Packet subackPacket;

    subackPacket.type = SUBACK;
    subackPacket.fixedHeaderFlags = 0;
    subackPacket.remainingLength = 2;

    listOfTopicsNode = getLastNode(listOfTopicsNode);

    
    for (int i = 2; i < subscribePacket.remainingLength; i++) {
        int j;
        unsigned char size = subscribePacket.variableHeader[i] + subscribePacket.variableHeader[i + 1];
        i += 2;

        char *topic = malloc((size + 1)*sizeof(char));
        topic[size] = '\0';

        for (j = i; j < i + size; j++) {
            topic[j - i] = subscribePacket.variableHeader[j];
        }

        addTopic(topic, listOfTopicsNode, connfd);
        listOfTopicsNode = getLastNode(listOfTopicsNode);
        i = j;
        subackPacket.remainingLength++;
    }

    subackPacket.variableHeader = malloc(subackPacket.remainingLength*sizeof(unsigned char *));

    subackPacket.variableHeader[0] = subscribePacket.variableHeader[0];
    subackPacket.variableHeader[1] = subscribePacket.variableHeader[1];

    for (int i = 2; i < subackPacket.remainingLength; i++)
        subackPacket.variableHeader[i] = SUCCESS_SUBSCRIBE;

    subackPacket.payload = NULL;

    return subackPacket;
}

char * convertPacketToMessage(Packet packet, int *size) {
    char * message;
    *size = 0;

    if (packet.type == NONE) 
        return NULL;
    
    *size = 2 + packet.remainingLength;

    message = malloc((*size)*sizeof(char));
    message[0] = (packet.type << 4) | packet.fixedHeaderFlags;
    message[1] = packet.remainingLength;

    for (int i = 2; i < *size; i++)
        message[i] = (char) packet.variableHeader[i - 2];

    return message;
}

void publish(Packet publishPacket) {
    int pipe;
    int sizeOfMessage;
    char path[MAXLINE + 1];
    char pipePath[MAXLINE + 1];
    char remainingLength = (char) publishPacket.remainingLength;
    char msb = (char) publishPacket.variableHeader[0];
    char lsb = (char) publishPacket.variableHeader[1];
    int sizeOfTopic = msb + lsb;
    char begin = 2;
    char end = begin + sizeOfTopic;
    char * topic = malloc((sizeOfTopic + 1)*sizeof(char));
    topic[sizeOfTopic] = '\0';

    DIR *directory;
    struct dirent *file;
    

    for (char i = begin; i < end; i++)
        topic[i - begin] = publishPacket.variableHeader[i]; 
    
    strcat(path, "/tmp/");
    strcat(path, "ep1");
    strcat(path, "/");
    mkdir(path, 0777);
    strcat(path, topic);
    mkdir(path, 0777);
    strcat(path, "/");

    char * message = convertPacketToMessage(publishPacket, &sizeOfMessage);

    directory = opendir(path);
    if (directory) {
        while ((file = readdir(directory)) != NULL) {
            strcpy(pipePath, path);
            if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0) {
                strcpy(pipePath, path);
                strcat(pipePath, file->d_name);
                pipe = open(pipePath, O_RDWR);
                write(pipe, message, sizeOfMessage);
                close(pipe);
            }
        }
        closedir(directory);
    }


    printf("Topic: %s", topic);
    printf("\n Identificador do pacote: ");

    for (char i = end; i < remainingLength; i++)
        printf("%c", (char) publishPacket.variableHeader[i]);
    printf("\n\n\n");
}


int main (int argc, char **argv) {
    /* Os sockets. Um que será o socket que vai escutar pelas conexões
     * e o outro que vai ser o socket específico de cada conexão */
    int listenfd, connfd;
    /* Informações sobre o socket (endereço e porta) ficam nesta struct */
    struct sockaddr_in servaddr;
    /* Retorno da função fork para saber quem é o processo filho e
     * quem é o processo pai */
    pid_t childpid;
    /* Armazena linhas recebidas do cliente */
    unsigned char recvline[MAXLINE + 1];
    /* Armazena o tamanho da string lida do cliente */
    ssize_t n;

    LinkedList * listOfTopics = malloc(sizeof(LinkedList));
    listOfTopics->next = NULL;
   
    if (argc != 2) {
        fprintf(stderr,"Uso: %s <Porta>\n",argv[0]);
        fprintf(stderr,"Vai rodar um servidor de echo na porta <Porta> TCP\n");
        exit(1);
    }

    /* Criação de um socket. É como se fosse um descritor de arquivo.
     * É possível fazer operações como read, write e close. Neste caso o
     * socket criado é um socket IPv4 (por causa do AF_INET), que vai
     * usar TCP (por causa do SOCK_STREAM), já que o MQTT funciona sobre
     * TCP, e será usado para uma aplicação convencional sobre a Internet
     * (por causa do número 0) */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket :(\n");
        exit(2);
    }

    /* Agora é necessário informar os endereços associados a este
     * socket. É necessário informar o endereço / interface e a porta,
     * pois mais adiante o socket ficará esperando conexões nesta porta
     * e neste(s) endereços. Para isso é necessário preencher a struct
     * servaddr. É necessário colocar lá o tipo de socket (No nosso
     * caso AF_INET porque é IPv4), em qual endereço / interface serão
     * esperadas conexões (Neste caso em qualquer uma -- INADDR_ANY) e
     * qual a porta. Neste caso será a porta que foi passada como
     * argumento no shell (atoi(argv[1]))
     */
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(atoi(argv[1]));
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind :(\n");
        exit(3);
    }

    /* Como este código é o código de um servidor, o socket será um
     * socket passivo. Para isto é necessário chamar a função listen
     * que define que este é um socket de servidor que ficará esperando
     * por conexões nos endereços definidos na função bind. */
    if (listen(listenfd, LISTENQ) == -1) {
        perror("listen :(\n");
        exit(4);
    }

    printf("[Servidor no ar. Aguardando conexões na porta %s]\n",argv[1]);
    printf("[Para finalizar, pressione CTRL+c ou rode um kill ou killall]\n");
   
    /* O servidor no final das contas é um loop infinito de espera por
     * conexões e processamento de cada uma individualmente */
	for (;;) {
        /* O socket inicial que foi criado é o socket que vai aguardar
         * pela conexão na porta especificada. Mas pode ser que existam
         * diversos clientes conectando no servidor. Por isso deve-se
         * utilizar a função accept. Esta função vai retirar uma conexão
         * da fila de conexões que foram aceitas no socket listenfd e
         * vai criar um socket específico para esta conexão. O descritor
         * deste novo socket é o retorno da função accept. */
        if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1 ) {
            perror("accept :(\n");
            exit(5);
        }
      
        /* Agora o servidor precisa tratar este cliente de forma
         * separada. Para isto é criado um processo filho usando a
         * função fork. O processo vai ser uma cópia deste. Depois da
         * função fork, os dois processos (pai e filho) estarão no mesmo
         * ponto do código, mas cada um terá um PID diferente. Assim é
         * possível diferenciar o que cada processo terá que fazer. O
         * filho tem que processar a requisição do cliente. O pai tem
         * que voltar no loop para continuar aceitando novas conexões.
         * Se o retorno da função fork for zero, é porque está no
         * processo filho. */
        if ( (childpid = fork()) == 0) {
            /**** PROCESSO FILHO ****/
            printf("[Uma conexão aberta]\n");
            /* Já que está no processo filho, não precisa mais do socket
             * listenfd. Só o processo pai precisa deste socket. */
            close(listenfd);
         
            /* Agora pode ler do socket e escrever no socket. Isto tem
             * que ser feito em sincronia com o cliente. Não faz sentido
             * ler sem ter o que ler. Ou seja, neste caso está sendo
             * considerado que o cliente vai enviar algo para o servidor.
             * O servidor vai processar o que tiver sido enviado e vai
             * enviar uma resposta para o cliente (Que precisará estar
             * esperando por esta resposta) 
             */

            /* ========================================================= */
            /* ========================================================= */
            /*                         EP1 INÍCIO                        */
            /* ========================================================= */
            /* ========================================================= */
            /* TODO: É esta parte do código que terá que ser modificada
             * para que este servidor consiga interpretar comandos MQTT  */
            while ((n=read(connfd, recvline, MAXLINE)) > 0) {
                recvline[n]=0;
                printf("[Cliente conectado no processo filho %d enviou:] ",getpid());
                if ((fputs(recvline,stdout)) == EOF) {
                    perror("fputs :( \n");
                    exit(6);
                }
                Packet packet = createPacket(recvline, n);
                Packet packetToClient;

                packetToClient.type = NONE;

                switch (packet.type) {

                    case CONNECT:
                        printf("Connect:\n\n");
                        packetToClient = createConnackPacket();
                        break;
                    case PUBLISH:
                        printf("Publish:\n\n");
                        publish(packet);
                        break;
                    case SUBSCRIBE:
                        printf("Subscribe:\n\n");
                        printf("Socket do subscriber \n\n %d", connfd);
                        packetToClient = createSubackPacket(packet, listOfTopics, connfd);
                        break;
                    case DISCONNECT:
                        printf("Disconnect:\n\n");
                        break;
                    default:
                        printf("Comportamento não previsto - hexadecimal\n\n: %02x, %d", packet.type, packet.type);
                        break;
                }
                int size;
                char * message = convertPacketToMessage(packetToClient, &size);

                if (message != NULL)
                    write(connfd, message, size);
                fflush(stdout);
            }
            /* ========================================================= */
            /* ========================================================= */
            /*                         EP1 FIM                           */
            /* ========================================================= */
            /* ========================================================= */

            /* Após ter feito toda a troca de informação com o cliente,
             * pode finalizar o processo filho */
            printf("[Uma conexão fechada]\n");
            exit(0);
        }
        else
            /**** PROCESSO PAI ****/
            /* Se for o pai, a única coisa a ser feita é fechar o socket
             * connfd (ele é o socket do cliente específico que será tratado
             * pelo processo filho) */
            close(connfd);
    }
    exit(0);
}
