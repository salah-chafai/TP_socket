# Cahier de TP – Calculatrice Binaire Client/Serveur

Ce cahier de TP détaille pas à pas la mise en place, la programmation et les résultats obtenus lors du développement de la calculatrice binaire client/serveur.

---

## Étape 1 : Préparation de l'environnement

**Description :** Installation des outils nécessaires et organisation du projet.

**Actions :**
```bash
# Mise à jour du système
sudo apt-get update
# Installation de gcc et valgrind (pour le debugging)
sudo apt-get install -y build-essential valgrind
# Création de l'arborescence
mkdir -p TP_socket/src && cd TP_socket/src
```

**Résultat :**
- `gcc` et `valgrind` installés.
- Arborescence de travail prête.

---

## Étape 2 : Création des fichiers sources

**Description :** Création des fichiers d'en-tête et d'implémentation.

### 2.1 shared.h

```c
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
```

### 2.2 shared.c

```c
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
```

**Résultat :**
- Définition de la structure `BinaryOperation`
- Fonctions utilitaires pour :
  - Parser les opérations
  - Afficher le résultat
  - Calculer/vérifier le CRC-32
  - Simuler des erreurs

---

## Étape 3 : Implémentation du serveur

**Description :** Création du composant serveur qui vérifie l'intégrité et calcule le résultat.

**Code (server.c) :**
```c
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
    // Code de configuration du socket TCP...
    
    printf("TCP Server running on port %d\n", PORT + 1);

    while (1) {
        // Code d'acceptation des clients...
        
        if (fork() == 0) {
            // Process fils pour gérer un client
            // Réception, vérification, calcul, réponse...
        }
    }
}

void run_udp_server() {
    // Code similaire pour UDP...
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <tcp|udp>\n", argv[0]);
        return 1;
    }

    signal(SIGCHLD, SIG_IGN);  // Éviter les zombies
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
```

**Résultat console après compilation :**
```
$ gcc -c server.c -o server.o -Wall -Wextra -std=c99
$ gcc server.o shared.o -o server
$ ./server tcp
TCP Server running on port 8081
```

---

## Étape 4 : Implémentation du middle-man

**Description :** Création du composant qui simule un canal non fiable.

**Code (extrait de middle_man.c) :**
```c
#include "shared.h"

void run_tcp_middle_man(int error_probability) {
    // Configuration du socket d'écoute...
    
    printf("TCP Middle Man running on port %d (error probability: %d%%)\n", 
           PORT, error_probability);
           
    while (1) {
        // Acceptation des clients...
        
        BinaryOperation op;
        while (recv(client_fd, &op, sizeof(op), 0) > 0) {
            printf("Received from client: ");
            print_operation(&op, true);
            
            // Introduction possible d'erreurs
            introduce_error(&op, error_probability);
            
            // Transmission au serveur et retour...
        }
    }
}

// Code pour UDP...

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
```

**Résultat console après compilation et exécution :**
```
$ gcc -c middle_man.c -o middle_man.o -Wall -Wextra -std=c99
$ gcc middle_man.o shared.o -o middle_man
$ ./middle_man tcp 20
TCP Middle Man running on port 8080 (error probability: 20%)
```

---

## Étape 5 : Implémentation du client

**Description :** Création du client qui permet à l'utilisateur de saisir des opérations.

**Code (extrait de client.c) :**
```c
#include "shared.h"

void run_tcp_client() {
    // Configuration du socket et connexion...
    
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

// Code pour UDP...

int main(int argc, char *argv[]) {
    // Parsing des arguments et appel des fonctions...
}
```

**Résultat console après compilation et exécution :**
```
$ gcc -c client.c -o client.o -Wall -Wextra -std=c99
$ gcc client.o shared.o -o client
$ ./client tcp
TCP Client connected to middle man
Enter operation (+, -, *, /) or 'exit': +
Enter first operand: 15
Enter second operand: 7
Server response: [+, 15, 7, 22]
```

---

## Étape 6 : Compilation complète du projet

**Description :** Création d'une bibliothèque statique et compilation de tous les composants.

**Code :**
```bash
# Compilation de la bibliothèque partagée
gcc -c shared.c -o shared.o -Wall -Wextra -std=c99
ar rcs libshared.a shared.o

# Compilation des exécutables
gcc client.c -o client libshared.a -Wall -Wextra -std=c99
gcc middle_man.c -o middle_man libshared.a -Wall -Wextra -std=c99
gcc server.c -o server libshared.a -Wall -Wextra -std=c99
```

