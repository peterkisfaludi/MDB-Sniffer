#include <SPI.h>
#include <Ethernet.h>

//SD
#include <SD.h>
#define chipSelect 4
const String MDBLog = "mdb.csv";

byte mac_addr[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};
EthernetClient client;
char server_addr[] = "mdbraspberry.site88.net";

void setup() {
  // put your setup code here, to run once:
  //pinMode(4, OUTPUT);
  //digitalWrite(4, HIGH);
  Serial.begin(9600);

  //Ethernet setup
  Ethernet_setup();

  //Http req
  if (client.connect(server_addr, 80)) {
    Serial.println("connected");
    // Make a HTTP request:
    Serial.println("GET req");

    char get_req[] = "GET /add.php?id=3&val=3 HTTP/1.1\n" \
                      "Host: mdbraspberry.site88.net\n" \
                      "\n" \
                      "Connection: close\n" \
                      "\n" \
                      ;
    
    unsigned long start = micros();       
    client.println(get_req);
    client.stop();
    Serial.println(micros() - start);
  }
  else {
    // kf you didn't get a connection to the server:
    Serial.println("connection failed");
  }
}

void loop() {

  delay(1000);
  Serial.println("OKS");
  return;
}

void Ethernet_setup() {
  if (Ethernet.begin(mac_addr) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    while (1) {
      delay(1000);
    }
  }

  // print your local IP address:
  Serial.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
  delay(1000);
}

