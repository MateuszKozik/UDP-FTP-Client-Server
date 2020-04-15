#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#define SERV_PORT 45000

int main(void)
{
    struct sockaddr_in serwer_addres, klient_addres;
    int listenfd;
    int klient_len;
    int recv_len;
    int pid;
    char bufor[1024];

    //utworzenie socketu
    listenfd=socket(AF_INET, SOCK_DGRAM, 0);
	memset((char *) &serwer_addres, '1', sizeof(serwer_addres));

    serwer_addres.sin_family = AF_INET;
    serwer_addres.sin_port = SERV_PORT;
    serwer_addres.sin_addr.s_addr = htonl(INADDR_ANY);


    bind(listenfd , (struct sockaddr*)&serwer_addres, sizeof(serwer_addres));
 
 	//pobranie rozmiaru klienta
 	klient_len = sizeof(klient_addres); 

    while(1){

    	//pobranie typu operacji
    	char typ[20];
		recvfrom(listenfd, bufor, 20, 0, &klient_addres, &klient_len);
		strcpy(typ, bufor);
		memset(bufor,0,1024);

		//pobieranie pliku od klienta
		if(strcmp(typ, "w") == 0){ 
			printf("\n\n--- ZAPISYWANIE PLIKU NA SERWERZE ---");

		 	//zmienna przechowujaca nazwę pliku
			char plik[20];
				
			//pobranie nazwy pliku
			recvfrom(listenfd, bufor, 20, 0, &klient_addres, &klient_len);
			strcpy(plik, bufor);
			printf("\nPobierany plik : %s", plik);

			//reset buforfora (zapelnienie go zerami)
			memset(bufor,0,1024);

			//pobranie rozmiaru pliku
			recvfrom(listenfd, bufor, 20, 0, &klient_addres, &klient_len);
			unsigned long rozmiar = atoi(bufor);
			memset(bufor,0,1024);

			//otworzenie pliku w trybie do zapisu binarnego
			FILE *f;
			f=fopen(plik,"wb");
			int licznik=1;
			  
			//pobieranie pliku
			while(licznik*1024<rozmiar)
			{
			    recvfrom(listenfd, bufor, 1024, 0, &klient_addres, &klient_len);
			    fwrite(bufor,1024, 1, f);
			    memset(bufor,0,1024);
			    licznik++;
			}

			//pobranie ostatniego bloku danych, ktory zostal pominiety przy wykonywaniu petli
			recvfrom(listenfd, bufor, (rozmiar%1024), 0, &klient_addres, &klient_len);
			fwrite(bufor,(rozmiar%1024), 1, f);
			memset(bufor,0,1024);

			printf("\nPlik %s zostal pobrany", plik);

			//zamknieci pliku
			fclose(f);
		}
	
		//wysylanie pliku do klienta
	   	else if(strcmp(typ, "p") == 0) {
			printf("\n\n--- WYSYLANIE PLIKU DO KLIENTA ---");

	   		//zmienna przechowująca nazwę pliku
	   		char plik[20];

	   		//pobranie nazwy pliku
	   		recvfrom(listenfd, bufor, 20, 0, &klient_addres, &klient_len);

	   		//utworzenie procesu potmnego
	   		pid=fork();

	   		//obsluga przekazana do procesu potomnego 
	   		if(pid==0) {
	   			int id = getpid();
				strcpy(plik, bufor);
				printf("\n[%d] Plik do wysłania : %s", id,plik);
				memset(bufor,0,1024);
			
				char sprawdz[20];

				//otworzenie pliku w trybie do odczytu binarnego
			    FILE *f;
			    f=fopen(plik,"rb");
			    if(access(plik, F_OK)) {
			        strcpy(sprawdz, "n");
			        sendto(listenfd, sprawdz, 20 , 0 , &klient_addres, klient_len);

            		printf("\nNie ma takiego pliku: %s", plik);

            		//zamkniecie socketu
			        close(listenfd);
			        exit(0);

            	}
            	else {
            		strcpy(sprawdz, "j");
					sendto(listenfd, sprawdz, 20 , 0 , &klient_addres, klient_len);

            		//pobranie rozmiaru pliku
				    fseek(f, 0, SEEK_END);
	    			unsigned long rozmiar = (unsigned long)ftell(f);
	    			fseek(f, 0, SEEK_SET);

	    			//konwersja rozmiaru na napis
				    char rozmiarPliku[10];
				    sprintf(rozmiarPliku, "%d", rozmiar);

				    //przeslanie rozmiaru pliku do klienta
				    sendto(listenfd, rozmiarPliku, 20 , 0 , &klient_addres, klient_len);		      

				    //wczytanie pierwszego bloku danych do buforora
				    fread(bufor, 1024,1,f);

				    //przesylanie kolenych blokow danych az do zakonczenia pliku
				    int licznik=1;
				    while(licznik*1024<rozmiar){

				    	//uspienie procesu
				    	sleep(1);

				       	sendto(listenfd, bufor, 1024 , 0 , &klient_addres, klient_len);
				        memset(bufor,0,1024);
				        fread(bufor, 1024,1,f);
				        licznik++;
				    }

				    //przeslanie ostatniego bloku danych, ktory zostal pominiety przy wykonywaniu petli
				    fread(bufor, (rozmiar % 1024),1,f);
				    sendto(listenfd, bufor, (rozmiar % 1024) , 0 ,&klient_addres, klient_len);
				    memset(bufor,0,1024);

				    //zamkniecie pliku
				    fclose(f);

				    printf("\n[%d] Plik %s został wysłany",id, plik);

				    //zamkniecie socketu
				    close(listenfd);
				    exit(0);
            	}
	   		}

	   		//blad podczas tworzenia procesu potomnego
	   		if(pid < 0){
	   			printf("Nie udalo sie utworzyc nowego procesu");
	   			exit(1);
	   		}	
	   	}
    }

  	//zamkniecie socketu
    close(listenfd);

    return 0;
}