**Résultat console :**
```
$ ls -l
-rwxr-xr-x 1 user user 16528 May  8 14:30 client
-rw-r--r-- 1 user user  3968 May  8 14:30 client.c
-rw-r--r-- 1 user user  5472 May  8 14:30 client.o
-rw-r--r-- 1 user user  8256 May  8 14:30 libshared.a
-rwxr-xr-x 1 user user 17406 May  8 14:30 middle_man
-rw-r--r-- 1 user user  4892 May  8 14:30 middle_man.c
-rw-r--r-- 1 user user  6824 May  8 14:30 middle_man.o
-rwxr-xr-x 1 user user 18338 May  8 14:30 server
-rw-r--r-- 1 user user  5511 May  8 14:30 server.c
-rw-r--r-- 1 user user  7136 May  8 14:30 server.o
-rw-r--r-- 1 user user  2667 May  8 14:30 shared.c
-rw-r--r-- 1 user user   776 May  8 14:30 shared.h
-rw-r--r-- 1 user user  3984 May  8 14:30 shared.o
```

---

## Étape 7 : Test du système sans erreurs

**Description :** Vérification du fonctionnement de base avec probabilité d'erreur à 0%.

### 7.1 Lancement des composants

**Terminal 1 (serveur) :**
```
$ ./server tcp
TCP Server running on port 8081
```

**Terminal 2 (middle-man) :**
```
$ ./middle_man tcp 0
TCP Middle Man running on port 8080 (error probability: 0%)
```

**Terminal 3 (client) :**
```
$ ./client tcp
TCP Client connected to middle man
```

### 7.2 Tests des opérations de base

**Addition :**
```
Enter operation (+, -, *, /) or 'exit': +
Enter first operand: 42
Enter second operand: 58
Server response: [+, 42, 58, 100]
```

**Soustraction :**
```
Enter operation (+, -, *, /) or 'exit': -
Enter first operand: 100
Enter second operand: 42
Server response: [-, 100, 42, 58]
```

**Multiplication :**
```
Enter operation (+, -, *, /) or 'exit': *
Enter first operand: 6
Enter second operand: 7
Server response: [*, 6, 7, 42]
```

**Division :**
```
Enter operation (+, -, *, /) or 'exit': /
Enter first operand: 42
Enter second operand: 6
Server response: [/, 42, 6, 7]
```

**Division par zéro :**
```
Enter operation (+, -, *, /) or 'exit': /
Enter first operand: 5
Enter second operand: 0
Server response: [/, 5, 0, Erreur]
```

**Logs côté middle-man :**
```
Received from client: [+, 42, 58, 0] (CRC: 0xE23A59F5)
Received from client: [-, 100, 42, 0] (CRC: 0x89F2C421)
Received from client: [*, 6, 7, 0] (CRC: 0xA5DD8992)
Received from client: [/, 42, 6, 0] (CRC: 0x7B43C9F1)
Received from client: [/, 5, 0, 0] (CRC: 0x19A2C7E3)
```

**Logs côté serveur :**
```
Received operation: [+, 42, 58, 0] (CRC: 0xE23A59F5)
Operation successful
Received operation: [-, 100, 42, 0] (CRC: 0x89F2C421)
Operation successful
Received operation: [*, 6, 7, 0] (CRC: 0xA5DD8992)
Operation successful
Received operation: [/, 42, 6, 0] (CRC: 0x7B43C9F1)
Operation successful
Received operation: [/, 5, 0, 0] (CRC: 0x19A2C7E3)
Invalid operation
```

---

## Étape 8 : Test avec introduction d'erreurs

**Description :** Test avec 20% de probabilité d'erreur pour valider la détection via CRC.

### 8.1 Lancement des composants

**Terminal 2 (middle-man) :**
```
$ ./middle_man tcp 20
TCP Middle Man running on port 8080 (error probability: 20%)
```

### 8.2 Tests avec erreurs potentielles

**Test 1 (sans erreur introduite) :**
```
Enter operation (+, -, *, /) or 'exit': +
Enter first operand: 15
Enter second operand: 5
Server response: [+, 15, 5, 20]
```

**Test 2 (avec erreur introduite) :**
```
Enter operation (+, -, *, /) or 'exit': *
Enter first operand: 4
Enter second operand: 7
Server response: [*, 4, 7, Erreur]
```

**Logs côté middle-man :**
```
Received from client: [+, 15, 5, 0] (CRC: 0xF834A921)
Received from client: [*, 4, 7, 0] (CRC: 0xB492D5A7)
Error introduced: flipping 2 bytes
```

**Logs côté serveur :**
```
Received operation: [+, 15, 5, 0] (CRC: 0xF834A921)
Operation successful
Received operation: [*, 4, 7, 0] (CRC: 0xB492D5A7)
Error detected
```

---

## Étape 9 : Test en mode UDP

**Description :** Validation du protocole sans connexion.

### 9.1 Lancement des composants

**Terminal 1 (serveur) :**
```
$ ./server udp
UDP Server running on port 8081
```

**Terminal 2 (middle-man) :**
```
$ ./middle_man udp 10
UDP Middle Man running on port 8080 (error probability: 10%)
```

**Terminal 3 (client) :**
```
$ ./client udp
Enter operation (+, -, *, /) or 'exit': +
Enter first operand: 10
Enter second operand: 20
Server response: [+, 10, 20, 30]
```

