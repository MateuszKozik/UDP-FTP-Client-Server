#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<ncurses.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#define SERV_PORT 45000

int main(int argc, char** argv)
{
    struct sockaddr_in serwer_addres;
    int listenfd;
    int serwer_len;
    char bufor[1024];
    char typ[3][20] = {"Wgraj plik", "Pobierz plik", "Zakoncz program"}; 
    int klawisz;
    int wybor=0;

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


    initscr();

	//utworzenie okien
	WINDOW * naglowek = newwin(5, 30, 0, 15);
	WINDOW * menu = newwin(10, 50, 6 , 5);

	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_BLUE, COLOR_BLACK);

	box(naglowek,0,0);
	box(menu, 0,0);
	
	wattroff(menu, COLOR_PAIR(3));


	refresh();
	wrefresh(naglowek);
	wrefresh(menu);

	//aktywowanie wykorzystania strzalek
	keypad(menu, true);

    while(1){
		
    	wattron(naglowek, A_BOLD);
    	mvwprintw(naglowek, 2, 5, "--- MENU GLOWNE ---");
		wattroff(naglowek, A_BOLD);
    	wrefresh(naglowek);
    	
    	//obsluga menu
	   	while(1){
	   		for(int i=0; i<3; i++){
	   			if
	   				(i == wybor)
	   				wattron(menu, A_REVERSE);
	   			mvwprintw(menu, i+3, 17, typ[i]);
	   			wattroff(menu, A_REVERSE);
	   		}
	   		klawisz = wgetch(menu);
	   		switch(klawisz){
	   			case KEY_UP:
	   				wybor--;
	   				if(wybor == -1)
	   					wybor = 0;
	   				break;
	   			case KEY_DOWN:
	   				wybor++;
	   				if(wybor == 3)
	   					wybor = 2;
	   				break;
	   			default:
	   				break;
	   		}
	   		if(klawisz == 10)
	   			break;
	   	}
	   	werase(menu);
	   	box(menu, 0,0);
		wrefresh(menu);


       	werase(naglowek);
	   	box(naglowek, 0,0);
		wrefresh(naglowek);

        //wysyłanie
        if(strcmp(typ[wybor], "Wgraj plik") == 0){

        	wattron(naglowek, A_BOLD);
			mvwprintw(naglowek, 2, 3, "--- WGRYWANIE PLIKU ---");
			wattroff(naglowek, A_BOLD);
    		wrefresh(naglowek);

         	//zmienna przechowujaca nazwe pliku
            char plik[20];
            mvwprintw(menu, 2, 11, "Wprowadz nazwe pliku: "); 
            mvwscanw(menu, 3, 11, "%s",&plik);;

            //otworzenie pliku w trybie do odczytu binarnego
            FILE *f;
            f=fopen(plik,"rb");

            if(access(plik, F_OK)){
            	mvwprintw(menu, 6, 11, "Nie znaleziono pliku \"%s\"", plik);
            	wrefresh(menu);
            	getch();
            	werase(menu);
	   			box(menu, 0,0);
				wrefresh(menu);

				werase(naglowek);
				box(naglowek,0,0);
				wrefresh(naglowek);

            } else{
            	//przeslanie informacji o wybranej operacji
	            sendto(listenfd, typ[wybor], 20 , 0 , &serwer_addres, serwer_len);

	            //reset buffora (zapelnienie go zerami)
	            memset(bufor,0,1024);        

	            //przeslanie nazwy pliku
	            sendto(listenfd, plik, 20 , 0 , &serwer_addres, serwer_len);
	            memset(bufor,0,1024);
				mvwprintw(menu, 6, 11, "Trwa wysylanie pliku \"%s\"",plik);
				wrefresh(menu);

	            //pobranie rozmiaru pliku
				fseek(f, 0, SEEK_END);
	    		unsigned long rozmiar = (unsigned long)ftell(f);
	    		fseek(f, 0, SEEK_SET);
	            char rozmiarPliku[10];
	            sprintf(rozmiarPliku, "%d", rozmiar);

	            //przeslanie rozmiaru pliku do serwera
	            sendto(listenfd, rozmiarPliku, 20 , 0 , &serwer_addres, serwer_len);

	            memset(bufor,0,1024);

	            //wczytanie pierwszego bloku danych do bufora
	            fread(bufor, 1024,1,f);

	            //przesylanie kolenych blokow danych az do zakonczenia pliku
	            int licznik =1;
	            while(licznik*1024<rozmiar){
	                sendto(listenfd, bufor, 1024 , 0 , &serwer_addres, serwer_len);
	                memset(bufor,0,1024);
	                fread(bufor, 1024,1,f);
	                licznik++;
	            }

	            //przeslanie ostatniego bloku danych, ktory zostal pominiety przy wykonywaniu petli
	            fread(bufor, (rozmiar % 1024),1,f);
	            sendto(listenfd, bufor, (rozmiar % 1024) , 0 , &serwer_addres, serwer_len);
	            memset(bufor,0,1024);

	            //zamkniecie pliku
	            fclose(f);

	            mvwprintw(menu, 7, 11, "Plik \"%s\" zostal wyslany", plik);
				wrefresh(menu);

	            getch();
	            werase(menu);
	   			box(menu,0,0);
				wrefresh(menu);

				werase(naglowek);
				box(naglowek,0,0);
				wrefresh(naglowek);
            }
        }

        //pobieranie
        else if(strcmp(typ[wybor], "Pobierz plik") == 0) {

        	wattron(naglowek, A_BOLD);
        	mvwprintw(naglowek, 2, 3, "--- POBIERANIE PLIKU ---");
        	wattroff(naglowek, A_BOLD);
    		wrefresh(naglowek);

            //przeslanie informacji o wybranej operacji
            sendto(listenfd, typ[wybor], 20 , 0 , &serwer_addres, serwer_len);
            memset(bufor,0,1024);

            //zmienna przechowujaca nazwe pliku
            char plik[20];
            mvwprintw(menu, 2, 11, "Wprowadz nazwe pliku: "); 
            mvwscanw(menu, 3, 11, "%s",&plik);

            //przeslanie nazwy pliku
            sendto(listenfd, plik, 20 , 0 , &serwer_addres, serwer_len);
            memset(bufor,0,1024);

			char sprawdz[20];
           	recvfrom(listenfd, bufor, 20, 0, &serwer_addres, &serwer_len);
           	strcpy(sprawdz, bufor);
			memset(bufor,0,1024);
			if(strcmp(sprawdz, "n") == 0){
				mvwprintw(menu, 6, 11, "Nie znaleziono pliku \"%s\"", plik);
            	wrefresh(menu);
            	getch();
            	werase(menu);
	   			box(menu, 0,0);
				wrefresh(menu);

				werase(naglowek);
				box(naglowek,0,0);
				wrefresh(naglowek);
			}
			else if(strcmp(sprawdz, "j") == 0){

				mvwprintw(menu, 6, 11, "Trwa pobieranie pliku \"%s\"",plik);
				wrefresh(menu);

				//pobranie rozmiaru pliku
	            recvfrom(listenfd, bufor, 20, 0, &serwer_addres, &serwer_len);
	            unsigned long rozmiar = atoi(bufor);

	            //otworzenie pliku w trybie do zapisu binarnego
	            FILE *f;
	            f=fopen(plik,"wb");
	            int licznik=1;
	            memset(bufor,0,1024);

	            //pobieranie pliku
	            while(licznik*1024<rozmiar)
	            {
	                recvfrom(listenfd, bufor, 1024, 0, &serwer_addres,&serwer_len);
	                fwrite(bufor,1024, 1, f);
	                memset(bufor,0,1024);
	                licznik++;
	            }

	            //pobranie ostatniego bloku danych, ktory zostal pominiety przy wykonywaniu petli
	            recvfrom(listenfd, bufor, (rozmiar%1024), 0, &serwer_addres, &serwer_len);
	            fwrite(bufor,(rozmiar%1024), 1, f);
	            memset(bufor,0,1024);

	            mvwprintw(menu, 7, 11, "Plik \"%s\" zostal pobrany", plik);
	            wrefresh(menu);

	            //zamkniecie pliku
	            fclose(f);

	            getch();
	            werase(menu);
	   			box(menu, 0,0);
				wrefresh(menu);

				werase(naglowek);
				box(naglowek,0,0);
				wrefresh(naglowek);
			}
        }
        else if(strcmp(typ[wybor], "Zakoncz program") == 0){
        	
        	wattron(naglowek, A_BOLD);
        	mvwprintw(naglowek, 2, 2, "--- ZAMYKANIE PROGRAMU ---");
        	wattroff(naglowek, A_BOLD);
    		wrefresh(naglowek);

        	mvwprintw(menu, 4, 15, "Koniec programu!");
        	wrefresh(menu);
			getch();

        	//zamkniecie socketu
    		close(listenfd);

    		//zwolenienie pamieci okna
    		endwin();
        	return 0;
        }
    }
    return 0;
}