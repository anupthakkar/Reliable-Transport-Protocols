#include "../include/simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define max_buffer_size 1000
#define max_timeout 20

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

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

int A_sequence_Number;
int B_sequence_Number;

int buffer_size = 0;

int packets_filled = 0;
int window_size;
int position_in_window = 0;
int start_window = 0;
bool timer_start = false;

struct payload {
  struct msg message;
  struct payload *next;
};

struct window_payload {
  struct pkt packet;
  bool ack;
  float timeout;
};

struct payload *head = NULL;
struct payload *end = NULL;

struct window_payload *window = NULL;

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

int send_packet_from_A(){
  int checksum = 0;
  // printf("Inside send packet from A \n");
  struct payload *new_message = (struct payload *) malloc(sizeof(struct payload));
  if(buffer_size == 0 ){
    return 0;
  }
  if(packets_filled == window_size){
    return 0;
  }
  new_message = pop_msg_from_buffer();
  // printf("After pop Inside send packet from A \n");
  for(int i=0; i<20; i++){
    window[position_in_window].packet.payload[i] = new_message->message.data[i];
  }
  // printf("Data in message.data is %s and in packetPayload is %s\n", new_message->message.data, window[position_in_window].packet.payload);
  window[position_in_window].packet.seqnum = A_sequence_Number;
  window[position_in_window].packet.acknum = A_sequence_Number;
  checksum = calculate_checksum(&window[position_in_window].packet);
  window[position_in_window].packet.checksum = checksum;
  window[position_in_window].ack = false;
  window[position_in_window].timeout = get_sim_time() + max_timeout;
  packets_filled += 1;
  // printf("Before sending Inside send packet from A \n");
  tolayer3(0, window[position_in_window].packet);
  // printf("Sequence number sent %d from send packet from A\n", window[position_in_window].packet.seqnum);
  position_in_window  = (position_in_window + 1) % window_size;
 
  A_sequence_Number++;
  return 1;
  // printf("After send packet from A positon_in_window: %d , A_seq_number: %d\n",position_in_window, A_sequence_Number);
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  struct payload *new_message = (struct payload *) malloc(sizeof(struct payload));
  new_message->message = message;
  new_message->next = NULL;
  add_msg_to_buffer(new_message);
  if(packets_filled == window_size){
    return;
  }
  send_packet_from_A();
  if(start_window == position_in_window - 1){
    // timer_start = true;
    // printf("Inside A out and starting timer for %d \n",max_timeout);
    starttimer(0, max_timeout);
  }
  return;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  // printf("Sequence number received %d in A input\n", packet.seqnum);
  if(packet.acknum != window[start_window].packet.seqnum){
    // printf("Inside A Input ack return\n");
    return;
  }
  if(packet.checksum != calculate_checksum(&packet)){
    // printf("Inside A Input checksum return\n");
    return;
  }
  stoptimer(0);
  packets_filled -= 1;
  window[start_window].packet.seqnum = -1;
  if(packets_filled == 0){
    start_window = 0;
    position_in_window = 0;
    int res = send_packet_from_A();
    if(res == 1){
      starttimer(0, max_timeout);
    }
  }
  else {
    start_window = (start_window + 1) % window_size;
    send_packet_from_A();
    // starttimer(0, max_timeout);
  }
  if(start_window != position_in_window || packets_filled == 1){
    starttimer(0, max_timeout);
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  int pos = start_window;
  int i = 0;
  while(i < packets_filled){
    if(window[pos].packet.seqnum != -1){
      // printf("Sequence number sent in Timer interrupt %d in A input\n", window[pos].packet.seqnum);
      tolayer3(0, window[pos].packet);
      break;
    }
    // if(pos == position_in_window - 1){
    //   break;
    // }
    pos = (pos + 1) % window_size;
    i++;
  }
  if(start_window != position_in_window - 1 || packets_filled == 1){
    starttimer(0, max_timeout);
  }

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  A_sequence_Number  = 0;
  window_size = getwinsize();
  window = (struct window_payload *) malloc(sizeof(struct window_payload) * window_size);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  // printf("Sequence number received is %d in B input\n", packet.seqnum);
  if(packet.checksum != calculate_checksum(&packet)){
    // printf("Inside B Input checksum\n");
    return;
  }
  if(packet.seqnum == B_sequence_Number){
    B_sequence_Number++;
    tolayer5(1, packet.payload);
  }
  if(packet.seqnum < B_sequence_Number){
    struct pkt response;
    response.acknum = packet.seqnum;
    response.seqnum = packet.seqnum;
    response.checksum = calculate_checksum(&response);
    tolayer3(1, response);
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  B_sequence_Number = 0;
}


