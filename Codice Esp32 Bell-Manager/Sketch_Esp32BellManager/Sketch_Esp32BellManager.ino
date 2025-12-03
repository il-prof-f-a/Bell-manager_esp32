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
 "<html>"
  "<head>"
  "<title>WebPage accendi/spengi led</title>"

  "<script>"
      "function StatoBottone() {"
          "var xhttp = new XMLHttpRequest();"
          "xhttp.onreadystatechange = function () {"
              "if (this.readyState == 4 && this.status == 200) {"
                  "document.getElementById('statoBottone').innerHTML = this.responseText;"
              "}"
          "};"
          "xhttp.open('GET', '/stato', true);"
          "xhttp.send();"
      "}"

      "function LedOn(){"
          "var xhttp = new XMLHttpRequest();"
          "xhttp.open('GET', '/on', true);"
          "xhttp.send();"
      "}"

      "function LedOff(){"
          "var xhttp = new XMLHttpRequest();"
          "xhttp.open('GET', '/off', true);"
          "xhttp.send();"
      "}"

      "setInterval(StatoBottone, 1000);"

  "</script>"
  "</head>"

  "<body>"

  "<h1>Accendi o Spegni LED ESP32</h1>"

  "<button onclick=\"LedOn()\">Accendi</button>"
  "<button onclick=\"LedOff()\">Spegni</button>"

  "<h1>Il tasto: <span id=\"statoBottone\"></span></h1>"

  "</body>"
  "</html>";
 
 

  server.send(200, "text/html", page);
}

// se ce un errore manda 404 errore
void handleNotFound() {
  server.send(404, "text/plain", "errore");
}

void setup() {
  
 Serial.begin(115200);
 Serial.println("Setup started");
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
 Serial.println("loop");
server.handleClient();
delay (100);

}
