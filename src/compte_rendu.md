# TP Téléinformatique RT2 – Calculatrice Binaire Client/Serveur

**Auteurs** : Salah Chafai, Mohamed Hedi Graba, Adem Kefi  
**Encadrant** : Kamel Karoui  
**Date** : Mai 2025

---

## 1. Contexte et objectifs

Dans le cadre du module de Téléinformatique, il est essentiel de comprendre la mise en place d'une chaîne de communication client/serveur, ainsi que les mécanismes de détection d'erreurs. Ce TP consiste à développer une **calculatrice binaire** distribuée, en s'appuyant sur des **sockets** (TCP et UDP) et un **simulateur de réseau non fiable**.

Les principaux objectifs sont :  
- Appréhender les fondamentaux des sockets en C (création, liaison, écoute, connexion).  
- Mettre en œuvre un protocole de contrôle d'intégrité (CRC-32).  
- Simuler la corruption de données sur le réseau et la gérer du côté serveur.  
- Concevoir une architecture modulaire garantissant la maintenabilité et l'évolution du code.

---

## 3. Architecture logicielle

L'application est organisée en quatre composants principaux :

1. **shared** (libshared.a + headers)  
   - `shared.h` : déclaration de la structure `BinaryOperation` et des prototypes.  
   - `shared.c` : implémentation des fonctions utilitaires :
     - `parse_operation()` : conversion de la saisie utilisateur en champs d'opération.  
     - `compute_crc()` : calcul du CRC-32 refléchi (polynôme 0xEDB88320).  
     - `introduce_error()` : simulation d'erreurs bit-à-bit sur la structure.  
     - `print_operation()` : affichage formaté de la structure.

2. **client.c**  
   - Saisie interactive : choix de l'opérateur (`+`, `-`, `*`, `/`) et des opérandes (entiers positifs).  
   - Remplissage de la structure `BinaryOperation`, initialisation du champ `crc` à `0`.  
   - Calcul du CRC et envoi au **middle-man** via TCP ou UDP (mode choisi en argument).  
   - Réception du paquet réponse et affichage de la valeur calculée ou du message d'erreur.

3. **middle_man.c**  
   - Serveur relais : écoute des connexions/sockets sur le **port 8080**.  
   - Application de `introduce_error()` avec un pourcentage de probabilité paramétrable.  
   - Transfert du paquet (éventuellement corrompu) vers le **serveur final** sur le **port 8081**.  
   - Réception de la réponse du serveur, puis renvoi au client d'origine.

4. **server.c**  
   - Réception du paquet : extraction du champ `crc` et remise à zéro pour recalculer le CRC local.  
   - Si le CRC reçu ne correspond pas au CRC calculé → `op.error = true`.  
   - En cas d'intégrité validée : réalisation de l'opération binaire et gestion des cas particuliers (division par zéro).  
   - Mise à jour du champ `result` ou du flag `error`, recalcul du CRC, puis renvoi de la structure au middle-man.

---

## 4. Spécification du protocole de communication

### 4.1 Format du message

Toute trame envoyée est une structure fixe en mémoire :

```c
typedef struct {
    char   op;         // '+' '-' '*' '/'
    uint32_t operand1; // opérande 1
    uint32_t operand2; // opérande 2
    uint32_t result;   // champ résultat ou 0
    uint32_t crc;      // checksum CRC-32
    bool   error;      // flag d'erreur (true = paquet corrompu ou division par zéro)
} BinaryOperation;
```

### 4.2 Séquence d'échange

```sequence
Client->MiddleMan: S (avec crc)
MiddleMan->Server: S' (avec erreur possible)
Server->MiddleMan: R (op.result ou error)
MiddleMan->Client: R
Client: affichage du résultat / message d'erreur
```

### 4.3 Mode TCP vs UDP

- **TCP** : connexion fiable, retransmission automatique, contrôle de flux.  
- **UDP** : mode datagramme, plus léger, pas de retransmission ni d'ordre garanti.  

