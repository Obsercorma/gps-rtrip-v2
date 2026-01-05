#ifndef HTTP_SERVICE_H
#define HTTP_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SdFat.h>

void http_start(SdFat& sd);

void http_stop(void);

void http_loop();

static void setup_routes(void);

static void handle_index(void);

static void handle_download(void);

static void handle_list_files(void);

static String content_type(const String *path);

#endif
