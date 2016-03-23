#include <SPI.h>
#include <Ethernet.h>

//RaspberryPi
#include <SoftwareSerial.h>
SoftwareSerial RPI(10, 11);

//Ethernet
byte mac_addr[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};
EthernetClient client;
const char server_addr[] = "mdbraspberry.site88.net";

//MDB
#define BAUD 9600
#include <util/setbaud.h>
#define MDB_BUFFER_SIZE (36+1)  // section 2.2, +1 for safety

struct MDB_Byte {
  byte data;
  byte mode;
};

struct MDB_Byte recvData[MDB_BUFFER_SIZE];

//commands
#define VEND 0x13
#define READER 0x14

//vend sub commands
#define VEND_REQUEST 0x00
#define VEND_APPROVED 0x05
#define VEND_CANCEL 0x01
#define VEND_SUCCESS 0x02
#define VEND_FAILURE 0x03
#define SESSION_COMPLETE 0x04
#define NEGATIVE_VEND_REQUEST 0x06

//reader sub commands
#define READER_DISABLE 0x00
#define READER_ENABLE 0x01

uint8_t flag  = 0;
uint8_t flag2 = 1;
const char mscode[] = "OF01";
int Item;
float Price;
#define STATUS_SIZE 40
char Status[STATUS_SIZE];
int Remark;
#define GET_REQ_SIZE 140
char get_req[GET_REQ_SIZE];

void setup() {
  RPI.begin(57600);

  //Ethernet setup
  Ethernet_setup();

  //MDB setup
  memset(Status, '\0', STATUS_SIZE);
  memset(get_req, '\0', GET_REQ_SIZE);
  MDB_setup();
}

void loop() {

  byte MDB_Output = MDB_read();
  RPI.println(MDB_Output);

  if (MDB_Output == VEND) {
    //Cashless Device 1 Vend
    byte sub_command = MDB_read();
    if (sub_command == VEND_REQUEST) {
      //vend request
      flag = 0;
      Remark = 0;
      byte price1 = MDB_read();
      byte price2 = MDB_read();
      byte item1 = MDB_read();
      byte item2 = MDB_read();
      Item = item2;
      strncpy(Status, "vend_request", STATUS_SIZE);
      upload_status();
      if (price1 > 0) {
        Price = 25.6f + price2 * 0.1f;
      } else {
        Price = price2 * 0.1f;
      }
    }
    else if (sub_command == VEND_APPROVED) {
      strncpy(Status, "vend_approved", STATUS_SIZE);
      Remark = 1;
      upload_status();
    }
    else if (sub_command == VEND_CANCEL) {
      strncpy(Status, "vend_cancel", STATUS_SIZE);
      Remark = 0;
      upload_status();
    }
    else if (sub_command == VEND_SUCCESS) {
      strncpy(Status, "vend_success", STATUS_SIZE);
      Remark = 1;
      flag = 1;
      upload_data();
      upload_status();
    }
    else if (sub_command == VEND_FAILURE) {
      strncpy(Status, "vend_failure", STATUS_SIZE);
      Remark = 1;
      upload_status();
    }
    else if (sub_command == SESSION_COMPLETE) {
      if (flag != 1) {
        strncpy(Status, "vend_session_completed", STATUS_SIZE);
        upload_data();
        upload_status();
      } else {
        flag = 0;
      }
    }
    else if (sub_command == NEGATIVE_VEND_REQUEST) {
      strncpy(Status, "vend_denied", STATUS_SIZE);
      Remark = 0;
      upload_status();
    }
  }

  else if (MDB_Output == READER) {
    byte sub_command2 = MDB_read();
    if (sub_command2 == READER_DISABLE) {
      strncpy(Status, "reader_disable", STATUS_SIZE);
      if (flag2 == 1) {
        upload_status();
      }
      flag2 = 0;
    }
    else if (sub_command2 == READER_ENABLE) {
      strncpy(Status, "reader_enable", STATUS_SIZE);
      if (flag2 == 0) {
        upload_status();
      }
      flag2 = 1;
    }
  }
}

void Ethernet_setup() {
  if (Ethernet.begin(mac_addr) == 0) {
    RPI.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    while (1) {
      delay(1000);
    }
  }

  // print your local IP address:
  RPI.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    RPI.print(Ethernet.localIP()[thisByte], DEC);
    RPI.print(".");
  }
  RPI.println();
  delay(1000);
}

void MDB_setup() {
  // Clear tx/rx buffers
  memset(recvData, 0, sizeof(recvData));

  // Set baud rate
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;
  // Disable USART rate doubler (arduino bootloader leaves it enabled...)
  UCSR0A &= ~(1 << U2X0);
  // Set async, no parity, 1 stop, 9bit
  UCSR0C = (0 << UMSEL01) | (0 << UMSEL00) | (0 << UPM01) | (0 << UPM00) | (0 << USBS0) | (1 << UCSZ01) | (1 << UCSZ00);
  UCSR0B |= (1 << UCSZ02); // 9bit
  // Enable rx/tx
  UCSR0B |= (1 << RXEN0) | (1 << TXEN0);
}

byte MDB_read() {
  // is data available?
  while (!(UCSR0A & (1 << RXC0))) {}
  MDB_getByte(&recvData[0]);
  return recvData[0].data;
}

void MDB_getByte(struct MDB_Byte* mdbb) {
  int b = USART_Receive();
  memcpy (mdbb, &b, 2);
}

int USART_Receive() {
  unsigned char status, resh, resl;

  // Wait for data to be received
  while (!(UCSR0A & (1 << RXC0)));
  // Get status and 9th bit, then data
  // from buffer
  status = UCSR0A;
  resh = UCSR0B;
  resl = UDR0;

  // If error, return -1
  if ( (status & (1 << FE0)) | (status & (1 << DOR0)) | (status & (1 << UPE0))) {
    return -1;
  }

  // Filter the 9th bit, then return
  resh = (resh >> 1) & 0x01;
  return ((resh << 8) | resl);
}

//date, machinecode, item, price, remark
void upload_data() {
  //Http req
  if (client.connect(server_addr, 80)) {
    RPI.println("upload_data");
    // Make a HTTP request:
    RPI.println("GET req");

    char format[] =  "GET /upload_data.php?machineCode=%s&arrangementNo=%d&price=%d.%d&remark=%d HTTP/1.1\n" \
                     "Host: %s\n" \
                     "\n" \
                     "Connection: close\n" \
                     "\n" \
                     ;
    sprintf(get_req, format, mscode, Item, int(Price), int((Price - int(Price)) * 100), Remark, server_addr);
    client.println(get_req);
    client.stop();
  }
  else {
    // no connection to the server:
    RPI.println("connection failed");
  }
}

//date, machinecode, status
void upload_status() {
  //Http req
  if (client.connect(server_addr, 80)) {
    RPI.println("upload_status");
    // Make a HTTP request:
    RPI.println("GET req");

    char format[] = "GET /upload_status.php?machineCode=%s&status=%s HTTP/1.1\n" \
                    "Host: %s\n" \
                    "\n" \
                    "Connection: close\n" \
                    "\n" \
                    ;
    sprintf(get_req, format, mscode, Status, server_addr);

    client.println(get_req);
    client.stop();
  }
  else {
    // no connection to the server:
    RPI.println("connection failed");
  }
}

