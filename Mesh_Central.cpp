#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "ricc/RF24Mesh/RF24Mesh.h"  
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>

#define n 1000

RF24 radio(RPI_V2_GPIO_P1_15, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);  
RF24Network network(radio);
RF24Mesh mesh(radio,network);

int active_nodes[n] = {0};
int active_nodes_tmp[n] = {0};

struct payload_t {
  unsigned long ms;
  unsigned long counter;
};

uint8_t turn_on = 0b11001100;
uint8_t turn_off = 0b00110011;
unsigned long ctr = 0;

uint32_t displayTimer = 0;

FILE *fp;

struct BME_res{
    uint16_t U;
    uint16_t T;
    uint16_t P;
};

struct Soil_res{
    uint8_t val1, val2, val3;
};

struct Data_package{
  Soil_res US;
  uint16_t TS;
  BME_res BME;
  uint16_t R;
  float WS;
  uint16_t WD;
  uint16_t P;
};

Data_package data = {{0,0,0}, 0, {0,0,0}, 0, 0, 0, 0};

uint32_t state = 0;

int cfileexists(const char * filename){
    FILE *file;
    if((file = fopen(filename, "r"))){
        fclose(file);
        return 1;
    }
    return 0;
}

void printData(int data_ID){
  time_t ltime;
  ltime=time(NULL);
  char *str = asctime(localtime(&ltime));
  char *str_out;
  str_out = strtok(str, "\n");
  char data_addr[80];
  sprintf(data_addr, "data/%d_%s", data_ID, str_out);
  fp=fopen(data_addr, "w+");
  fprintf(fp,"%d,%d,%d,%d,%d,%d,%.2f,%d,%d,%d,%d", \
          data.US.val1, data.US.val2, \
          data.US.val3, data.TS, data.WD, data.P, data.WS, \
          data.BME.U, data.BME.T, data.BME.P, data.R);
  fclose(fp);
}

void appendActiveNodes(int ID){
  for(int i=0; i<n; i++){
    if(active_nodes[i] == 0){
      active_nodes[i] = ID;
      break;
    }
  }
}

void resetActiveNodes(){
  for(int i=0; i<n; i++){
    if(active_nodes[i] == 0){
      break;
    }
    active_nodes[i] = 0;
  }
}

int searchActiveNodes(int ID){
  for(int i=0; i<n; i++){
    if(active_nodes[i] == ID){
      return 1;
    }
    else if(active_nodes[i] == 0){
      return 0;
    }
  }
  return -1;
}

void removeDeactiveNodes(){
  if(active_nodes[0] == 0){
    system("sudo rm -rf dev/");
    system("mkdir dev");
    system("sudo rm -rf data/");
    system("mkdir data");
  }
  else{
    for(int i=0; i<n; i++){
      if(active_nodes_tmp[i] == 0){
        break;
      }
      if(!searchActiveNodes(active_nodes_tmp[i])){
        char str_rem[80];
        sprintf(str_rem, "sudo rm dev/est_%d", active_nodes_tmp[i]);
        system(str_rem);
      }
    }
  }
}

int main(int argc, char** argv) {
  system("sudo rm dhcplist.txt");
  mesh.setNodeID(0);
  mesh.begin();
  removeDeactiveNodes();
  FILE *fs;
  char ch;
  int actuator_control;
  int ID;
  // void loop
  while(1){
    mesh.update();
    mesh.DHCP();

    // Check for incoming data from the sensors
    if(network.available()){
      RF24NetworkHeader header;
      network.peek(header);
      switch(header.type){
        case 'Q': 
          network.read(header,&data,sizeof(data));
          ID = mesh.getNodeID(header.from_node);
          printf("Received package from: ");
          printf("%d (addr %d)\n", ID, mesh.getAddress(ID));
          // printf("Enviando os dados\n");
          // printf("\n");
          // printf("Umidade do solo 1: ");
          // printf("%d\n", data.US.val1);
          // printf("Umidade do solo 2: ");
          // printf("%d\n", data.US.val2);
          // printf("Umidade do solo 3: ");
          // printf("%d\n", data.US.val3);
          // printf("Temperatura do solo: ");
          // printf("%f\n", data.TS);
          // printf("Direção do vento: ");
          // printf("%d\n", data.WD);
          // printf("Pluviosidade: ");
          // printf("%d\n", data.P);
          // printf("Velocidade do vento: ");
          // printf("%f\n", data.WS);
          // printf("Umidade do ar: ");
          // printf("%lf\n", data.BME.U);
          // printf("Temperatura do ar: ");
          // printf("%f\n", data.BME.T);
          // printf("Pressão atmosférica: ");
          // printf("%f\n", data.BME.P);
          // printf("Radiação Solar: ");
          // printf("%f\n", data.R);
          if(!searchActiveNodes(ID)){
            appendActiveNodes(ID);
          }
          printData(ID);
          break;
        case 'S':
          network.read(header, &state, sizeof(state));
          ID = mesh.getNodeID(header.from_node);
          printf("Received actuator state from: ");
          printf("%d (addr %d) -> ", ID, mesh.getAddress(ID));
          if(state == 0){
            printf("OFF \n");
          }
          else if(state == 1){
            printf("ON \n");
          }
          else{
            printf("Unknown state");
          }
          if(!searchActiveNodes(ID)){
            appendActiveNodes(ID);
          }
          break;
        default:network.read(header, 0, 0);break;
      }
    }

    if(millis() - displayTimer > 5000){
      printf("\n");
      printf("********Assigned Addresses********\n");
      for(int i=0; i<n; i++){
          if(active_nodes[i] == 0){
            break;
          }
          ID = active_nodes[i];
          printf("NodeID: ");
          printf("%d", ID);
          printf(" RF24Network Address: ");
          printf("%d \n", mesh.getAddress(ID));
          char str[80];
          sprintf(str, "dev/est_%d", ID);
          if(!cfileexists(str)){
              FILE *file;
              file = fopen(str, "w+");
              time_t ltime;
              ltime=time(NULL);
              char *str_time = asctime(localtime(&ltime));
              char *str_out;
              str_out = strtok(str_time, "\n");
              fprintf(file,"%s", str_out);
              fclose(file);
          }
          if(active_nodes[i] == 1){
            RF24NetworkHeader header(mesh.getAddress(ID), 8); //Constructing a header
            fs = fopen("actuator/command", "r");
            ch = fgetc(fs);
            actuator_control = atoi(&ch);
            if(actuator_control == 1 && state == 0){
              printf("Sending to Actuator: ");
              printf("%d \n", turn_on);
              network.write(header, &turn_on, sizeof(turn_on));
            }
            else if(actuator_control==0 && state == 1){
              printf("Sending to Actuator: ");
              printf("%d \n", turn_off);
              network.write(header, &turn_off, sizeof(turn_off));
            }
          }
      }
      removeDeactiveNodes();
      for(int k = 0; k < n; k++){
        if(active_nodes[k] == 0 && active_nodes_tmp[k] == 0){
          break;
        }
        active_nodes_tmp[k] = active_nodes[k];
      }
      resetActiveNodes();
      printf("**********************************\n");
      displayTimer = millis();
    }
    delay(2);
  }

  return 0;
}
