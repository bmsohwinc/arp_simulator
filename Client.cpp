/*  
    author:     sourabha (sbm12)
    Compile:    $ g++ Client.cpp -o c -lpthread
    Run:        $ ./c
*/


#include <bits/stdc++.h>
#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 

#define PORT 8080 
#define BFSZ 1024

using namespace std;

// cache of MAC addresses [client number : MAC address]
map < int, string > mp;
map< int, string > id_to_ip;

// set a default IP address and MAC address
string myIP;
string myMAC;

// to handle messages sent by the SERVER
void *reader(void *sock) {
    int socket = *((int *)sock);
    char buffer[BFSZ];

    while (1) {
        memset(buffer, 0, BFSZ);
        // read that msg
        int valread = read(socket, buffer, BFSZ); 
        printf("[ S ] Server says: %s\n", buffer);

        if (buffer[0] == 'A' and buffer[1] == 'Q') {
            // Some client has asked for the MAC address.
            // Check if that IP address is matching with me

            // get the IP
            string s(buffer);
            int n = s.size();
            s = s.substr(3, n - 3);

            if (s == myIP) {
                // Send ack to the server, saying that I am the one who should send the MAC address
                // Prepare the ARP reply format
                s = "AR " + myMAC;
                memset(buffer, 0, BFSZ);
                strcpy(buffer, s.c_str());
                
                // Send this msg format
                send(socket, buffer, BFSZ, 0);
                printf("[ * ] I was matched with the IP, hence sent an AR reply of my MAC!\n");
            }
            else {
                // Ignore the msg
                printf("[ D ] IP did not match, hence broadcast ignored!\n");
            }

        }
        else if (buffer[0] == 'A' and buffer[1] == 'R') {
            // Server has replied with the MAC address of the destination device
            // Hence add it the map
            string s(buffer);
            int n = s.size();

            // get the client ID with whom the requested IP had matched
            int i = 3; // to start iterating from actual data
            string dest_id;
            while(1) {
                if (buffer[i] == ' ' or buffer[i] == '\0') {
                    break;
                }
                dest_id += buffer[i];
                // cout << "[ - ] dest_id = " << dest_id << "\n";
                i++;
            }
            
            // get the MAC address from that msg
            i++;
            s = s.substr(i, n - i);

            // store this data in the map
            int cid = stoi(dest_id);
            mp[cid] = s;

            printf("[ I ] Obtained the ARP reply for client ID: %2d; it's MAC = %s\n", cid, s.c_str());
            printf("\t==== Data was successfully added to the map\n");
        }
        else if (buffer[0] == 'N' and buffer[1] == 'N') {
            // A normal msg
            printf("\tThat was a normal message!\n");
        }
        else if (buffer[0] == 'L' and buffer[1] == 'L') {
            // list of active clients
            printf("\tThose are the active clients in the network\n");
            string s(buffer);
            int n = s.size();
            s = s.substr(3, n - 3);

            int i = 0;
            while(i < s.size()) {
                string ip;
                string id;
                while(1) {
                    if (s[i] == ':') {
                        break;
                    }
                    id += s[i];
                    i++;
                }

                int cid = stoi(id);

                i++;
                while(1) {
                    if (s[i] == ';') {
                        break;
                    }
                    ip += s[i];
                    i++;
                }
                i+=2;
                id_to_ip[cid] = ip;
            }

            printf("-------------------------------------------\n");
            printf("\tID to IP map\n");
            for (auto z : id_to_ip) {
                cout << "" << z.first << "\t" << z.second << "\n";
            }
            printf("-------------------------------------------\n");
        }
    }
}


void generateMAC() {

    // a random seed
    srand(unsigned(time(0)));

    vector<char> chars;
    int i;
    for (i=0;i<6;i++) {
        chars.push_back('A' + i);
    }

    for (i=0;i<10;i++) {
        chars.push_back('0' + i);
    }

    int n = chars.size();

    // shuffle the vector
    random_shuffle(chars.begin(), chars.end());

    // generate 6 segments, each having 2 random chars (MAC )
    for (i=0;i<6;i++) {
        int i1, i2;
        i1 = rand() % n;
        i2 = rand() % n;

        myMAC += chars[i1];
        myMAC += chars[i2];

        // add this colon only to starting 5 segments
        if (i < 5) {
            myMAC += ':';
        }
    }

    printf("[ D ] Your unique MAC-48 address in |hex| has been generated as : %s\n", myMAC.c_str());
}


int main(int argc, char const *argv[]) { 
    
    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 
    char buffer[BFSZ] = {0}; 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 

    // create a thread that will handle all the reads
    pthread_t p;
    pthread_create(&p, NULL, reader, (void *) &sock);

    // send the IP data
    printf("[ D ] Enter your device IP address: ");
    cin >> myIP;
    generateMAC();    
    memset(buffer, 0, BFSZ);
    string s = "DD " + myIP;
    strcpy(buffer, s.c_str());
    send(sock, buffer, BFSZ, 0);
    printf("[ I ] IP data was successfully sent to the server!\n");


    printf("[ I ] Here is the msg format:\n");
    printf("=======================================\n");
    printf("1. To send normal messages:\n\tNN <destination_client_id> <message>\n");
    printf("2. To send ARP request:\n\tAQ <destination_IP>\n");
    printf("3. To get list of available client IDs and IPs:\n\tLL\n");
    printf("=======================================\n");

    while(1) {
        char buffer[BFSZ];
        memset(buffer, 0, BFSZ);
        printf("Your msg: ");
        // scanf("%s", buffer);
        cin.getline(buffer, BFSZ);
        // check for the format
        if (buffer[0] == 'L' and buffer[1] == 'L') {
            // simply send the msg
            send(sock , buffer , BFSZ , 0 );
        }
        else if (buffer[0] == 'N' and buffer[1] == 'N') {
            // check if this client id's MAC is already known
            int i = 3;
            string dest_id;
            // printf("[ + ] buffer[0] = %c, buffer[1] = %c, buffer[2] = %c, buffer[3] = %c, buffer[4] = %c\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
            // cout << "[ + ] for i = 3, buffer[i] = " << buffer[i] << "\n";
            while(1) {
                if (buffer[i] == ' ' or buffer[i] == '\0') {
                    break;
                }
                dest_id += buffer[i];
                // cout << "[ - ] dest_id = " << dest_id << "\n";
                i++;
            }

            int cid = stoi(dest_id);
            if (mp[cid].size() == 0) {
                printf("[ E ] This client's MAC is not yet known. Please send an ARP request to %s before talking to it!\n", id_to_ip[cid].c_str());
                printf("\tMessage could not be sent to the server!\n");
            }
            else {
                // no problem in sending to the destination, as it's MAC is already known!
                send(sock , buffer , BFSZ , 0 );
            }
        }
        else if (buffer[0] == 'A' and buffer[1] == 'Q') {
            // simply send the msg
            send(sock , buffer , BFSZ , 0 );

            // can be improved for checking duplicate requests!
        }
        else {
            printf("[ E ] Sorry! Wrong Format!!\n");
        }
        
    }

    // close the thread
    pthread_join(p, NULL);

    return 0; 
} 