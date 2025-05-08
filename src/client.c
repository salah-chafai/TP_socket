#include "shared.h"

void run_tcp_client() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("TCP socket creation failed");
        return;
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("TCP connection failed");
        return;
    }

    printf("TCP Client connected to middle man\n");

    char op_str[MAX_OP_SIZE];
    BinaryOperation op;

    while (1) {
        printf("Enter operation (+, -, *, /) or 'exit': ");
        scanf("%s", op_str);

        if (strcmp(op_str, "exit") == 0) break;

        op.operation = parse_operation(op_str);
        if (op.operation == INVALID_OP) {
            printf("Invalid operation\n");
            continue;
        }

        printf("Enter first operand: ");
        scanf("%d", &op.operand1);
        printf("Enter second operand: ");
        scanf("%d", &op.operand2);

        op.result = 0;
        op.error = false;
        op.crc = compute_crc(&op);

        send(sock, &op, sizeof(op), 0);
        recv(sock, &op, sizeof(op), 0);

        printf("Server response: ");
        print_operation(&op, false);
    }

    close(sock);
}

void run_udp_client() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP socket creation failed");
        return;
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };

    socklen_t addr_len = sizeof(serv_addr);
    char op_str[MAX_OP_SIZE];
    BinaryOperation op;

    while (1) {
        printf("Enter operation (+, -, *, /) or 'exit': ");
        scanf("%s", op_str);

        if (strcmp(op_str, "exit") == 0) break;

        op.operation = parse_operation(op_str);
        if (op.operation == INVALID_OP) {
            printf("Invalid operation\n");
            continue;
        }

        printf("Enter first operand: ");
        scanf("%d", &op.operand1);
        printf("Enter second operand: ");
        scanf("%d", &op.operand2);

        op.result = 0;
        op.error = false;
        op.crc = compute_crc(&op);

        sendto(sockfd, &op, sizeof(op), 0, (struct sockaddr*)&serv_addr, addr_len);
        recvfrom(sockfd, &op, sizeof(op), 0, NULL, NULL);

        printf("Server response: ");
        print_operation(&op, false);
    }

    close(sockfd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <tcp|udp>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "tcp") == 0) {
        run_tcp_client();
    } else if (strcmp(argv[1], "udp") == 0) {
        run_udp_client();
    } else {
        printf("Invalid protocol. Use 'tcp' or 'udp'.\n");
        return 1;
    }

    return 0;
}
