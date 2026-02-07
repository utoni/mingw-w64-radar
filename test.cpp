#include <radar.hpp>

#include <random>

static void simple_test() {
  Radar::Config cfg;
  cfg.entity_config.emplace_back(Radar::EntityConfig{"Enemy", true, true});
  Radar::Drawer drawer(std::move(cfg));

  drawer.AddEntity("some_nickname", Radar::Entity{{100, 0, 0.0f}, "Enemy", {255, 0, 0, 255}});
  drawer.AddEntity("another_nickname", Radar::Entity{{300, 0, 90.0f}, "Enemy", {255, 0, 0, 255}});

  std::random_device rd;
  std::uniform_int_distribution<> distrib(1, 10);
  float time_passed = 0.0f;
  drawer.Init();
  while (!drawer.WindowShouldClose()) {
    if (drawer.WindowPollEvents()) break;
    drawer.Render();
    time_passed += drawer.GetDeltaTime();
    if (time_passed > 0.15f) {
      const auto rng = distrib(rd);
      Radar::Position rel_pos;
      switch (rng) {
        case 1:
        case 9:
        case 10:
          rel_pos.rotation += 5.0f;
          break;
        case 2:
          rel_pos.rotation -= 5.0f;
          break;
        case 3:
          rel_pos.x += 1.5f;
          break;
        case 4:
        case 7:
          rel_pos.x -= 1.5f;
          break;
        case 5:
        case 8:
          rel_pos.y += 1.5f;
          break;
        case 6:
          rel_pos.y -= 1.5f;
          break;
      }

      drawer.UpdateEntityRelative("some_nickname", rel_pos);

      time_passed = 0.0f;
    }
  }
}

int main(int, char**) {
  simple_test();
}
