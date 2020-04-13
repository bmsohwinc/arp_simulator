#include <bits/stdc++.h>
#include <errno.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h>

#define TRUE 1 
#define FALSE 0 
#define PORT 8080
#define BFSZ 1024

using namespace std;

// map of client-socket ID and it's default IP
map <int, string> mp;

// the id of the client who initiated the ARP request
int arp_requester;


void print_map() {
    cout << "==================================\n";  
    for (auto z : mp) {              
        cout << z.first << "\t" << z.second << "\n";        
    }
    cout << "==================================\n";
}



int main(int argc , char *argv[]) { 
	int opt = 1; 
	int master_socket, addrlen, new_socket, client_socket[30], max_clients = 30, activity, i, valread, sd; 
	int max_sd; 
	struct sockaddr_in address; 
		
	char buffer[BFSZ]; //data buffer of 1K 
		
	//set of socket descriptors 
	fd_set readfds; 
		
	//a message 
	char *message = "\nHello Client!!\n"; 
	
	//initialise all client_socket[] to 0 so not checked 
	for (i = 0; i < max_clients; i++) { 
		client_socket[i] = 0; 
	} 
		
	//create a master socket 
	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) < 0) { 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	//set master socket to allow multiple connections and reuse the same port
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char *)&opt, 
		sizeof(opt)) < 0 ) { 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 
	
	//type of socket created 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( PORT ); 
		
	//bind the socket to localhost port 8888 
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) { 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	printf("Listener on port %d \n", PORT); 
		
	//try to specify maximum of 3 pending connections for the master socket - ??
	if (listen(master_socket, 3) < 0) { 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	} 
		
	//accept the incoming connection 
	addrlen = sizeof(address); 
	puts("Waiting for connections ..."); 
		
	while(1) { 
		//clear the socket set 
		FD_ZERO(&readfds); 
	
		//add master socket to set 
		FD_SET(master_socket, &readfds); 
		max_sd = master_socket; 
			
		//add child sockets to set 
		for ( i = 0 ; i < max_clients ; i++) { 
			//socket descriptor 
			sd = client_socket[i]; 
				
			//if valid socket descriptor then add to read list 
			if(sd > 0) 
				FD_SET( sd , &readfds); 
				
			//highest file descriptor number, need it for the select function 
			if(sd > max_sd) 
				max_sd = sd; 
		} 
	
		//wait for an activity on one of the sockets , timeout is NULL , 
		//so wait indefinitely 
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL); 
	
		if ((activity < 0) && (errno!=EINTR)) { 
			printf("select error"); 
		} 
			
		//If something happened on the master socket , 
		//then its an incoming connection 
		if (FD_ISSET(master_socket, &readfds)) { 
			if ((new_socket = accept(master_socket, 
					(struct sockaddr *)&address, (socklen_t*)&addrlen))<0) { 
				perror("accept"); 
				exit(EXIT_FAILURE); 
			} 
			
			//inform user of socket number - used in send and receive commands 
			printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs 
				(address.sin_port)); 
		
			//send new connection greeting message 
			if( send(new_socket, message, strlen(message), 0) != strlen(message) ) { 
				perror("send"); 
			} 
				
			puts("Welcome message sent successfully"); 
			
            // read the IP data and add it to the map
            memset(buffer, 0, BFSZ);            
            read(new_socket, buffer, BFSZ);
            printf("[ D ] IP data successfully received as: %s\n", buffer);
            string ss(buffer);
            int n = ss.size();
            ss = ss.substr(3, n - 3);
            mp[new_socket] = ss;
            print_map();

			//add new socket to array of sockets 
			for (i = 0; i < max_clients; i++) { 
				//if position is empty 
				if( client_socket[i] == 0 ) { 
					client_socket[i] = new_socket; 
					printf("Adding to list of sockets as %d\n" , i); 
						
					break; 
				} 
			} 
		} 
			
		//else its some IO operation on some other socket 
		for (i = 0; i < max_clients; i++) { 
			sd = client_socket[i]; 
				
			if (FD_ISSET( sd , &readfds)) { 
                memset(buffer, 0, BFSZ);
				//Check if it was for closing , and also read the 
				//incoming message 
				if ((valread = read( sd , buffer, BFSZ)) == 0) { 
					//Somebody disconnected , get his details and print 
					getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen); 
					printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port)); 
						
					//Close the socket and mark as 0 in list for reuse 
					close( sd ); 
					client_socket[i] = 0; 
				} 
				else { 

                    printf("[ Client ] #%2d says: %s\n", sd, buffer);

                    // check for the msg formats and do the expected
                    if (buffer[0] == 'L' and buffer[1] == 'L') {
                        
                        // prepare the list of clients and their IPs and IDs
                        string ans;
                        for (auto z : mp) {
                            ans += (to_string(z.first) + ":" + z.second + "; ");
                        }                        

                        // send this data to the client
                        ans = "LL " + ans;
                        cout << "[ I ] the LL request ans is: " << ans << "\n";
                        memset(buffer, 0, BFSZ);
                        strcpy(buffer, ans.c_str());
                        send(sd, buffer, BFSZ, 0);

                    }
                    else if (buffer[0] == 'N' and buffer[1] == 'N') {
                        // a normal message by the client to another one
                        // Get the destination_client_id
                        int z = 3;
                        string dest_id;
                        while(1) {
                            if (buffer[z] == ' ' or buffer[z] == '\0') {
                                break;
                            }
                            dest_id += buffer[z];
                            // cout << "[ - ] dest_id = " << dest_id << "\n";
                            z++;
                        }
                        int cid = stoi(dest_id);

                        // get that message
                        z++;
                        string s(buffer);
                        int n = s.size();
                        s = s.substr(z, n - z);

                        // send this message to the destination client
                        memset(buffer, 0, BFSZ);
                        s = "NN Client #" + to_string(sd) + " " + s;
                        strcpy(buffer, s.c_str());
                        send(cid, buffer, BFSZ, 0);                    
                    }
                    else if (buffer[0] == 'A' and buffer[1] == 'Q') {
                        // client has requested an ARP protocol
                        printf("[ I ] Processing the ARP request by client #%2d\n", sd);
						arp_requester = sd;
                        // forward this message to all the clients
                        for (int z = 0; z < max_clients; z++) {
                            if (client_socket[z] != 0 and client_socket[z] != sd) {
                                send(client_socket[z], buffer, BFSZ, 0);
                            }
                        }
                        printf("[ I ] Message has been broadcasted to all the connected devices!\n");
                    }
                    else if (buffer[0] == 'A' and buffer[1] == 'R') {
                        // a client has replied to the ARP request
                        printf("[ I ] ARP Reply has been obtained from client #%2d\n", sd);
                        // Send this MAC back to the requested client

                        // Get the MAC data
                        string s(buffer);
                        int n = s.size();
                        s = s.substr(3, n - 3);
                        
                        // Send the MAC to the requester, along with the dest_id
                        s = "AR " + to_string(sd) + " " + s;
                        memset(buffer, 0, BFSZ);
                        strcpy(buffer, s.c_str());
                        send(arp_requester, buffer, BFSZ, 0);
                    }
				} 
			} 
		} 
	} 
		
	return 0; 
} 
