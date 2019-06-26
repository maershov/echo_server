#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>

#include <stdio.h>
#include <fcntl.h>
#include <algorithm>
#include <set>

using namespace std;

int main()
{
    int listener;
    struct sockaddr_in addr;
    char buffer[1024];
    int byte_reader;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener < 0)
    {
        perror("my socket");
        exit(1);
    }

    fcntl(listener, F_SETFL, O_NONBLOCK);

    addr.sin_family = AF_INET;
    //устанавливаем порт, на котором будет работать наш "чат" с бродкаст запросом
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(2);
    }

    listen(listener, 2);

    set<int> my_clients;
    my_clients.clear();

    while(1)
    {
        // Заполняем сокеты
        fd_set my_set_read;
        FD_ZERO(&my_set_read);
        FD_SET(listener, &my_set_read);

        for(set<int>::iterator it = my_clients.begin(); it != my_clients.end(); it++)
            FD_SET(*it, &my_set_read);

        // Ставим таймаут сколько висит соединение
        timeval timeout;
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        // Событие в сокете которое ожидаем
        int mx = max(listener, *max_element(my_clients.begin(), my_clients.end()));
        if(select(mx+1, &my_set_read, NULL, NULL, &timeout) <= 0)
        {
            perror("select");
            exit(3);
        }

        //Выполняем действие
        if(FD_ISSET(listener, &my_set_read))
        {
            // Запрос на соединение
            int sock = accept(listener, NULL, NULL);
            if(sock < 0)
            {
                perror("accept");
                exit(3);
            }

            fcntl(sock, F_SETFL, O_NONBLOCK);

            my_clients.insert(sock);
        }

        for(set<int>::iterator it = my_clients.begin(); it != my_clients.end(); it++)
        {
            if(FD_ISSET(*it, &my_set_read))
            {
                // Берем данные и читаем их
                byte_reader = recv(*it, buffer, 1024, 0);

                if(byte_reader <= 0)
                {
                    // Удаляем из сета когда соединение разорвано
                    close(*it);
                    my_clients.erase(*it);
                    continue;
                }

                // Отправим данные всем клиентам
                for(set<int>::iterator it = my_clients.begin(); it != my_clients.end(); it++){
                    send(*it, buffer, byte_reader, 0);

                }
            }
        }
    }

    return 0;
}
