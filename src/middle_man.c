#include "shared.h"

void run_tcp_middle_man(int error_probability) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("TCP socket creation failed");
        return;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("TCP bind failed");
        return;
    }

    if (listen(server_fd, 5) < 0) {
        perror("TCP listen failed");
        return;
    }

    printf("TCP Middle Man running on port %d (error probability: %d%%)\n", PORT, error_probability);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("TCP accept failed");
            continue;
        }

        printf("TCP Client connected\n");

        BinaryOperation op;
        while (recv(client_fd, &op, sizeof(op), 0) > 0) {
            printf("Received from client: ");
            print_operation(&op, true);

            introduce_error(&op, error_probability);

            int server_fd = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in server_addr = {
                .sin_family = AF_INET,
                .sin_port = htons(PORT + 1),
                .sin_addr.s_addr = inet_addr("127.0.0.1")
            };

            if (connect(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                perror("Failed to connect to server");
                close(server_fd);
                continue;
            }

            send(server_fd, &op, sizeof(op), 0);
            recv(server_fd, &op, sizeof(op), 0);
            close(server_fd);

            send(client_fd, &op, sizeof(op), 0);
        }

        close(client_fd);
    }
}

void run_udp_middle_man(int error_probability) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP socket creation failed");
        return;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("UDP bind failed");
        return;
    }

    printf("UDP Middle Man running on port %d (error probability: %d%%)\n", PORT, error_probability);

    struct sockaddr_in client_addr, server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT + 1),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };

    socklen_t addr_len = sizeof(client_addr);
    BinaryOperation op;

    while (1) {
        if (recvfrom(sockfd, &op, sizeof(op), 0, (struct sockaddr*)&client_addr, &addr_len) != sizeof(op)) {
            perror("Failed to receive from client");
            continue;
        }

        printf("Received from client: ");
        print_operation(&op, true);

        introduce_error(&op, error_probability);

        sendto(sockfd, &op, sizeof(op), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        recvfrom(sockfd, &op, sizeof(op), 0, NULL, NULL);

        sendto(sockfd, &op, sizeof(op), 0, (struct sockaddr*)&client_addr, addr_len);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <tcp|udp> <error_probability>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));
    int error_probability = atoi(argv[2]);

    if (strcmp(argv[1], "tcp") == 0) {
        run_tcp_middle_man(error_probability);
    } else if (strcmp(argv[1], "udp") == 0) {
        run_udp_middle_man(error_probability);
    } else {
        printf("Invalid protocol. Use 'tcp' or 'udp'.\n");
        return 1;
    }

    return 0;
}