Le choix s'insère au moment de la création du socket :

```c
int sock = socket(AF_INET, (use_udp) ? SOCK_DGRAM : SOCK_STREAM, 0);
```

---

## 5. Contrôle d'intégrité (CRC-32)

1. **Initialisation** : registre CRC mis à `0xFFFFFFFF`.  
2. **Traitement** : pour chaque octet du buffer (hors champ `crc`), on applique le polynôme réfléchi 0xEDB88320.  
3. **Finalisation** : inversement du registre (`crc = ~crc`).  
4. **Vérification** : comparaison entre CRC reçu et CRC recalculé.

Ce mécanisme détecte 100% des erreurs d'un seul bit et la majorité des erreurs multiples.

---

## 6. Simulation de réseau non fiable

La fonction `introduce_error(BinaryOperation *op, int p)` procède comme suit :

- Génération d'un nombre aléatoire pourcentage `r` entre 0 et 99.  
- Si `r < p` :
  - Choix aléatoire de 1 à 3 octets dans la structure.  
  - Flip bit-à-bit (`op_bytes[i] ^= 0xFF`).

Cette technique permet de valider la robustesse de la détection CRC.

---


## 9. Exemples de code détaillés

Cette section présente une analyse approfondie des principales fonctions du projet, avec des explications détaillées de leur fonctionnement.

### 9.1 Bibliothèque partagée

#### 9.1.1 Structure BinaryOperation

La structure centrale du projet est définie dans `shared.h` :

```c
typedef struct {
    Operation operation;  // Énumération (ADD, SUB, MUL, DIV)
    int operand1;         // Premier opérande
    int operand2;         // Second opérande
    int result;           // Résultat calculé par le serveur
    bool error;           // Drapeau d'erreur
    uint32_t crc;         // Somme de contrôle CRC-32
} BinaryOperation;
```

Cette structure encapsule tous les éléments nécessaires à la transmission d'une opération et de son résultat.

#### 9.1.2 Parse Operation

```c
Operation parse_operation(const char* op_str) {
    if (strcmp(op_str, "+") == 0) return ADD;
    if (strcmp(op_str, "-") == 0) return SUB;
    if (strcmp(op_str, "*") == 0) return MUL;
    if (strcmp(op_str, "/") == 0) return DIV;
    return INVALID_OP;
}
```

Cette fonction convertit une chaîne de caractère en valeur énumérée correspondant à l'opération mathématique. Elle est utilisée par le client pour interpréter la saisie utilisateur.

#### 9.1.3 Compute CRC (CRC-32)

