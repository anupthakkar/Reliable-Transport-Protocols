#include "../include/simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define max_buffer_size 1000
#define max_timeout 17

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

int B_received_sequences [1000];
int B_received_pointer = -1;
int B_start = 0;
int B_last = 0;

struct payload {
  struct msg message;
  struct payload *next;
};

struct window_payload {
  struct pkt packet;
  bool ack;
  float timeout;
  int buffer_seq_no;
};

struct payload *head = NULL;
struct payload *end = NULL;

struct window_payload *window = NULL;
struct window_payload *B_received = NULL;

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
    checksum += (unsigned char)packet->payload[i];
  }
  checksum += packet->acknum;
  checksum += packet->seqnum;
  // printf("Checksum value: %d \n",checksum);
  return checksum;
}

void send_packet_from_A(int from_output){
  int checksum = 0;
  struct payload *new_message = (struct payload *) malloc(sizeof(struct payload));
  if(buffer_size == 0 ){
    return;
  }
  if(packets_filled == window_size){
    return;
  }
  new_message = pop_msg_from_buffer();
  // if((position_in_window + 1) % window_size == start_window){
  //   return;
  // }
  if(packets_filled != 0 && from_output == 1){
    position_in_window  = (position_in_window + 1) % window_size;
  }
  
  strcpy(window[position_in_window].packet.payload, new_message->message.data);
  // printf("Data in message.data is %s and in packetPayload is %s\n", new_message->message.data, window[position_in_window].packet.payload);
  window[position_in_window].packet.seqnum = A_sequence_Number;
  window[position_in_window].packet.acknum = A_sequence_Number;
  checksum = calculate_checksum(&window[position_in_window].packet);
  window[position_in_window].packet.checksum = checksum;
  window[position_in_window].ack = false;
  window[position_in_window].timeout = get_sim_time() + max_timeout;
  packets_filled += 1;
  tolayer3(0, window[position_in_window].packet);
  // printf("Sequence number sent %d from send packet from A\n", window[position_in_window].packet.seqnum);
  
  // printf("Sequence number sent %d \n", window[position_in_window].packet.seqnum);
  A_sequence_Number++;
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
  send_packet_from_A(1);
  if(timer_start == false){
    timer_start = true;
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
  if(packet.checksum != calculate_checksum(&packet)){
    // printf("Inside A Input checksum return\n");
    return;
  }
  if(packet.seqnum < window[start_window].packet.seqnum){
    return;
  }
  if(packet.acknum == window[start_window].packet.seqnum){
    window[start_window].ack = true;
    packets_filled -= 1;
    if(packets_filled == 0){
      start_window = (start_window + 1) % window_size;
      position_in_window = (position_in_window + 1) % window_size;
      if(buffer_size ==0){
        timer_start = false;
        stoptimer(0);
      }
      else {
        send_packet_from_A(0);

      }
    } 
    else {
      int pos = start_window;
      while(true){
        // printf("While inside A input seq num equals");
        if(pos == position_in_window || window[(pos + 1) % window_size].ack == false){
          break;
        }
        packets_filled -= 1;
        pos = (pos + 1) % window_size;
      }
      start_window = (pos + 1) % window_size;
      if (packets_filled == 0){
        position_in_window = start_window;
      }
      
      send_packet_from_A(0);

    }
  }

  if(packet.acknum > window[start_window].packet.seqnum){
    int pos = start_window;
    while(true){
      if(window[pos].packet.seqnum == packet.acknum){
        window[pos].ack = true;
        break;
      }
      if(pos==position_in_window) {
        break;
      }
      pos = (pos + 1) % window_size;
    }
  }

  return;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  // printf("\n%d is the time and inside timerinterrupt\n",get_sim_time());
  if(packets_filled != 0){
    int pos = start_window;
    while(true){
      // printf("While inside timer interrupt");
      if(window[pos].ack == false && window[pos].timeout < get_sim_time()){
        // printf("Sequence number sent in Timer interrupt %d in A input\n", window[pos].packet.seqnum);
        window[pos].timeout = get_sim_time() + max_timeout;
        tolayer3(0, window[pos].packet);
      }
      if(pos == position_in_window){
        break;
      }
      pos = (pos + 1) % window_size;
    }
    
    
  }
  starttimer(0, 1);
  
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  A_sequence_Number = 0;
  window_size = getwinsize();
  window = (struct window_payload *) malloc(sizeof(struct window_payload) * window_size);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

void send_to_app(struct pkt packet){
  tolayer5(1,packet.payload);
}

void send_ack(struct pkt packet){
  struct pkt response;
  response.acknum = packet.seqnum;
  response.seqnum = packet.seqnum;
  response.checksum = calculate_checksum(&response);
  tolayer3(1, response);
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  
  if(packet.checksum != calculate_checksum(&packet)){
    // printf("Inside B Input checksum\n");
    return;
  }
  // printf("Sequence number received %d \n", packet.seqnum);
  // printf("B_sequence number: %d\n", B_sequence_Number);
  if(packet.seqnum == B_sequence_Number){
    B_sequence_Number++;
    send_to_app(packet);
    send_ack(packet);

    B_received[B_start].buffer_seq_no = B_sequence_Number + window_size - 1;
    B_start = (B_start + 1) % window_size;
    while(true){
      // printf("Inside while B_received seq number: %d\n", B_received[B_start].packet.seqnum);
      if(B_received[B_start].packet.seqnum != B_sequence_Number){
        break;
      }
      send_to_app(B_received[B_start].packet);
      B_sequence_Number += 1;
      B_received[B_start].buffer_seq_no = B_sequence_Number + window_size - 1;
      B_start = (B_start + 1) % window_size;

    }
  }
  else {
    // printf("Inside else \n");
    int x = 0;
    if(packet.seqnum > B_sequence_Number){
      if(packet.seqnum <= B_sequence_Number + window_size){
        while(x < window_size){
          if(B_received[x].buffer_seq_no == packet.seqnum){
            B_received[x].packet = packet;
            send_ack(packet);
            break;
          }
          x++;
        }
      }
    }
    else{
      send_ack(packet);
    }
  }

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  B_sequence_Number = 0;
  B_received = (struct window_payload *) malloc(sizeof(struct window_payload) * window_size);
}




