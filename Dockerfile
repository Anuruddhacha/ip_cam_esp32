# Root Dockerfile for container hosts (Back4App, Render, Railway, Fly.io...).
# The deployable app is the WebSocket relay in ./relay; the rest of the repo
# is ESP32 firmware and is not needed at runtime.
FROM node:20-alpine

WORKDIR /app

# Install only the relay's production dependencies
COPY relay/package.json ./
RUN npm install --omit=dev

# Copy the relay source (server.js + public/)
COPY relay/ ./

# Most platforms inject PORT and route HTTPS/WSS (443) to it.
ENV PORT=8080
EXPOSE 8080

CMD ["node", "server.js"]
