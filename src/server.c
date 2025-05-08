#include "shared.h"
#include <sys/wait.h>
#include <signal.h>

int verify_operation(BinaryOperation* op) {
    uint32_t received_crc = op->crc;
    op->crc = 0;
    op->error = false;
    uint32_t computed_crc = compute_crc(op);

    if (received_crc != computed_crc) {
        op->error = true;
        return 1;
    }
    
    switch (op->operation) {
        case ADD: op->result = op->operand1 + op->operand2; break;
        case SUB: op->result = op->operand1 - op->operand2; break;
        case MUL: op->result = op->operand1 * op->operand2; break;
        case DIV:
            if (op->operand2 == 0) return -1;
            op->result = op->operand1 / op->operand2;
            break;
        default: return -1;
    }
    
    return 0;
}

void run_tcp_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT + 1)
    };

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("TCP bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("TCP listen failed");
        exit(EXIT_FAILURE);
    }

    printf("TCP Server running on port %d\n", PORT + 1);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("TCP accept failed");
            continue;
        }

        if (fork() == 0) {
            close(server_fd);
            BinaryOperation op;

            while (recv(client_fd, &op, sizeof(op), 0) > 0) {
                printf("Received operation: ");
                print_operation(&op, true);

                int verification = verify_operation(&op);
                if (verification == -1) {
                    op.error = true;
                    printf("Invalid operation\n");
                } else if (verification == 1) {
                    op.error = true;
                    printf("Error detected\n");
                } else {
                    op.error = false;
                    printf("Operation op->n");
                }

                send(client_fd, &op, sizeof(op), 0);
            }

            close(client_fd);
            exit(0);
        }

        close(client_fd);
    }
}

void run_udp_server() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT + 1)
    };

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }

    printf("UDP Server running on port %d\n", PORT + 1);

    BinaryOperation op;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (1) {
        if (recvfrom(sockfd, &op, sizeof(op), 0, (struct sockaddr*)&client_addr, &addr_len) != sizeof(op)) {
            perror("Failed to receive operation");
            continue;
        }

        printf("Received operation: ");
        print_operation(&op, true);

        int verification = verify_operation(&op);
        if (verification == -1) {
            op.error = true;
            printf("Invalid operation\n");
        } else if (verification == 1) {
            op.error = true;
            printf("Error detected\n");
        } else {
            op.error = false;
            printf("Operation op->n");
        }

        sendto(sockfd, &op, sizeof(op), 0, (struct sockaddr*)&client_addr, addr_len);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <tcp|udp>\n", argv[0]);
        return 1;
    }

    signal(SIGCHLD, SIG_IGN);
    srand(time(NULL));

    if (strcmp(argv[1], "tcp") == 0) {
        run_tcp_server();
    } else if (strcmp(argv[1], "udp") == 0) {
        run_udp_server();
    } else {
        printf("Invalid protocol. Use 'tcp' or 'udp'.\n");
        return 1;
    }

    return 0;
}
