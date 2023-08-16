#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>


const char *values = " A 2 3 4 5 6 7 8 910 J Q K";
const char *suits = "♣️♦️♥️♠️";

const IPAddress apIp(192, 168, 0, 1);
DNSServer dnsServer;
ESP8266WebServer server(80);

void startNetwork(void);

void createNetworks(int value, int suit) {
  char ssid[33];

  memset(ssid, 0, sizeof(ssid));
  memcpy((char *)&ssid[0], &values[value * 2], 2);
  memcpy((char *)&ssid[2], &suits[suit * 6], 6);
  for (int padding = 8; padding < 32; padding++) {
    ssid[padding] = ' ';
    int channel = random() % 13 + 1;
    if (!WiFi.softAP(ssid, NULL, channel)) {
      Serial.printf("Unable to bring up SoftAP (ssid: '%s', channel: %d) ... \r\n", ssid, channel);
    }
    delay(100);
  }
}

void httpCard() {
  Serial << "httpCard: " << (String &)server.arg("val") << "\r\n";
  int value = -1;
  int suit = -1;
  auto card = server.arg("c");
  if (card.length() != 2) goto respond;
  switch (card[0]) {
    case 'a': value = 1; break;
    case '0': value = 10; break;
    case 'j': value = 11; break;
    case 'q': value = 12; break;
    case 'k': value = 13; break;
    default: value = card[0] - '0';
  }
  switch (card[1]) {
    case 'c': suit = 0; break;
    case 'd': suit = 1; break;
    case 'h': suit = 2; break;
    case 's': suit = 3; break;
    default: goto respond;
  }
  Serial.printf("Parsed card value: %d, suit: %d\r\n", value, suit);

respond:
  auto client = server.client();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
  client.println("<html><body>Success</body></html>");
  client.println();
  client.stop();

  if (value >= 0 && value <= 13) {
    WiFi.disconnect(true);
    while(true) {
      createNetworks(value-1, suit);
    }
  }
}

void httpHome() {
  Serial << "httpHome: " << (String &)server.uri() << "\r\n";
  auto client = server.client();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  // Display the HTML web page
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  // CSS to style the on/off buttons
  // Feel free to change the background-color and font-size attributes to fit your preferences
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
  client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
  client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
  client.println(".button2 {background-color: #77878A;}</style></head>");

  // Web Page Heading
  client.println("<body><h1>Passcode</h1>");
  client.println("<p><form action='/card'><input name='c' type='password'><br><input type='submit'></form></p>");
  client.println("</body></html>");
  client.println();
  client.stop();
}

void startNetwork() {
  WiFi.persistent(false);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));
  WiFi.softAP("trickster");

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", apIp);

  server.on("/hotspot-detect.html", httpHome);
  server.on("/", httpHome);
  server.on("/card", httpCard);
  server.onNotFound(httpHome);
  server.begin();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
  startNetwork();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
