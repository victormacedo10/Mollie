#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "RICC_Central/RF24Mesh/RF24Mesh.h"  
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>

RF24 radio(RPI_V2_GPIO_P1_15, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);  
RF24Network network(radio);
RF24Mesh mesh(radio,network);

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
    float U;
    float T;
    float P;
};

struct Soil_res{
    float val1, val2, val3;
};

struct Data_package{
  Soil_res US;
  float TS;
  BME_res BME;
  float R;
  float WS;
  uint16_t WD;
  uint16_t P;
};

Data_package data = {{0,0,0}, 0, {0,0,0}, 0, 0, 0, 0};

uint32_t state = 0;

void printData(int data_ID){
  /*printf("Umidade do solo 1: \n");
  printf("%lf \n",data.US.val1);
  printf("Umidade do solo 2: \n");
  printf("%lf \n",data.US.val2);
  printf("Umidade do solo 3: \n");
  printf("%lf \n",data.US.val3);
  printf("Temperatura do solo: \n");
  printf("%lf \n",data.TS);
  printf("Direção do vento: \n");
  printf("%d \n",data.WD);
  printf("Pluviosidade: \n");
  printf("%d \n",data.P);
  printf("Velocidade do vento: \n");
  printf("%lf \n",data.WS);
  printf("Umidade do ar: \n");
  printf("%lf \n",data.BME.U);
  printf("Temperatura do ar: \n");
  printf("%lf \n",data.BME.T);
  printf("Pressão atmosférica: \n");
  printf("%lf \n",data.BME.P);
  printf("Radiação Solar: \n");
  printf("%lf \n",data.R);
  printf("\n");*/
  time_t ltime;
  ltime=time(NULL);
  char *str = asctime(localtime(&ltime));
  char *str_out;
  str_out = strtok(str, "\n");
  fp=fopen("data.csv", "a");
  fprintf(fp,"%d,%s,%lf,%lf,%lf,%lf,%d,%d,%lf,%lf,%lf,%lf,%lf\n", \
          data_ID, str_out, data.US.val1, data.US.val2, \
          data.US.val3, data.TS, data.WD, data.P, data.WS, \
          data.BME.U, data.BME.T, data.BME.P, data.R);
  fclose(fp);
}

int main(int argc, char** argv) {

  // void setup  
    // Set the nodeID to 0 for the master node
    mesh.setNodeID(0);
    // Connect to the mesh
    printf("start\n");
    mesh.begin();
    radio.printDetails();
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
        case 'P': 
          network.read(header,&data,sizeof(data));
          ID = mesh.getNodeID(header.from_node);
          printf("Received package from: ");
          printf("%d \n", ID);
          printData(ID);
          printf("**********************************\n");
          break;
        case 'M':
          network.read(header, &state, sizeof(state));
          ID = mesh.getNodeID(header.from_node);
          printf("Received actuator state from: ");
          printf("%d \n", ID);
          if(state == 0){
            printf("Actuator state OFF \n");
          }
          else if(state == 1){
            printf("Actuator state ON \n");
          }
          else{
            printf("Unknown state");
          }
          break;
        default:network.read(header, 0, 0);break;
      }
    }

    if(millis() - displayTimer > 5000){
      ctr++;
      printf("\n");
      printf("********Assigned Addresses********\n");
      for(int i=0; i<mesh.addrListTop; i++){
          payload_t payload = {millis(), ctr};
          RF24NetworkHeader header(mesh.addrList[i].address, 8); //Constructing a header
          if(network.write(header, &payload, sizeof(payload))==1){
            printf("NodeID: ");
            printf("%d", mesh.addrList[i].nodeID);
            printf(" RF24Network Address: 0");
            printf("%d \n", mesh.addrList[i].address);
            if(mesh.addrList[i].nodeID == 1){
              fs = fopen("control.txt", "r");
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
          else{
            mesh.remove(mesh.addrList[i].nodeID)
          }

      }
      printf("**********************************\n");
      displayTimer = millis();
    }
  }

  return 0;
}
