
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <esp_wifi.h>

// ===== WIFI =====
const char* ssid = "Wifi Khali";

DNSServer dns;
WebServer server(80);

unsigned long dernierAffichage = 0;
const unsigned long INTERVALLE = 5000;

// ===== PAGE VIDEO =====
const char page[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>KHALEYY</title>
<style>
body {
  font-family: Arial, sans-serif;
  background: #18212b;
  color: white;
  text-align: center;
  padding: 15px;
}
video {
  width: 100%;
  max-width: 700px;
}
</style>
</head>
<body>
<video controls autoplay muted playsinline>
  <source src="/1.mp4" type="video/mp4">
  Votre navigateur ne prend pas en charge cette video.
</video>
</body>
</html>
)HTML";

// ===== PAGE PRINCIPALE =====
void racine() {
  server.send_P(200, "text/html; charset=utf-8", page);
}

// ===== VIDEO =====
void video() {
  File f = LittleFS.open("/1.mp4", "r");

  if (!f || f.isDirectory()) {
    if (f) f.close();
    server.send(404, "text/plain",
                "Video /1.mp4 introuvable dans LittleFS");
    return;
  }

  server.streamFile(f, "video/mp4");
  f.close();
}

// ===== APPAREILS CONNECTES =====
void afficherAppareils() {
  wifi_sta_list_t liste = {};

  esp_err_t erreur = esp_wifi_ap_get_sta_list(&liste);

  if (erreur != ESP_OK) {
    Serial.println("Erreur lecture WiFi");
    return;
  }

  Serial.println("\n===== WIFI KHALI =====");
  Serial.printf("Reseau : %s\n", ssid);
  Serial.printf("IP ESP32 : %s\n",
                WiFi.softAPIP().toString().c_str());
  Serial.printf("Appareils connectes : %d\n",
                (int)liste.num);

  for (int i = 0; i < liste.num; i++) {
    Serial.printf("Appareil %d : ", i + 1);

    for (int j = 0; j < 6; j++) {
      Serial.printf("%02X", liste.sta[i].mac[j]);
      if (j < 5) Serial.print(":");
    }

    Serial.println();
  }

  Serial.println("======================");
}

// ===== DEMARRAGE =====
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\nDemarrage ESP32...");

  // Initialiser la video
  if (!LittleFS.begin(false)) {
    Serial.println("ERREUR : LittleFS indisponible");
  } else {
    File f = LittleFS.open("/1.mp4", "r");

    if (f && !f.isDirectory()) {
      Serial.printf("Video trouvee : %u octets\n",
                    (unsigned int)f.size());
      f.close();
    } else {
      Serial.println("ATTENTION : /1.mp4 absent");
      if (f) f.close();
    }
  }

  // Creer le reseau WiFi
  WiFi.mode(WIFI_AP);

  WiFi.softAPConfig(
    IPAddress(192, 168, 4, 1),
    IPAddress(192, 168, 4, 1),
    IPAddress(255, 255, 255, 0)
  );

  if (!WiFi.softAP(ssid)) {
    Serial.println("ERREUR : impossible de creer le WiFi");
    return;
  }

  // DNS
  dns.start(53, "*", IPAddress(192, 168, 4, 1));

  // Serveur web
  server.on("/", HTTP_GET, racine);
  server.on("/1.mp4", HTTP_GET, video);

  server.onNotFound([]() {
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302, "text/plain", "Portail WiFi");
  });

  server.begin();

  Serial.println("\nESP32 PRET !");
  Serial.printf("WiFi : %s\n", ssid);
  Serial.println("Mot de passe : aucun");
  Serial.println("Adresse : http://192.168.4.1/");

  afficherAppareils();
}

// ===== BOUCLE =====
void loop() {
  dns.processNextRequest();
  server.handleClient();

  if (millis() - dernierAffichage >= INTERVALLE) {
    dernierAffichage = millis();
    afficherAppareils();
  }

  delay(10);
}