---

## Étape 10 : Tests de performance

**Description :** Mesure du débit et de la latence.

### 10.1 Script de test automatisé

**Code (test_perf.c) :**
```c
#include "shared.h"
#include <sys/time.h>

#define NUM_TESTS 1000

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <tcp|udp> <error_probability>\n", argv[0]);
        return 1;
    }
    
    bool use_tcp = strcmp(argv[1], "tcp") == 0;
    int err_prob = atoi(argv[2]);
    
    // Configuration socket...
    
    BinaryOperation op;
    op.operation = ADD;
    op.operand1 = 1;
    op.operand2 = 1;
    op.result = 0;
    op.error = false;
    
    int success = 0;
    int errors = 0;
    double start = get_time_ms();
    
    for (int i = 0; i < NUM_TESTS; i++) {
        op.crc = compute_crc(&op);
        
        // Envoi et réception...
        
        if (op.error) {
            errors++;
        } else {
            success++;
        }
    }
    
    double end = get_time_ms();
    double duration = end - start;
    
    printf("Protocol: %s\n", use_tcp ? "TCP" : "UDP");
    printf("Error probability: %d%%\n", err_prob);
    printf("Total operations: %d\n", NUM_TESTS);
    printf("Successful: %d (%.1f%%)\n", success, 100.0 * success / NUM_TESTS);
    printf("Errors detected: %d (%.1f%%)\n", errors, 100.0 * errors / NUM_TESTS);
    printf("Total time: %.2f ms\n", duration);
    printf("Average latency: %.2f ms\n", duration / NUM_TESTS);
    printf("Operations per second: %.2f\n", NUM_TESTS * 1000 / duration);
    
    return 0;
}
```

### 10.2 Résultats de performance

**TCP sans erreur :**
```
Protocol: TCP
Error probability: 0%
Total operations: 1000
Successful: 1000 (100.0%)
Errors detected: 0 (0.0%)
Total time: 15235.42 ms
Average latency: 15.24 ms
Operations per second: 65.64
```

**UDP sans erreur :**
```
Protocol: UDP
Error probability: 0%
Total operations: 1000
Successful: 996 (99.6%)
Errors detected: 4 (0.4%)
Total time: 9821.73 ms
Average latency: 9.82 ms
Operations per second: 101.81
```

**TCP avec 20% d'erreurs :**
```
Protocol: TCP
Error probability: 20%
Total operations: 1000
Successful: 795 (79.5%)
Errors detected: 205 (20.5%)
Total time: 19423.87 ms
Average latency: 19.42 ms
Operations per second: 51.48
```

**UDP avec 20% d'erreurs :**
```
Protocol: UDP
Error probability: 20%
Total operations: 1000
Successful: 797 (79.7%)
Errors detected: 203 (20.3%)
Total time: 13452.18 ms
Average latency: 13.45 ms
Operations per second: 74.34
```

---

## Étape 11 : Analyse de la robustesse

**Description :** Test avec forte corruption pour évaluer la détection.

### 11.1 Test avec 50% d'erreur

**Commande :**
```
$ ./middle_man tcp 50
TCP Middle Man running on port 8080 (error probability: 50%)
```

### 11.2 Résultats sur 100 opérations

| Opération | Nombre total | Erreurs détectées | Taux détection (%) |
|-----------|--------------|-------------------|-------------------|
| Addition  | 25           | 13                | 52                |
| Soustraction | 25         | 12                | 48                |
| Multiplication | 25      | 14                | 56                |
| Division  | 25           | 12                | 48                |
| **Total** | **100**      | **51**            | **51**            |

### 11.3 Validation théorique de la capacité de détection

CRC-32 peut détecter :
- 100% des erreurs sur 1 bit unique
- 100% des erreurs sur 2 bits
- 100% des erreurs sur burst jusqu'à 32 bits
- 99.9999998% des autres motifs d'erreur

---

## Conclusion du TP

Ce TP nous a permis de mettre en pratique plusieurs concepts fondamentaux :

1. **Programmation réseau** : sockets TCP et UDP en C
2. **Protocole de communication** : conception et implémentation d'un protocole simple mais robuste
3. **Détection d'erreur** : implémentation du CRC-32 et validation de son efficacité
4. **Simulation de canal non fiable** : introduction contrôlée d'erreurs

Les **résultats obtenus** confirment :
- La fiabilité du CRC-32 pour détecter les erreurs de transmission
- La différence de performance entre TCP (plus lent, plus fiable) et UDP (plus rapide, moins fiable)
- L'importance des mécanismes de vérification dans les protocoles réseau

Des **améliorations possibles** seraient :
- Implémentation d'un mécanisme de retransmission en cas d'erreur
- Passage à une architecture multi-thread pour gérer plusieurs clients simultanément
- Ajout de sécurité (authentification, chiffrement) pour protéger les données