#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <limits>

#define PORT 1053
#define NUM_CLIENTS 5

uint32_t totalNumbers = 0;
uint32_t primeNumbers = 0;
uint32_t minNumber = std::numeric_limits<uint32_t>::max();
uint32_t maxNumber = 0;

pthread_mutex_t fileLock;
pthread_mutex_t valueLock;

void sigpipe_handler(int signal) {
    // Do nothing
}

bool isPrime(uint32_t n) {
    if (n <= 1)
        return false;
    if (n <= 3)
        return true;

    if (n % 2 == 0 || n % 3 == 0)
        return false;

    for (uint32_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    }

    return true;
}

// Функція для запису в системний журнал
void logToServer(const char *message) {
    time_t now = time(0);
    struct tm *timeinfo = localtime(&now);
    char timestamp[100];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %X", timeinfo);

    pthread_mutex_lock(&fileLock);
    FILE *logFile = fopen("server_log.txt", "a");
    if (logFile) {
        fprintf(logFile, "[%s] %s\n", timestamp, message);
        fclose(logFile);
    }
    pthread_mutex_unlock(&fileLock);
}

void *manageClientPrimeSearch(void *param) {
    printf("Client connected, waiting for numbers or commands...\n");
    uint64_t new_socket;
    new_socket = (long) param;

    uint32_t return_status;

    while (1) {
        uint32_t request;
        if (recv(new_socket, &request, sizeof(request), 0) == -1) {
            printf("Problem receiving request from client, closing connection\n");
            close(new_socket);
            break;
        }

        request = ntohl(request);

        if (request == 0) {
            // Завершити з'єднання, якщо клієнт відправив 0
            printf("Client has finished sending numbers. Closing connection...\n");

            // Отправка зведених даних клієнту
            uint32_t summaryData[4] = {
                    htonl(totalNumbers),
                    htonl(primeNumbers),
                    htonl(minNumber),
                    htonl(maxNumber)
            };
            write(new_socket, summaryData, sizeof(summaryData));

            close(new_socket);
            break; // Вихід із циклу
        } else if (request == 1) {
            // Обробка команди "Who"
            const char *authorInfo = "Author: Myroslava Tsvietkova, Variant: 28, Prime numbers filter";
            write(new_socket, authorInfo, strlen(authorInfo) + 1);
            logToServer("Client requested 'Who' information");
        } else {
            // Оновити зведені дані
            pthread_mutex_lock(&valueLock);
            totalNumbers++;
            if (isPrime(request)) {
                primeNumbers++;
            }
            if (request < minNumber) {
                minNumber = request;
            }
            if (request > maxNumber) {
                maxNumber = request;
            }
            pthread_mutex_unlock(&valueLock);

            // Відправити результат клієнту
            uint32_t primeToSend = isPrime(request) ? 1 : 0;
            uint32_t convertedResult = htonl(primeToSend);
            return_status = write(new_socket, &convertedResult, sizeof(convertedResult));

            if (return_status == -1) {
                printf("Client not responding, closing connection\n");
                close(new_socket);
                break; // Вихід із циклу
            }

            char logMessage[100];
            snprintf(logMessage, sizeof(logMessage), "Received number: %u, Prime: %d", request, primeToSend);
            logToServer(logMessage);
        }
    }
}

int main(int argc, char const *argv[]) {
    printf("\nWelcome to the Server Program!\n");

    pthread_t tid_client[NUM_CLIENTS];
    pthread_mutex_init(&fileLock, nullptr);
    pthread_mutex_init(&valueLock, nullptr);
    signal(SIGPIPE, &sigpipe_handler);
    int server_fd;
    long new_socket;
    struct sockaddr_in address{};
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(PORT);

    if (::bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int thread_index = 0;

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("New client connected.\n");
        pthread_create(&tid_client[thread_index], nullptr, manageClientPrimeSearch, (void *) new_socket);
        thread_index = (thread_index + 1) % NUM_CLIENTS;
    }

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        pthread_join(tid_client[i], nullptr);
    }

    return 0;
}
