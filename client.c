// COEN 233 Computer Networks
// 
// Programming Assignment 1
// Name: Yash Parekh
// SCU ID: W1607346



#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#define TIMER 3 // server timeout for resending the packet again
#define PORT 48000

// PROTOCOL PRIMITIVES
enum PROTOCOL_PRIMITIVES
{
    START_OF_PACKET_ID = 0XFFFF,
    END_OF_PACKET_ID = 0XFFFF,
    CLIENT_ID = 0XFF,
    LENGTH = 0XFF
};


// PACKET TYPES
enum PACKET_TYPES 
{
    DATA    = 0XFFF1,
    ACK     = 0XFFF2,
    REJECT  = 0XFFF3
        
};


// REJECT CODES
enum REJECT_CODES 
{
    OUT_OF_SEQUENCE         = 0xFFF4,
    LENGTH_MISMATCH         = 0xFFF5,
    END_OF_PACKET_MISSING   = 0xFFF6,
    DUPLICATE_PACKETS       = 0xFFF7
};


struct _DATA_PACKET 
{
    uint16_t start_of_packet_id;
    uint8_t client_id;
    uint16_t packet_type;
    uint8_t seqno;
    uint8_t length;
    char payload[255];
    uint16_t end_of_packet_id;
};

struct _ACK_PACKET 
{
    uint16_t start_of_packet_id;
    uint8_t client_id;
    uint16_t packet_type;
    uint8_t recieved_seqno;
    uint16_t end_of_packet_id;
};

struct _REJECT_PACKET
{
    uint16_t start_of_packet_id;
    uint8_t client_id;
    uint16_t packet_type;
    uint16_t reject_sub_code;
    uint8_t recieved_seqno;
    uint16_t end_of_packet_id;
};

// sets data packet before sending to the server
struct _DATA_PACKET set_data_packet()
{
    struct _DATA_PACKET data_packet;

    data_packet.start_of_packet_id = START_OF_PACKET_ID;
    data_packet.client_id = CLIENT_ID;
    data_packet.packet_type = DATA;
    data_packet.end_of_packet_id = END_OF_PACKET_ID;
    
    return data_packet;
}

// to print packet for verification at client's end
void print_packet(struct _DATA_PACKET data_packet)
{
	printf("\n");
	printf("|----------------------------------------------------------\n");
	printf("| Start of Packet ID: %x\n",data_packet.start_of_packet_id);
	printf("| Client ID: %hhx\n",data_packet.client_id);
	printf("| Data Packet Type: %x\n",data_packet.packet_type);
	printf("| Segment Number: %d\n",data_packet.seqno);
	printf("| Length: %d\n",data_packet.length);
	printf("| Payload: %s",data_packet.payload);
	printf("| End of Packet ID: %x\n",data_packet.end_of_packet_id);
	printf("|----------------------------------------------------------\n");
}


int main(int argc, char* argv[])
{
	FILE *fp;
   	struct _DATA_PACKET data_packet;	
	struct _REJECT_PACKET recieved_packet;
	
	// socket primitives to define socket
	struct sockaddr_in socket_address;
	socklen_t address_size;
	
	// payload message
	char payload[255];
	
	int socket_discriptor;
	int n = 0;
	int counter = 0;
	int seqno = 1;


	// set socket descriptor
	socket_discriptor = socket(AF_INET,SOCK_DGRAM,0);
	
	if(socket_discriptor < 0) 
	{
		perror("[ERROR] Failed To Create Socket!\n");
	}

	// sets zero-valued bytes to socket address
	bzero(&socket_address,sizeof(socket_address));
	socket_address.sin_family = AF_INET;
	socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
	socket_address.sin_port=htons(PORT);
	address_size = sizeof(socket_address);

	// configuring the socket to timeout in 3 seconds
	struct timeval time_value;
	time_value.tv_sec = TIMER; 
	time_value.tv_usec = 0;

	// sets optional values to socket. here it's timeout
	setsockopt(socket_discriptor, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_value,sizeof(struct timeval));
	data_packet = set_data_packet();

    // opening input file
	fp = fopen("input.txt", "rt");
	if(fp == NULL)
	{
		perror("[ERROR] Cannot Open File!\n");
		exit(0);
	}

    // transferring the data from file to the packet
	while(fgets(payload, sizeof(payload), fp) != NULL) 
	{
		n = 0;
		counter = 0;
		data_packet.seqno = seqno;
		strcpy(data_packet.payload,payload);
		data_packet.length = strlen(data_packet.payload);
		
		// Case 1: Not in the sequence
		if(seqno == 7) 
		{
			data_packet.seqno = data_packet.seqno+100;
		}

		// Case 2: Length Mismatch
		if(seqno == 8) 
		{
			data_packet.length++;
		}

		// Case 3: End of Packet Identifier
		if(seqno == 9) 
		{
			data_packet.end_of_packet_id = 0;
		}

		// if seqno is not 9 set END_OF_PACKET_ID
		if(seqno != 9) 
		{
			data_packet.end_of_packet_id = END_OF_PACKET_ID;
		}

		// Case 4: Duplicate Packet
		if(seqno == 10) 
		{
			data_packet.seqno = 2;
		}
		
		print_packet(data_packet);

		while(n <= 0 && counter < 3) 
		{

			// send packet to server
			sendto(socket_discriptor,&data_packet,sizeof(struct _DATA_PACKET),0,(struct sockaddr *)&socket_address,address_size);
			n = recvfrom(socket_discriptor,&recieved_packet,sizeof(struct _REJECT_PACKET),0,NULL,NULL);

			if(n <= 0 ) 
			{
				printf("No response from server for three seconds. Sending the packet again!\n");
				counter ++;
			}

			// if packet is transferred successfully, it will recieve acknolodgement packet
			else if(recieved_packet.packet_type == ACK) 
			{
				printf("\n");
				printf("Ack packet received!\n");
			}

			// else a reject packet is recieved from the server
			else if(recieved_packet.packet_type == REJECT) 
			{
				printf("\n");
				printf("Reject Packet Received!\n");
				printf("Reject Sub-code : %x \n" , recieved_packet.reject_sub_code);

				if(recieved_packet.reject_sub_code == OUT_OF_SEQUENCE ) 
				{
					printf("\n");
					printf("[ERROR] Packet Out of Sequence Error!\n");
				}
				
				else if(recieved_packet.reject_sub_code == LENGTH_MISMATCH) 
				{
					printf("\n");
					printf("[ERROR] Length Mismatch Error!\n");
				}
				
				else if(recieved_packet.reject_sub_code == END_OF_PACKET_MISSING ) 
				{
					printf("\n");
					printf("[ERROR] End of Packet Identifier Missing!\n");
				}
				
				else if(recieved_packet.reject_sub_code == DUPLICATE_PACKETS) 
				{
					printf("\n");
					printf("[ERROR] Duplicate Packet Received!\n");
				}
			}
		}


		// if client does't get response in 3 times, exit()
		if(counter >= 3 ) 
		{
			printf("\n");
			perror("[ERROR] Server does not respond!");
			printf("\n");
			exit(0);
		}

        seqno++;
        printf("\n");
        printf("###########################################################\n");		
	 }
}
