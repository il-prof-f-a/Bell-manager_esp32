#include <WebServer.h>
#include <WiFi.h>

// nome rete
const char *ssid = "Bell-Manager";
// password rete
const char *password = "12345678910";

// server nella porta 80
WebServer server(80);

// funzione per mandare la pagina
void handleRoot() {

 String page =
 
 

  server.send(200, "text/html", page);
}

// se ce un errore manda 404 errore
void handleNotFound() {
  server.send(404, "text/plain", "errore");
}

void setup() {
 Serial.begin(115200);
 delay(1000);
 // access point
 WiFi.mode(WIFI_AP);  
 WiFi.softAP(ssid, password);

 Serial.println();
 Serial.println("ACCESS POINT ATTIVO");
 Serial.print("SSID: ");
 Serial.println(ssid);
 Serial.print("IP del ESP: ");
 // IP 
 Serial.println(WiFi.softAPIP());   

 
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("Server web avviato!");

}

void loop() {
server.handleClient();

}
