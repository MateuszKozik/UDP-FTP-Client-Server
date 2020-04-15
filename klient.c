#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#define SERV_PORT 45000

int main(int argc, char** argv)
{
    struct sockaddr_in serwer_addres;
    int listenfd;
    int serwer_len;
    char bufor[1024];

    if(argc<2){
    	printf("Uruchamiajac program podaj adres serwer\n");
    	return 0;
    }

    listenfd=socket(AF_INET, SOCK_DGRAM, 0);
 	memset((char *) &serwer_addres, '1', sizeof(serwer_addres));

    serwer_addres.sin_family = AF_INET;
    serwer_addres.sin_port = SERV_PORT;
    inet_pton(AF_INET , argv[1], &serwer_addres.sin_addr);
 
 	serwer_len = sizeof(serwer_addres);

    while(1){
        //wybranie typu operacji
        char typ[20];
        printf("\n--- Wybierz operacje ---");
        printf("\n[w] - wgrywanie pliku na serwer");
        printf("\n[p] - pobieranie pliku z serwera");
        printf("\n[z] - zakoncz program\n");
        scanf("%s", &typ);

        //wysyłanie
        if(strcmp(typ, "w") == 0){

         	//zmienna przechowujaca nazwe pliku
            char plik[20];
            printf("Wprowadz nazwe pliku: ");
            scanf("%s",&plik);

            //otworzenie pliku w trybie do odczytu binarnego
            FILE *f;
            f=fopen(plik,"rb");

            if(access(plik, F_OK)){
            	printf("\nNie ma takiego pliku: %s", plik);
            } else{
            	//przeslanie informacji o wybranej operacji
	            sendto(listenfd, typ, 20 , 0 , (struct sockaddr *) &serwer_addres, serwer_len);

	            //reset buffora (zapelnienie go zerami)
	            memset(bufor,0,1024);        

	            //przeslanie nazwy pliku
	            sendto(listenfd, plik, 20 , 0 , (struct sockaddr *) &serwer_addres, serwer_len);
	            memset(bufor,0,1024);
				printf("Plik %s zostanie wyslany.\n",plik);

	            //pobranie rozmiaru pliku
				fseek(f, 0, SEEK_END);
	    		unsigned long rozmiar = (unsigned long)ftell(f);
	    		fseek(f, 0, SEEK_SET);
	            char rozmiarPliku[10];
	            sprintf(rozmiarPliku, "%d", rozmiar);

	            //przeslanie rozmiaru pliku do serwera
	            sendto(listenfd, rozmiarPliku, 20 , 0 , (struct sockaddr *) &serwer_addres, serwer_len);

	            memset(bufor,0,1024);

	            //wczytanie pierwszego bloku danych do bufora
	            fread(bufor, 1024,1,f);

	            //przesylanie kolenych blokow danych az do zakonczenia pliku
	            int licznik =1;
	            while(licznik*1024<rozmiar){
	                sendto(listenfd, bufor, 1024 , 0 , (struct sockaddr *) &serwer_addres, serwer_len);
	                memset(bufor,0,1024);
	                fread(bufor, 1024,1,f);
	                licznik++;
	            }

	            //przeslanie ostatniego bloku danych, ktory zostal pominiety przy wykonywaniu petli
	            fread(bufor, (rozmiar % 1024),1,f);
	            sendto(listenfd, bufor, (rozmiar % 1024) , 0 , (struct sockaddr *) &serwer_addres, serwer_len);
	            memset(bufor,0,1024);

	            //zamkniecie pliku
	            fclose(f);

	            printf("\n Plik %s został wysłany", plik);
            }
        }

        //pobieranie
        else if(strcmp(typ, "p") == 0) {
            //przeslanie informacji o wybranej operacji
            sendto(listenfd, typ, 20 , 0 , (struct sockaddr *) &serwer_addres, serwer_len);
            memset(bufor,0,1024);

            //zmienna przechowujaca nazwe pliku
            char plik[20];
            printf("Wprowadz nazwe pliku: ");
            scanf("%s",&plik);
            printf("Plik %s zostanie pobrany.",plik);

            //przeslanie nazwy pliku
            sendto(listenfd, plik, 20 , 0 , (struct sockaddr *) &serwer_addres, serwer_len);
            memset(bufor,0,1024);

			char sprawdz[20];
           	recvfrom(listenfd, bufor, 20, 0, (struct sockaddr *) &serwer_addres, &serwer_len);
           	strcpy(sprawdz, bufor);
			memset(bufor,0,1024);
			if(strcmp(sprawdz, "n") == 0){
				printf("\nNie ma takiego pliku %s", plik);
			}
			else if(strcmp(sprawdz, "j") == 0){
				//pobranie rozmiaru pliku
	            recvfrom(listenfd, bufor, 20, 0, (struct sockaddr *) &serwer_addres, &serwer_len);
	            unsigned long rozmiar = atoi(bufor);

	            //otworzenie pliku w trybie do zapisu binarnego
	            FILE *f;
	            f=fopen(plik,"wb");
	            int licznik=1;
	            memset(bufor,0,1024);

	            //pobieranie pliku
	            while(licznik*1024<rozmiar)
	            {
	                recvfrom(listenfd, bufor, 1024, 0, (struct sockaddr *)&serwer_addres,&serwer_len);
	                fwrite(bufor,1024, 1, f);
	                memset(bufor,0,1024);
	                licznik++;
	            }

	            //pobranie ostatniego bloku danych, ktory zostal pominiety przy wykonywaniu petli
	            recvfrom(listenfd, bufor, (rozmiar%1024), 0, (struct sockaddr *) &serwer_addres, &serwer_len);
	            fwrite(bufor,(rozmiar%1024), 1, f);
	            memset(bufor,0,1024);

	            printf("\nPlik %s zostal pobrany", plik);
	                
	            //zamkniecie pliku
	            fclose(f);
			}
        }
        else if(strcmp(typ, "z") == 0){
        	printf("\nKoniec programu!\n");

        	//zamkniecie socketu
    		close(listenfd);
        	return 0;
        }
        printf("\n\n---------------------------");
    }
   
    //zamkniecie socketu
    close(listenfd);

    return 0;
}
