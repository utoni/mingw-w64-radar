#include <radar.hpp>

static void simple_test() {
  Radar::Config cfg;
  cfg.entity_config.emplace_back(Radar::EntityConfig{"Enemy", true, true});
  Radar::Drawer drawer(std::move(cfg));

  drawer.AddEntity("some_nickname", Radar::Entity{{100, 0, 0.0f}, "Enemy", {255, 0, 0, 255}});
  drawer.AddEntity("another_nickname", Radar::Entity{{300, 0, 90.0f}, "Enemy", {255, 0, 0, 255}});

  drawer.Init();
  while (!drawer.WindowShouldClose()) {
    if (drawer.WindowPollEvents()) break;
    drawer.Render();
  }
}

int main(int, char**) {
  simple_test();
}
