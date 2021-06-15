
// COEN 233 Computer Networks
// 
// Programming Assignment 1
// Name: Yash Parekh
// SCU ID: W1607346



#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <strings.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
    DUPLICATE_PACKETS       = 0xFFF7,
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

// to print packet for verification at client's end
void print_packet(struct _DATA_PACKET data_packet)
{
    printf("\n");
    printf("|----------------------------------------------------------\n");
    printf("| Start of Packet ID: %x\n",data_packet.start_of_packet_id);
    printf("| Client ID: %hhx\n",data_packet.client_id);
    printf("| Data Packet Type: %x\n",data_packet.packet_type);
    printf("| Segment Number: %d \n",data_packet.seqno);
    printf("| Length: %d\n",data_packet.length);
    printf("| Payload: %s",data_packet.payload);
    printf("| End of Packet ID: %x\n",data_packet.end_of_packet_id);
    printf("|----------------------------------------------------------\n");
}

// Generate reject packet to send to the client
struct _REJECT_PACKET get_reject_packet(struct _DATA_PACKET data_packet) {
    struct _REJECT_PACKET reject_packet;
    reject_packet.start_of_packet_id = data_packet.start_of_packet_id;
    reject_packet.client_id = data_packet.client_id;
    reject_packet.recieved_seqno = data_packet.seqno;
    reject_packet.packet_type = REJECT;
    reject_packet.end_of_packet_id = data_packet.end_of_packet_id;
    return reject_packet;
}


// Generate acknoledgement packet to send to client
struct _ACK_PACKET get_acknowledgement_packet(struct _DATA_PACKET data_packet) {
    struct _ACK_PACKET ack_packet;
    ack_packet.start_of_packet_id = data_packet.start_of_packet_id;
    ack_packet.client_id = data_packet.client_id;
    ack_packet.recieved_seqno = data_packet.seqno;
    ack_packet.packet_type = ACK;
    ack_packet.end_of_packet_id = data_packet.end_of_packet_id;
    return ack_packet;
}


int main(int argc, char* argv[])
{
    int socket_descriptor, n;
    int expected_packet = 1;

    struct sockaddr_in socket_address;
    struct sockaddr_storage buffer;
    socklen_t address_size;

    struct _DATA_PACKET data_packet;
    struct _ACK_PACKET  ack_packet;
    struct _REJECT_PACKET reject_packet;

    // store the count of each segment number
    int segment_count[20];
    for(int j = 0; j < 20;j++) {
        segment_count[j] = 0;
    }

    socket_descriptor=socket(AF_INET,SOCK_DGRAM,0);
    
    bzero(&socket_address,sizeof(socket_address));
    
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr=htonl(INADDR_ANY);
    socket_address.sin_port=htons(PORT);
    
    bind(socket_descriptor,(struct sockaddr *)&socket_address,sizeof(socket_address));
    address_size = sizeof(socket_address);
    
    printf("\n");
    printf("[SUCCESS] Starting Server! Press \'Ctrl+C\' to close.\n\n");

    while(1){

        // recieve packet from the client
        n = recvfrom(socket_descriptor,&data_packet,sizeof(struct _DATA_PACKET),0,(struct sockaddr *)&buffer, &address_size);
        int length = strlen(data_packet.payload);

        print_packet(data_packet);
        segment_count[data_packet.seqno]++;
        
        // if seqno is 12 or 11then set segment_count to 1
        // so that if packet repeats it doesn't count fo duplicate packet
        if(data_packet.seqno == 12 || data_packet.seqno == 11) {
            segment_count[data_packet.seqno] = 1;
        }

        if(data_packet.seqno != expected_packet && data_packet.seqno != 2) {
            reject_packet = get_reject_packet(data_packet);
            reject_packet.reject_sub_code = OUT_OF_SEQUENCE;
            sendto(socket_descriptor,&reject_packet,sizeof(struct _REJECT_PACKET),0,(struct sockaddr *)&buffer,address_size);
            printf("\n");
            printf("[ERROR] Packet Out of Sequence Error!\n\n");
        }

        else if(length != data_packet.length) {
            reject_packet = get_reject_packet(data_packet);
            reject_packet.reject_sub_code = LENGTH_MISMATCH;
            sendto(socket_descriptor,&reject_packet,sizeof(struct _REJECT_PACKET),0,(struct sockaddr *)&buffer,address_size);
            printf("\n");
            printf("[ERROR] Length Mismatch Error!\n\n");
        }

        else if(segment_count[data_packet.seqno] != 1) {
            reject_packet = get_reject_packet(data_packet);
            reject_packet.reject_sub_code = DUPLICATE_PACKETS;
            sendto(socket_descriptor,&reject_packet,sizeof(struct _REJECT_PACKET),0,(struct sockaddr *)&buffer,address_size);
            printf("\n");
            printf("[ERROR] Duplicate Packet Recieved!\n\n");
        }
        
        else if(data_packet.end_of_packet_id != END_OF_PACKET_ID ) {
            reject_packet = get_reject_packet(data_packet);
            reject_packet.reject_sub_code = END_OF_PACKET_MISSING ;
            sendto(socket_descriptor,&reject_packet,sizeof(struct _REJECT_PACKET),0,(struct sockaddr *)&buffer,address_size);
            printf("\n");
            printf("[ERROR] End of Packet Identifier Missing\n\n");

        }
        
        else {
            if(data_packet.seqno == 11) {
                sleep(15);
            }
            ack_packet = get_acknowledgement_packet(data_packet);
            sendto(socket_descriptor,&ack_packet,sizeof(struct _ACK_PACKET),0,(struct sockaddr *)&buffer,address_size);
        }
        
        expected_packet++;
        printf("###########################################################\n");
    }
}