```c
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

Analyse détaillée :
- Initialisation du registre CRC à 0xFFFFFFFF (tous les bits à 1)
- Parcours octet par octet (`uint8_t*`) de la structure `op` en excluant le champ CRC lui-même
- Pour chaque octet:
  - XOR avec le registre courant
  - Traitement bit à bit (8 itérations):
    - Si bit LSB = 1: XOR avec le polynôme 0xEDB88320 (version réfléchie du polynôme standard)
    - Décalage à droite d'un bit
- Finalisation: complément à 1 (~crc) pour obtenir la valeur finale

L'algorithme implémente le CRC-32 IEEE 802.3 standard, largement utilisé dans les protocoles réseau.

#### 9.1.4 Introduce Error

```c
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
```

Fonction clé du simulateur de canal bruité :
- Tirage aléatoire pour déterminer si une erreur doit être introduite selon la probabilité fournie
- Si erreur: sélection de 1 à 3 octets aléatoires dans la structure
- Pour chaque octet sélectionné: application d'un XOR avec 0xFF (inverse tous les bits)

Cette approche crée des erreurs significatives qui devraient être détectées par le CRC.

#### 9.1.5 Print Operation

```c
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
```

Fonction d'affichage formaté avec:
- Conversion de l'énumération `operation` en symbole mathématique
- Affichage conditionnel selon la présence d'une erreur
- Option pour afficher ou masquer la valeur CRC (format hexadécimal)

### 9.2 Module Client

#### 9.2.1 Run TCP Client

```c
void run_tcp_client() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    // Configuration socket...
    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("TCP connection failed");
        return;
    }
    
    // Boucle principale...
    while (1) {
        printf("Enter operation (+, -, *, /) or 'exit': ");
        scanf("%s", op_str);
        
        if (strcmp(op_str, "exit") == 0) break;
        
        // Remplissage structure...
        op.operation = parse_operation(op_str);
        printf("Enter first operand: ");
        scanf("%d", &op.operand1);
        printf("Enter second operand: ");
        scanf("%d", &op.operand2);
        
        // Préparation envoi...
        op.result = 0;
        op.error = false;
        op.crc = compute_crc(&op);
        
        // Envoi/réception...
        send(sock, &op, sizeof(op), 0);
        recv(sock, &op, sizeof(op), 0);
        
        printf("Server response: ");
        print_operation(&op, false);
    }
    
    close(sock);
}
```

Points importants:
- Connexion persistante (TCP) vers le middle-man
- Interface interactive avec l'utilisateur (saisie opération et opérandes)
- Calcul du CRC avant envoi
- Communication synchrone: attente de la réponse après chaque envoi

#### 9.2.2 Run UDP Client (similaire mais avec les spécificités UDP)

```c
void run_udp_client() {
    // Création socket...
    
    // Boucle principale similaire
    // ...
    
    // Principales différences:
    sendto(sockfd, &op, sizeof(op), 0, (struct sockaddr*)&serv_addr, addr_len);
    recvfrom(sockfd, &op, sizeof(op), 0, NULL, NULL);
    
    // ...
}
```

Différences clés avec TCP:
- Pas de connexion établie (`connect`)
- Utilisation de `sendto`/`recvfrom` avec adressage explicite
- Le socket reste unique pour toutes les communications

### 9.3 Module Middle-Man

#### 9.3.1 Run TCP Middle-Man

```c
void run_tcp_middle_man(int error_probability) {
    // Configuration socket serveur...
    
    // Boucle d'acceptation...
    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        printf("TCP Client connected\n");
        
        BinaryOperation op;
        while (recv(client_fd, &op, sizeof(op), 0) > 0) {
            printf("Received from client: ");
            print_operation(&op, true);
            
            introduce_error(&op, error_probability);
            
            // Connexion au serveur final...
            int server_fd = socket(AF_INET, SOCK_STREAM, 0);
            // ...
            
            send(server_fd, &op, sizeof(op), 0);
            recv(server_fd, &op, sizeof(op), 0);
            close(server_fd);
            
            send(client_fd, &op, sizeof(op), 0);
        }
        
        close(client_fd);
    }
}
```

Analyse du rôle du middle-man:
- Écoute en mode serveur sur PORT (8080)
- Pour chaque client accepté: traitement des opérations dans une boucle
- Introduction potentielle d'erreurs selon la probabilité fournie
- Établit une nouvelle connexion TCP vers le serveur pour chaque opération
- Relais bidirectionnel: client → serveur, puis serveur → client

#### 9.3.2 Run UDP Middle-Man

```c
void run_udp_middle_man(int error_probability) {
    // Configuration socket...
    
    // Boucle principale...
    while (1) {
        // Réception depuis client...
        recvfrom(sockfd, &op, sizeof(op), 0, (struct sockaddr*)&client_addr, &addr_len);
        
        printf("Received from client: ");
        print_operation(&op, true);
        
        introduce_error(&op, error_probability);
        
        // Transmission au serveur...
        sendto(sockfd, &op, sizeof(op), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        recvfrom(sockfd, &op, sizeof(op), 0, NULL, NULL);
        
        // Retour au client...
        sendto(sockfd, &op, sizeof(op), 0, (struct sockaddr*)&client_addr, addr_len);
    }
}
```

Spécificités UDP:
- Single-threaded: traitement séquentiel des requêtes
- Mémorisation de l'adresse client pour le retour
- Utilisation d'un seul socket pour toutes les communications

### 9.4 Module Serveur

#### 9.4.1 Verify Operation

```c
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
```

Fonction critique du serveur:
1. Vérification d'intégrité:
   - Sauvegarde du CRC reçu
   - Remise à zéro du champ CRC
   - Calcul du CRC local
   - Comparaison des CRC (détection d'erreur)
2. Traitement conditionnel:
   - Si CRC valide: calcul de l'opération demandée
   - Si opération invalide ou division par zéro: retour d'erreur
3. Codes de retour:
   - 0: succès
   - 1: erreur CRC
   - -1: erreur sémantique (division par zéro ou opération invalide)

#### 9.4.2 Run TCP Server

```c
void run_tcp_server() {
    // Configuration socket...
    
    while (1) {
        // Accepter client...
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (fork() == 0) {
            // Processus fils...
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
                    printf("Operation successful\n");
                }
                
                // Nouvel CRC et envoi...
                op.crc = compute_crc(&op);
                send(client_fd, &op, sizeof(op), 0);
            }
            
            close(client_fd);
            exit(0);
        }
        
        close(client_fd);
    }
}
```

Caractéristiques du serveur TCP:
- Architecture multi-process (fork pour chaque client)
- Vérification et calcul via `verify_operation`
- Recalcul du CRC après traitement
- Gestion propre des ressources (close des descripteurs non utilisés)

#### 9.4.3 Run UDP Server

```c
void run_udp_server() {
    // Configuration socket...
    
    // Boucle principale...
    while (1) {
        // Réception datagramme...
        recvfrom(sockfd, &op, sizeof(op), 0, (struct sockaddr*)&client_addr, &addr_len);
        
        printf("Received operation: ");
        print_operation(&op, true);
        
        int verification = verify_operation(&op);
        // Traitement similaire au TCP...
        
        // Envoi réponse...
        op.crc = compute_crc(&op);
        sendto(sockfd, &op, sizeof(op), 0, (struct sockaddr*)&client_addr, addr_len);
    }
}
```

Spécificités serveur UDP:
- Single-threaded: traitement séquentiel des datagrammes
- Même logique de vérification et calcul
- Communication stateless (sans connexion persistante)

---

Ce projet constitue une base solide pour comprendre les enjeux des systèmes distribués et l'importance des protocoles de communication robustes dans un environnement réseau non fiable.

---

## Annexes

### A. Compilation et exécution

```bash
# Compilation des composants
gcc -c shared.c -o shared.o -Wall -Wextra -std=c99
ar rcs libshared.a shared.o
gcc client.c -o client -L. -lshared -Wall -Wextra -std=c99
gcc middle_man.c -o middle_man -L. -lshared -Wall -Wextra -std=c99
gcc server.c -o server -L. -lshared -Wall -Wextra -std=c99

# Exécution (sur 3 terminals)
./server tcp
./middle_man tcp 20
./client tcp
```

### B. Méthode de test automatisé

Script bash pour tester automatiquement différentes configurations :

```bash
#!/bin/bash
# test_all.sh

PROTOCOLS="tcp udp"
ERROR_RATES="0 10 30 50"
OPERATIONS="+ - * /"

for proto in $PROTOCOLS; do
    for err in $ERROR_RATES; do
        echo "=== Testing $proto with $err% error rate ==="
        # Lancement des composants
        ./server $proto &
        SERVER_PID=$!
        sleep 1
        ./middle_man $proto $err &
        MIDDLE_PID=$!
        sleep 1
        
        # Tests automatiques
        for op in $OPERATIONS; do
            for i in {1..10}; do
                # Génération aléatoire des opérandes
                op1=$((RANDOM % 100))
                op2=$((RANDOM % 100))
                
                # Test avec opérandes calculés
                result=$(./client_auto $proto $op $op1 $op2)
                echo "$op $op1 $op2 = $result"
            done
        done
        
        # Nettoyage
        kill $SERVER_PID $MIDDLE_PID
        sleep 1
    done
done
```
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
