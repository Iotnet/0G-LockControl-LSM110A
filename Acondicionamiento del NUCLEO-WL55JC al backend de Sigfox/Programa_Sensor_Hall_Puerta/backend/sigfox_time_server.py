#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
sigfox_time_server.py — Callback BIDIR de HORA para Programa_Sensor_Hall_Puerta
================================================================================
Responde las peticiones de downlink (uplink 0xF0) con la hora actual:
  downlinkData (8 bytes) = [epoch Unix UTC uint32 BE][4 bytes reservados 0x00]
El firmware aplica el offset local (DOOR_TZ_OFFSET_S = UTC-6) al recibirla.

Uso (pruebas con laptop):
  1) python3 sigfox_time_server.py            (escucha en puerto 8000)
  2) Tunel publico, cualquiera de los dos:
       ngrok http 8000                        (requiere cuenta ngrok)
       cloudflared tunnel --url http://localhost:8000   (sin cuenta)
  3) En backend.sigfox.com -> Device Type -> Callbacks -> New -> Custom:
       Type: DATA / BIDIR   Channel: URL   Method: POST
       Url pattern:  https://<tu-tunel>/sigfox/time
       Content-Type: application/json
       Body:
         {"device":"{device}","time":{time},"data":"{data}","ack":{ack}}
     Y en el Device Type: Downlink data -> CALLBACK (este callback).
  4) Enciende la Nucleo: a los ~8 s manda F0 y ~45 s despues veras en la
     consola serie "Reloj sincronizado por downlink".

Empresa: 0G IoT Solutions (previamente WND Mexico) — https://0giotsolutions.com/
Fecha: Julio 2026 · Autor: Yahir Flores
"""

import json
import time
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, HTTPServer

PORT = 8000


class SigfoxTimeHandler(BaseHTTPRequestHandler):

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        raw = self.rfile.read(length).decode("utf-8", errors="replace")

        try:
            msg = json.loads(raw) if raw else {}
        except json.JSONDecodeError:
            msg = {}

        device = str(msg.get("device", "UNKNOWN"))
        data = str(msg.get("data", ""))
        ack = msg.get("ack", False)
        ack = (ack is True) or (str(ack).lower() == "true")

        now = int(time.time())  # epoch Unix UTC
        stamp = datetime.now(timezone.utc).strftime("%H:%M:%S UTC")

        print(f"[{stamp}] uplink de {device}: data={data or '-'} ack={ack}")

        if ack:
            # 4 bytes epoch BE + 4 reservados = 16 caracteres hex
            downlink = f"{now:08x}" + "00000000"
            body = json.dumps({device: {"downlinkData": downlink}})
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body.encode())
            print(f"          -> downlink {downlink}  (epoch {now} = "
                  f"{datetime.fromtimestamp(now, timezone.utc).isoformat()})")
        else:
            # Uplink normal (evento de puerta): sin downlink que responder
            self.send_response(204)
            self.end_headers()

    def log_message(self, *_args):
        pass  # silencia el log crudo del HTTPServer (usamos los prints)


if __name__ == "__main__":
    print("=" * 64)
    print(" Callback BIDIR de hora — 0G IoT Solutions (prev. WND Mexico)")
    print(f" Escuchando en http://0.0.0.0:{PORT}/sigfox/time")
    print(" Expon con: ngrok http 8000  |  cloudflared tunnel --url http://localhost:8000")
    print("=" * 64)
    HTTPServer(("0.0.0.0", PORT), SigfoxTimeHandler).serve_forever()
