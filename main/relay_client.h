#pragma once

// Outbound WebSocket client that pushes whatever frame_buffer last
// published to the public relay (see ../relay). Does not touch the camera;
// if config::RELAY_ENABLED is false this is a no-op (local /stream still
// works on the LAN).
namespace relay_client {

void init();

}  // namespace relay_client
