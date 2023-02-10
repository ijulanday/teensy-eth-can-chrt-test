#include <Arduino.h>
#include <NativeEthernet.h>
#include <FlexCAN_T4.h>

#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>

#include <ChRt.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 177);

unsigned int localPort = 8888;      // local port to listen on

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged";        // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;

  

void canSniff(const CAN_message_t &msg) 
{
  Serial.print("MB "); Serial.print(msg.mb);
  Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
  Serial.print("  LEN: "); Serial.print(msg.len);
  Serial.print(" EXT: "); Serial.print(msg.flags.extended);
  Serial.print(" TS: "); Serial.print(msg.timestamp);
  Serial.print(" ID: "); Serial.print(msg.id, HEX);
  Serial.print(" Buffer: ");

  for ( uint8_t i = 0; i < msg.len; i++ ) 
    Serial.print(msg.buf[i], HEX); Serial.print(" ");
  
  Serial.println();
}


static THD_WORKING_AREA(waThreadCAN, 2048);
static THD_FUNCTION(ThreadCAN, arg)
{
  (void)arg;

  Can0.begin();
  Can0.setBaudRate(1000000);
  Can0.setMaxMB(16);
  Can0.enableFIFO();
  Can0.enableFIFOInterrupt();
  Can0.onReceive(canSniff);
  Can0.mailboxStatus();
  
  while(1)
  {
    Can0.events();
    static uint32_t timeout = millis();
    if ( millis() - timeout > 200 ) {
      CAN_message_t msg;
      msg.id = random(0x1,0x7FE);
      for ( uint8_t i = 0; i < 8; i++ ) msg.buf[i] = i + 1;
      Can0.write(msg);
      timeout = millis();
      Serial.println("Wrote me a CAN message!");
    }
    chThdSleepMilliseconds(10);
  }
}

static THD_WORKING_AREA(waThreadUDP, 2048);
static THD_FUNCTION(ThreadUDP, arg)
{
  (void)arg;

  // start the Ethernet
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start UDP
  Udp.begin(localPort);

  while(1)
  {
    chThdSleepMilliseconds(10);

    int packetSize = Udp.parsePacket();
    if (packetSize) {
      Serial.print("Received packet of size ");
      Serial.println(packetSize);
      Serial.print("From ");
      IPAddress remote = Udp.remoteIP();
      for (int i=0; i < 4; i++) {
        Serial.print(remote[i], DEC);
        if (i < 3) {
          Serial.print(".");
        }
      }
      Serial.print(", port ");
      Serial.println(Udp.remotePort());

      // read the packet into packetBufffer
      Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
      Serial.println("Contents:");
      Serial.println(packetBuffer);
      Udp.flush();

      // send a reply to the IP address and port that sent us the packet we received
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write(ReplyBuffer);
      Udp.endPacket();
    }
  }
}

static THD_WORKING_AREA(waThreadBlink, 1024);
static THD_FUNCTION(ThreadBlink, arg)
{
  (void)arg;

  pinMode(LED_BUILTIN, OUTPUT); 

  while (1)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    chThdSleepMilliseconds(100);
    digitalWrite(LED_BUILTIN, LOW);
    chThdSleepMilliseconds(70);
    digitalWrite(LED_BUILTIN, HIGH);
    chThdSleepMilliseconds(90);
    digitalWrite(LED_BUILTIN, LOW);
    chThdSleepMilliseconds(1000);
  }
}

void chSetup()
{
  chThdCreateStatic(waThreadCAN, sizeof(waThreadCAN),
                          NORMALPRIO + 2, ThreadCAN, NULL);
  chThdCreateStatic(waThreadUDP, sizeof(waThreadUDP),
                          NORMALPRIO + 1, ThreadUDP, NULL); 
  chThdCreateStatic(waThreadBlink, sizeof(waThreadBlink),
                          NORMALPRIO, ThreadBlink, NULL); 
                      
}

void setup() 
{
  Serial.begin(115200); 
  while (!Serial) {
  }

  
  chBegin(chSetup);
  // chBegin() resets stacks and should never return.
  while (1) {}  
}

void loop() 
{
  
/* ... */
  
}