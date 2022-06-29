#include "../include/simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define max_buffer_size 1000
#define timeout 20
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

int A_sequence_Number;
// int A_waiting_ack_number = 0;
int B_sequence_Number;
// int B_sending_ack_number = 0;
// int A_sent = 0;
// int B_sent = 0;
bool wait_for_ack = false;
int buffer_size = 0;
bool start = true;

struct pkt current_packet;

int calculate_checksum(struct pkt *packet){
  int checksum = 0;
  // printf("Inside Checksum\n");
  if(packet == NULL){
    return checksum;
  }

  for(int i=0; i<20; i++){
    checksum += packet->payload[i];
  }
  checksum += packet->acknum;
  checksum += packet->seqnum;
  // printf("Checksum value: %d \n",checksum);
  return checksum;
}

struct payload {
  struct msg message;
  struct payload *next;
};

struct payload *head = NULL;
struct payload *end = NULL;

void add_msg_to_buffer(struct payload *p){
  // printf("Inside Adding to buffer\n");
  if(p != NULL && buffer_size < max_buffer_size){
    if(end == NULL){
      head = (struct payload *) malloc(sizeof(struct payload));
      end = (struct payload *) malloc(sizeof(struct payload));
      head = p;
      end = p;
    }
    else{
      end->next = (struct payload *) malloc(sizeof(struct payload));
      end->next = p;
      end = end->next;
    }
    buffer_size += 1;
  }
  return;
}

struct payload * pop_msg_from_buffer(){
  // printf("Inside Popping from buffer\n");
  struct payload *p = (struct payload *) malloc(sizeof(struct payload));
  if(buffer_size != 0){
    p = head;
    head = head->next;
    buffer_size -= 1;
    if(buffer_size == 0){
      head = NULL;
      end = NULL;
    }
  }
  return p;
}

void send_packet_from_A(){
  // printf("Buffer Size inside send packet: %d \n", buffer_size);
  int checksum = 0;
  struct payload *new_message = (struct payload *) malloc(sizeof(struct payload));
  if (buffer_size == 0){
    return;
  }
  new_message = pop_msg_from_buffer();
  for(int i=0; i<20; i++){
    current_packet.payload[i] = new_message->message.data[i];
  }
  current_packet.seqnum = A_sequence_Number;
  current_packet.acknum = A_sequence_Number;
  checksum = calculate_checksum(&current_packet);
  current_packet.checksum = checksum;
  tolayer3(0, current_packet);
  wait_for_ack = true;
  starttimer(0, timeout);
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  // printf("A_Sent %d, B_Sent %d start of A output \n", A_sent, B_sent);
  int checksum = 0;
  struct payload *new_message = (struct payload *) malloc(sizeof(struct payload));
  new_message->message = message;
  new_message->next = NULL;
  add_msg_to_buffer(new_message);
  // printf("Buffer Size: %d \n", buffer_size);
  //logic to send only after ack received
  if(wait_for_ack){
    //do nothing
    // printf("A_Sent %d, B_Sent %d if both not equal \n", A_sent, B_sent);
    // printf("Inside A Output do nothing\n");
    return;
  }
  // printf("Inside A Output pop and send\n");
  // A_sent = A_sequence_Number;
  // new_message = pop_msg_from_buffer();
  // if (new_message == NULL){
  //   return;
  // }
  // for(int i=0; i<20; i++){
  //   current_packet.payload[i] = new_message->message.data[i];
  // }
  // current_packet.seqnum = A_sequence_Number;
  // current_packet.acknum = A_sequence_Number;
  // checksum = calculate_checksum(&current_packet);
  // current_packet.checksum = checksum;
  // tolayer3(0, current_packet);
  // wait_for_ack = true;
  // starttimer(0, timeout);
  // printf("A_Sent %d, B_Sent %d end of A output\n", A_sent, B_sent);

  send_packet_from_A();

  return;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  // printf("Inside A Input\n");
  // printf("A_Sent %d, B_Sent %d start A input\n", A_sent, B_sent);
  if(packet.acknum != A_sequence_Number){
    // printf("Inside A Input ack return\n");
    return;
  }
  if(packet.checksum != calculate_checksum(&packet)){
    // printf("Inside A Input checksum return\n");
    return;
  }
  // printf("Inside A Input after returns\n");
  stoptimer(0);
  A_sequence_Number = (A_sequence_Number + 1) % 2;
  wait_for_ack = false;
  send_packet_from_A();
  // A_waiting_ack_number = A_sequence_Number;
  return;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  // printf("Inside A timer interrupt\n");
  // if(A_sent != B_sent){
  // printf("A_Sent %d, B_Sent %d timer interrupt \n", A_sent, B_sent);
  // printf("Inside A interrupt send again\n");
  stoptimer(0);
  tolayer3(0, current_packet);
  starttimer(0, timeout);
  // }
  return;
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  A_sequence_Number = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  // printf("Inside B Input\n");
  // printf("A_Sent %d, B_Sent %d start B input \n", A_sent, B_sent);
  if(packet.seqnum != B_sequence_Number){
    // printf("Inside B Input seqnum\n");
    return;
  }
  if(packet.checksum != calculate_checksum(&packet)){
    // printf("Inside B Input checksum\n");
    return;
  }
  // printf("Inside B after  returns\n");
  tolayer5(1, packet.payload);
  struct pkt response;
  response.acknum = B_sequence_Number;
  response.seqnum = B_sequence_Number;
  response.checksum = calculate_checksum(&response);
  // printf("Inside B send ack\n");
  // B_sent = B_sequence_Number;
  tolayer3(1, response);
  B_sequence_Number = (B_sequence_Number + 1) % 2;
  // printf("A_Sent %d, B_Sent %d end B input \n", A_sent, B_sent);
  // B_sending_ack_number = B_sequence_Number;
  // printf("Inside B done\n");
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  B_sequence_Number = 0;
}



