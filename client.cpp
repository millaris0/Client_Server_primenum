#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdint>
#include <random>
#include <vector>
#include <ctime>
#include <fstream>

#define PORT 1053
#define NUMBERS_TO_GENERATE 50
const uint32_t END_OF_INPUT = 0;

bool isPrime(uint32_t n);

// Функція для запису в системний журнал клієнта
void logToClient(const char *message) {
    time_t now = time(0);
    struct tm *timeinfo = localtime(&now);
    char timestamp[100];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %X", timeinfo);

    std::ofstream logFile("client_log.txt", std::ios::app);
    if (logFile.is_open()) {
        logFile << "[" << timestamp << "] " << message << std::endl;
        logFile.close();
    }
}

int main(int argc, char const *argv[]) {
    printf("\nWelcome to the Client Program!\n");

    std::default_random_engine randomGenerator(static_cast<unsigned>(time(nullptr)));
    std::uniform_int_distribution<uint32_t> distribution(2, UINT32_MAX);

    std::vector<uint32_t> randomNumbers;
    for (int i = 0; i < NUMBERS_TO_GENERATE; ++i) {
        randomNumbers.push_back(distribution(randomGenerator));
        std::cout << randomNumbers[i] << std::endl;
    }

    std::ofstream primeFile("prime_numbers.txt");

    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }

    printf("Connected to server.\n");
    uint64_t return_status;

    uint32_t whoRequest = htonl(1);
    return_status = write(sock, &whoRequest, sizeof(whoRequest));

    if (return_status < 0) {
        printf("Problem sending 'Who' request to the server, exiting\n");
        close(sock);
        return -1;
    }

    char authorInfo[256];
    if (read(sock, authorInfo, sizeof(authorInfo)) > 0) {
        printf("Server responded to 'Who' request: %s\n", authorInfo);
        logToClient("Received 'Who' response from server");
    } else {
        printf("No response from the server to 'Who' request.\n");
        logToClient("No response from server to 'Who' request");
    }

    for (uint32_t testNumber : randomNumbers) {
        uint32_t convertedNumber = htonl(testNumber);
        return_status = write(sock, &convertedNumber, sizeof(convertedNumber));

        if (return_status < 0) {
            printf("Problem sending number to the server, exiting\n");
            close(sock);
            return -1;
        }

        uint32_t result;
        if (read(sock, &result, sizeof(result)) < sizeof(result)) {
            printf("Problem receiving result from server, exiting\n");
            close(sock);
            return -1;
        }

        if (ntohl(result) == 1) {
            printf("Server found prime %u\n", testNumber);
            primeFile << testNumber << std::endl;
            std::string logMessage = "Server found prime number: " + std::to_string(testNumber);
            logToClient(logMessage.c_str());
        }

    }

    uint32_t endMarker = htonl(END_OF_INPUT);
    return_status = write(sock, &endMarker, sizeof(endMarker));

    uint32_t receivedData[4];
    if (read(sock, receivedData, sizeof(receivedData)) == sizeof(receivedData)) {
        uint32_t totalNumbers = ntohl(receivedData[0]);
        uint32_t primeNumbers = ntohl(receivedData[1]);
        uint32_t minNumber = ntohl(receivedData[2]);
        uint32_t maxNumber = ntohl(receivedData[3]);

        printf("Total numbers processed: %u\n", totalNumbers);
        printf("Total prime numbers found: %u\n", primeNumbers);
        printf("Minimum number: %u\n", minNumber);
        printf("Maximum number: %u\n", maxNumber);
        logToClient("Received summary data from server");
    } else {
        printf("Error receiving summary data from the server.\n");
        logToClient("Error receiving summary data from server");
    }

    close(sock);
    primeFile.close();

    return 0;
}
