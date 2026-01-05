#include "http_service.h"

static WebServer server(80);
SdFat *sdHttp;
bool http_active = false;

static String content_type(const String& path) {
    if (path.endsWith(".html")) return "text/html";
    if (path.endsWith(".js"))   return "application/javascript";
    if (path.endsWith(".css"))  return "text/css";
    if (path.endsWith(".csv"))  return "text/csv";
    return "application/octet-stream";
}


static void handle_list_files(void) {
    FsFile dir = sdHttp->open("trips");
    if (!dir || !dir.isDirectory()) {
        server.send(500, "application/json", "[]");
        return;
    }

    String json = "[";
    bool first = true;

    while (true) {
        FsFile file = dir.openNextFile();
        if (!file) break;

        char name[64];
        if (!file.isDirectory() && file.getName(name, sizeof(name))) {
            if (!first) json += ",";
            json += "\"" + String(name) + "\"";
            first = false;
        }
        file.close();
    }

    json += "]";
    server.send(200, "application/json", json);
}

static void handle_download() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Paramètre file manquant");
    return;
  }

  String path = "trips/" + server.arg("file");

  FsFile file = sdHttp->open(path.c_str(), O_RDONLY);
  if (!file) {
    server.send(404, "text/plain", "Fichier introuvable");
    return;
  }

  WiFiClient client = server.client();

  server.setContentLength(file.size());
  server.sendHeader(
    "Content-Disposition",
    "attachment; filename=\"" + server.arg("file") + "\""
  );
  server.send(200, "application/octet-stream", "");

  uint8_t buffer[512];
  while (file.available()) {
    int n = file.read(buffer, sizeof(buffer));
    if (n > 0) client.write(buffer, n);
  }

  file.close();
}

static void handle_index() {
  FsFile file = sdHttp->open("/www/index.html", O_RDONLY);
  if (!file) {
    server.send(500, "text/plain", "index.html introuvable");
    return;
  }

  WiFiClient client = server.client();

  server.setContentLength(file.size());
  server.send(200, "text/html", "");

  uint8_t buffer[512];
  while (file.available()) {
    int n = file.read(buffer, sizeof(buffer));
    if (n > 0) {
      client.write(buffer, n);
    }
  }

  file.close();
}


void http_loop() {
  if(http_active) server.handleClient();
}

void http_start(SdFat& sd) {
  sdHttp = &sd;
  server.on("/", handle_index);

  server.on("/files", handle_list_files);
  server.on("/download", handle_download);

  server.begin();
  http_active = true;
}

void http_stop(){
  if(!http_active) server.stop();
  http_active = false;
}
