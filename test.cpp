#include <radar.hpp>

#include <iostream>
#include <random>
#include <string>

#include "imgui.h"

static bool extended_tests = false;
static bool extended_tests_rng = true;

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

    if (extended_tests) {
      drawer.NewFrame();
      drawer.RenderRadarWindow();
      ImGui::Begin("Test Options");
      ImGui::Checkbox("Randomize", &extended_tests_rng);
      if (!extended_tests_rng) {
        auto pos = drawer.GetEntity("some_nickname");
        if (pos) {
          ImGui::SliderFloat("some_nickname x", &pos->x, -1000.0f, 1000.0f);
          ImGui::SliderFloat("some_nickname y", &pos->y, -1000.0f, 1000.0f);
          ImGui::SliderFloat("some_nickname rotation", &pos->rotation, 0.0f, 360.0f);
          drawer.UpdateEntity("some_nickname", std::move(*pos));
        }

        pos = drawer.GetEntity("another_nickname");
        if (pos) {
          ImGui::SliderFloat("another_nickname x", &pos->x, -1000.0f, 1000.0f);
          ImGui::SliderFloat("another_nickname y", &pos->y, -1000.0f, 1000.0f);
          ImGui::SliderFloat("another_nickname rotation", &pos->rotation, 0.0f, 360.0f);
          drawer.UpdateEntity("another_nickname", std::move(*pos));
        }

        pos = drawer.GetEntity("rotating_nobody");
        if (pos) {
          ImGui::SliderFloat("rotating_nobody x", &pos->x, -1000.0f, 1000.0f);
          ImGui::SliderFloat("rotating_nobody y", &pos->y, -1000.0f, 1000.0f);
          ImGui::SliderFloat("rotating_nobody rotation", &pos->rotation, 0.0f, 360.0f);
          drawer.UpdateEntity("rotating_nobody", std::move(*pos));
        }
      }
      ImGui::End();
      drawer.EndFrame();
      if (!extended_tests_rng)
        continue;
    } else {
      drawer.Render();
    }

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

int main(int argc, char** argv) {
  static constexpr auto ets = "extended-tests";
  if (argc == 2) {
    const auto first_arg = std::string(argv[1]);
    extended_tests = (first_arg == ets);
  } else if (argc > 0) {
    std::cout << "Usage: " << argv[0] << " [" << ets << "]\n";
  } else return 1;

  simple_test();
  return 0;
}
