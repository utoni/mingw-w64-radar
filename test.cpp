#include <radar.hpp>

#include <random>

static void simple_test() {
  Radar::Config cfg;
  cfg.entity_config.emplace_back(Radar::EntityConfig{"Enemy", true, true});
  Radar::Drawer drawer(std::move(cfg));

  drawer.AddEntity("some_nickname", Radar::Entity{{100.0f, 0.0f, 0.0f}, "Enemy", {255, 0, 0, 255}});
  drawer.AddEntity("another_nickname", Radar::Entity{{300.0f, 0.0f, 90.0f}, "Enemy", {255, 0, 0, 255}});
  drawer.AddEntity("rotating_nobody", Radar::Entity{{-100.0f, -100.0f, 180.0f}, "Neutral", {255, 255, 0, 255}});

  std::random_device rd;
  std::uniform_int_distribution<> distrib(1, 10);
  float time_passed = 0.0f;
  drawer.Init();
  while (!drawer.WindowShouldClose()) {
    if (drawer.WindowPollEvents()) break;
    drawer.Render();
    time_passed += drawer.GetDeltaTime();
    if (time_passed > 0.08f) {
      const auto rng = distrib(rd);
      Radar::Position rel_local_pos;
      Radar::Position rel_enemy_pos;
      Radar::Position rel_neutral_pos;

      if (rng < 3) {
        rel_local_pos.rotation += 1.0f;
        rel_local_pos.x += 1.5f;
      }
      if (rng >= 3) {
        rel_local_pos.rotation -= 1.0f;
      }

      switch (rng) {
        case 1:
        case 9:
        case 10:
          rel_enemy_pos.rotation += 5.0f;
          break;
        case 2:
          rel_enemy_pos.rotation -= 5.0f;
          break;
        case 3:
          rel_enemy_pos.x += 1.5f;
          break;
        case 4:
        case 7:
          rel_enemy_pos.x -= 1.5f;
          break;
        case 5:
        case 8:
          rel_enemy_pos.y += 1.5f;
          break;
        case 6:
          rel_enemy_pos.y -= 1.5f;
          break;
      }

      rel_neutral_pos.rotation += 5.0f;

      drawer.UpdateLocalPlayerRelative(rel_local_pos);
      drawer.UpdateEntityRelative("some_nickname", rel_enemy_pos);
      drawer.UpdateEntityRelative("rotating_nobody", rel_neutral_pos);

      time_passed = 0.0f;
    }
  }
}

int main(int, char**) {
  simple_test();
}
