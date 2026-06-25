'use strict';

// WebSocket relay for ESP32-CAM.
//
//   ESP32-CAM  --ws-->  /ingest  (publisher, token protected)
//   Browsers   --ws-->  /view    (subscribers, receive JPEG frames)
//
// The camera connects OUTBOUND, so this works from any network without
// port-forwarding. Run this on a public host (VPS / free tier).

const http = require('http');
const fs = require('fs');
const path = require('path');
const { WebSocketServer } = require('ws');

const PORT = process.env.PORT || 8080;
const TOKEN = process.env.TOKEN || 'changeme'; // must match the ESP32 URI

const INDEX_HTML = fs.readFileSync(path.join(__dirname, 'public', 'index.html'));

const server = http.createServer((req, res) => {
  if (req.url === '/' || req.url.startsWith('/index.html')) {
    res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
    res.end(INDEX_HTML);
  } else if (req.url === '/healthz') {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('ok');
  } else {
    res.writeHead(404);
    res.end('Not found');
  }
});

const wss = new WebSocketServer({ noServer: true });

const viewers = new Set();
let lastFrame = null; // Buffer of the most recent JPEG

server.on('upgrade', (req, socket, head) => {
  let url;
  try {
    url = new URL(req.url, `http://${req.headers.host}`);
  } catch {
    socket.destroy();
    return;
  }

  const role = url.pathname;
  const token = url.searchParams.get('token');

  // Only the camera (publisher) needs the token.
  if (role === '/ingest' && token !== TOKEN) {
    console.warn('Rejected /ingest: bad token');
    socket.destroy();
    return;
  }

  if (role !== '/ingest' && role !== '/view') {
    socket.destroy();
    return;
  }

  wss.handleUpgrade(req, socket, head, (ws) => {
    if (role === '/ingest') {
      console.log('Camera connected');
      ws.on('message', (data) => {
        lastFrame = data; // Buffer
        for (const v of viewers) {
          if (v.readyState === 1) v.send(data, { binary: true });
        }
      });
      ws.on('close', () => console.log('Camera disconnected'));
      ws.on('error', (e) => console.warn('Camera socket error:', e.message));
    } else {
      viewers.add(ws);
      console.log('Viewer connected. Total viewers:', viewers.size);
      if (lastFrame) ws.send(lastFrame, { binary: true }); // show something immediately
      ws.on('close', () => {
        viewers.delete(ws);
        console.log('Viewer left. Total viewers:', viewers.size);
      });
      ws.on('error', () => viewers.delete(ws));
    }
  });
});

server.listen(PORT, () => {
  console.log(`ESP32-CAM relay listening on :${PORT}`);
  console.log(`  Viewer page : http://localhost:${PORT}/`);
  console.log(`  Camera URI  : ws://<this-host>:${PORT}/ingest?token=${TOKEN}`);
});
