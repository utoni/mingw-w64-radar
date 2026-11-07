#ifndef COMMON_H
#define COMMON_H 1

// Common header used by radar driver and radar imgui
extern "C" {
  #include <stdint.h>

  #define COMMON_BUFFER_SLOTS (3u)
  #define COMMON_RINGBUFFER_SLOTS (1024u)
  #define MAX_ENTITIES (64u)

  struct radar_entity {
    char name[64];
    float pos_x, pos_y, pos_z;
    float pitch, yaw;
  };

  struct radar_buffer_data {
    uint32_t count;
    struct radar_entity entities[MAX_ENTITIES];
  };

  struct radar_ringbuffer_data {
    char msg[256];
  };
};

#endif
