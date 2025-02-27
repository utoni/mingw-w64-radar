#ifndef RADAR_H
#define RADAR_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Radar {
struct Position {
  float x = 0.0f;
  float y = 0.0f;
  float rotation = 0.0f;  // in degree
};

struct Color {
  unsigned char r = 0;
  unsigned char g = 0;
  unsigned char b = 0;
  unsigned char a = 255;
};

struct Entity {
  Position position;
  std::string type;
  Color color;
};

struct EntityConfig {
  std::string type;
  bool show_on_radar;
  bool show_direction;
};

struct Config {
  float range = 300.0f;
  bool sync_rotation = true;
  float manual_rotation = 90.0f;
  std::vector<EntityConfig> entity_config = {};
  float clear_color_rgba[4] = {0.45f, 0.55f, 0.60f, 1.00f};
  unsigned int resolution[2] = {1920, 1080};
  std::string title = "Unnamed Radar";
  bool imgui_dark_style = true;
};

class Drawer {
 public:
  Drawer() = delete;
  Drawer(Config&) = delete;
  Drawer(const Config&) = delete;
  Drawer(Config&& cfg);
  ~Drawer();
  void Init();
  void UpdateLocalPlayer(Position&& pos);
  void UpdateEntity(const std::string& name, Position&& pos);
  void AddEntity(const std::string& name, Entity&& entity);
  void RemoveEntity(const std::string& name);
  bool WindowShouldClose();
  bool WindowPollEvents();
  void Render();

 private:
  struct LocalPlayer {
    Position position;
  };

  void RenderRadarWindow();
  void RenderEntities();

  Config cfg;
  struct GfxImpl;
  std::unique_ptr<GfxImpl> gfx_impl;
  LocalPlayer local_player;
  std::unordered_map<std::string, Entity> entities;
};
};  // namespace Radar

#endif
