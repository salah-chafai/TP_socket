#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <time.h>

#define PORT 8080
#define MAX_OP_SIZE 10

typedef enum {
    ADD, SUB, MUL, DIV, INVALID_OP
} Operation;

typedef struct {
    Operation operation;
    int operand1;
    int operand2;
    int result;
    bool error;
    uint32_t crc;
} BinaryOperation;

Operation parse_operation(const char* op_str);
void print_operation(const BinaryOperation* op, const bool show_crc);
void introduce_error(BinaryOperation* op, int probability);
uint32_t compute_crc(const BinaryOperation* op);

#endif
