#include <radar.hpp>

#include "common.hpp"

#define BUILD_USERMODE 1

#include <iostream>
#include <ipc.hpp>

void radar_main(Radar::Config&& initial_config) {
  IPC::UserSharedMemory umem;
  if (!umem.Allocate({sizeof(radar_buffer_data), COMMON_BUFFER_SLOTS},
                     {sizeof(radar_ringbuffer_data), COMMON_RINGBUFFER_SLOTS}))
  {
    std::cerr << "\tFailed InitSharedMemory\n";
  }

  Radar::Drawer drawer(std::move(initial_config));
  float time_passed = 0.0f;

  drawer.Init();
  while (!drawer.WindowShouldClose()) {
    if (drawer.WindowPollEvents()) break;

    if (umem.OpenedByKernel()) {
      auto ret = umem.ReadBufferData([](void* data, uint64_t) {
        auto slot_data = reinterpret_cast<struct radar_buffer_data*>(data);
        for (auto i = 0u; i < slot_data->count; ++i) {
          printf("[%u] Entity: %.*s\n", i, (int)sizeof(slot_data->entities[i].name),
                 slot_data->entities[i].name);
        }
      });
      if (!ret) {
        std::cerr << "IPC: Read Buffer failure!\n";
        break;
      }

      ret = umem.ReadRingbufferData([&drawer](void* data, uint64_t) {
        auto slot_data = reinterpret_cast<struct radar_ringbuffer_data*>(data);
        std::string msg(slot_data->msg, sizeof(slot_data->msg));
        drawer.AddLogMsg(std::move(msg));
      });
      if (!ret) {
        std::cerr << "IPC: Read RingBuffer failure!\n";
        break;
      }
    }

    drawer.NewFrame();
    drawer.RenderLogWindow();
    drawer.RenderRadarWindow();
    drawer.EndFrame();

    time_passed += drawer.GetDeltaTime();
    if (time_passed > 0.08f) {
      // Nothing to be done at 60hz right now
      time_passed = 0.0f;
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
