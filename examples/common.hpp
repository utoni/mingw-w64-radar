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

  enum buffer_type {
    TYPE_ENTITES = 0,
    TYPE_TO_USER = 1,
    TYPE_TO_KERNEL = 2,
  };

  struct radar_buffer_data {
    uint32_t count;
    enum buffer_type type;
    union {
      struct radar_entity entities[MAX_ENTITIES];
      struct {
        uint8_t do_restart;
        uint8_t game_reads_disabled;
        uint8_t local_player_read_enabled;
      } to_user;
      struct {
        uint8_t do_restart;
        uint8_t disable_game_reads;
        uint8_t enable_local_player_read;
      } to_kernel;
    };
  };

  struct radar_ringbuffer_data {
    char msg[256];
  };
};

#endif
