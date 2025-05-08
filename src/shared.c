#include "shared.h"
#include <time.h>

Operation parse_operation(const char* op_str) {
    if (strcmp(op_str, "+") == 0) return ADD;
    if (strcmp(op_str, "-") == 0) return SUB;
    if (strcmp(op_str, "*") == 0) return MUL;
    if (strcmp(op_str, "/") == 0) return DIV;
    return INVALID_OP;
}

void print_operation(const BinaryOperation* op, bool show_crc) {
    const char* op_str;
    switch(op->operation) {
        case ADD: op_str = "+"; break;
        case SUB: op_str = "-"; break;
        case MUL: op_str = "*"; break;
        case DIV: op_str = "/"; break;
        default: op_str = "?"; break;
    }
    if (op->error) printf("[%s, %d, %d, Erreur]", op_str, op->operand1, op->operand2);
    else printf("[%s, %d, %d, %d]", op_str, op->operand1, op->operand2, op->result);
    if (show_crc) printf(" (CRC: 0x%08X)", op->crc);
    puts("");
}

void introduce_error(BinaryOperation* op, int probability) {
    if (rand() % 100 >= probability) return;
    
    uint8_t* data = (uint8_t*)op;
    size_t length = sizeof(BinaryOperation) - sizeof(uint32_t);
    int bytes_to_flip = 1 + rand() % 3;
    
    for (int i = 0; i < bytes_to_flip; i++) {
        int byte_pos = rand() % length;
        data[byte_pos] ^= 0xFF;
    }
}

uint32_t compute_crc(const BinaryOperation* op) {
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* data = (const uint8_t*)op;
    size_t length = sizeof(BinaryOperation) - sizeof(uint32_t);
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}
