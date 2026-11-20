#include <radar.hpp>

#include "common.hpp"
#include "imgui.h"

#define BUILD_USERMODE 1

#include <chrono>
#include <iostream>
#include <ipc.hpp>
#include <string_view>
#include <string.h>
#include <thread>

constexpr double TARGET_FPS = 60.0;
constexpr auto FRAME_TIME = std::chrono::duration<double>(1.0 / TARGET_FPS);

void radar_main(Radar::Config&& initial_config) {
  IPC::UserSharedMemory umem;
  if (!umem.Allocate({sizeof(radar_buffer_data), COMMON_BUFFER_SLOTS},
                     {sizeof(radar_ringbuffer_data), COMMON_RINGBUFFER_SLOTS}))
  {
    std::cerr << "\tFailed InitSharedMemory\n";
  }

  struct GameData {
    GameData(struct radar_entity const * const entity)
      : entity(*entity)
    {}
    bool is_new = true;
    struct radar_entity entity;
  };

  Radar::Drawer drawer(std::move(initial_config));
  drawer.Init();
  struct fake_game_options {
    bool do_resart = false;
    bool game_reads_disabled = false;
    bool enable_local_player_read = true;
  } current, next;
  bool send_pending = true;
  while (!drawer.WindowShouldClose()) {
    if (drawer.WindowPollEvents()) break;

    if (umem.OpenedByKernel()) {
      if (send_pending)
      {
        umem.WriteBufferData([&next](void *data) {
          auto slot_data = reinterpret_cast<struct radar_buffer_data*>(data);
          slot_data->type = TYPE_TO_KERNEL;
          slot_data->to_kernel.do_restart = next.do_resart;
          slot_data->to_kernel.disable_game_reads = next.game_reads_disabled;
          slot_data->to_kernel.enable_local_player_read = next.enable_local_player_read;
        });
        send_pending = false;
      }

      auto ret = umem.ReadBufferData([&drawer, &current, &next](void* data, uint64_t) {
        auto slot_data = reinterpret_cast<struct radar_buffer_data*>(data);
        if (slot_data->type == TYPE_TO_USER) {
          current.do_resart = slot_data->to_user.do_restart;
          current.game_reads_disabled = slot_data->to_user.game_reads_disabled;
          current.enable_local_player_read = slot_data->to_user.local_player_read_enabled;
          next.do_resart = current.do_resart;
          return;
        }
        if (slot_data->type != TYPE_ENTITES)
          return;
        for (auto i = 0u; i < slot_data->count; ++i) {
#if 0
          printf("[%u] Entity: %.*s\n", i, (int)sizeof(slot_data->entities[i].name),
                 slot_data->entities[i].name);
#endif
          const auto len = strnlen(slot_data->entities[i].name, sizeof(slot_data->entities[i].name));
          const auto key = std::string_view(slot_data->entities[i].name, len);
          auto data = GameData(&slot_data->entities[i]);
          if (key == "LocalPlayer") {
            drawer.UpdateLocalPlayer(
              Radar::Position{
                .x = data.entity.pos_x,
                .y = data.entity.pos_y,
                .rotation = data.entity.yaw
              }
            );
          } else {
            drawer.AddEntity(slot_data->entities[i].name,
                             Radar::Entity{
                               .position = {
                                 .x = data.entity.pos_x,
                                 .y = data.entity.pos_y,
                                 .rotation = data.entity.yaw
                               },
                               .type = "Enemy",
                               .color = {255, 0, 0}
                             });
          }
        }
      });
      if (!ret) {
        std::cerr << "IPC: Read Buffer failure!\n";
        break;
      }

      ret = umem.ReadRingbufferData([&drawer](void* data, uint64_t) {
        auto slot_data = reinterpret_cast<struct radar_ringbuffer_data*>(data);
        std::string msg(slot_data->msg, strnlen(slot_data->msg, sizeof(slot_data->msg)));
        drawer.AddLogMsg(std::move(msg));
      });
      if (!ret) {
        std::cerr << "IPC: Read RingBuffer failure!\n";
        break;
      }
    }

    auto frameStart = std::chrono::steady_clock::now();
    drawer.NewFrame();
    ImGui::SetNextWindowSize(
      ImVec2(125.0f, 800.0f),
      ImGuiCond_FirstUseEver
    );
    drawer.RenderLogWindow();
    ImGui::SetNextWindowSize(
      ImVec2(1024.0f, 768.0f),
      ImGuiCond_FirstUseEver
    );
    drawer.RenderRadarWindow();
    ImGui::SetNextWindowSize(
      ImVec2(225.0f, 75.0f),
      ImGuiCond_FirstUseEver
    );
    ImGui::Begin("Fake Game Options");
    send_pending |= ImGui::Checkbox("Game Reads Disabled", &next.game_reads_disabled);
    send_pending |= ImGui::Checkbox("Enable Local Player Read", &next.enable_local_player_read);
    send_pending |= ImGui::Checkbox("Do Restart", &next.do_resart);
    ImGui::End();
    drawer.EndFrame();

    auto elapsed = std::chrono::steady_clock::now() - frameStart;
    if (elapsed < FRAME_TIME)
    {
        std::this_thread::sleep_for(FRAME_TIME - elapsed);
    }
  }
  umem.RequestShutdown();
}

int main(int, char**) {
  Radar::Config cfg;

  cfg.entity_config.emplace_back(Radar::EntityConfig{"Enemy", true, true});

  std::cout << "RadarMain\n";
  radar_main(std::move(cfg));
  return 0;
}
