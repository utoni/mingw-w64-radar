#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#ifdef __WIN32__
#include <windows.h>
#endif

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "radar.hpp"

using namespace Radar;

struct Drawer::GfxImpl {
  ImVec4 clear_color;
  GLFWwindow* window;
  float last_time;
  float delta_time;
};

static void glfw_error_callback(int error, const char* description) {
  auto error_str = std::string("GLFW Error Callback: ");
  error_str += description;
  throw std::runtime_error(error_str);
}

Drawer::Drawer(Config&& cfg) : gfx_impl(std::make_unique<Drawer::GfxImpl>()) {
  this->cfg = std::move(cfg);
}

Drawer::~Drawer() {}

void Drawer::Init() {
  // GLFW Setup
  if (glfwSetErrorCallback(glfw_error_callback) != NULL)
    throw std::runtime_error("glfwSetErrorCallback failed");
  if (!glfwInit()) throw std::runtime_error("glfwInit failed");

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  gfx_impl->window =
      glfwCreateWindow(cfg.resolution[0], cfg.resolution[1], cfg.title.c_str(), nullptr, nullptr);
  if (gfx_impl->window == nullptr) throw std::runtime_error("glfwCreateWindow failed");
  glfwMakeContextCurrent(gfx_impl->window);
  glfwSwapInterval(1);

  // ImGui Setup
  gfx_impl->clear_color = ImVec4(cfg.clear_color_rgba[0], cfg.clear_color_rgba[1],
                                 cfg.clear_color_rgba[2], cfg.clear_color_rgba[2]);
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  if (cfg.imgui_dark_style)
    ImGui::StyleColorsDark();
  else
    ImGui::StyleColorsLight();
  ImGui_ImplGlfw_InitForOpenGL(gfx_impl->window, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  gfx_impl->last_time = ImGui::GetTime();
  ImGui::SetNextWindowSize({static_cast<float>(cfg.resolution[0]),
                            static_cast<float>(cfg.resolution[1])});
}

void Drawer::UpdateLocalPlayer(Position&& pos) {
  local_player.position = std::move(pos);
}

void Drawer::UpdateEntity(const std::string& name, Position&& pos) {
  auto found = entities.find(name);
  if (found == entities.end()) return;

  entities[name].position = std::move(pos);
}

void Drawer::UpdateEntityRelative(const std::string& name, const Position& pos_rel) {
  auto found = entities.find(name);
  if (found == entities.end()) return;

  entities[name].position += pos_rel;
}

void Drawer::AddEntity(const std::string& name, Entity&& entity) {
  entities[name] = std::move(entity);
}

void Drawer::RemoveEntity(const std::string& name) {
  entities.erase(name);
}

bool Drawer::WindowShouldClose() {
  return glfwWindowShouldClose(gfx_impl->window);
}

bool Drawer::WindowPollEvents() {
  glfwPollEvents();

  float now = ImGui::GetTime();
  gfx_impl->delta_time = now - gfx_impl->last_time;
  gfx_impl->last_time = now;

  int state = glfwGetKey(gfx_impl->window, GLFW_KEY_ESCAPE);
  if (state == GLFW_PRESS) return true;

  return false;
}

void Drawer::Render() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  RenderRadarWindow();

  ImGui::Render();

  int display_w, display_h;
  glfwGetFramebufferSize(gfx_impl->window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  auto& clear_color = gfx_impl->clear_color;
  glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
               clear_color.z * clear_color.w, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glfwSwapBuffers(gfx_impl->window);
}

float Drawer::GetDeltaTime() {
  return gfx_impl->delta_time;
}

void Drawer::RenderRadarWindow() {
  ImGui::Begin("Proximity Radar");

  auto& player = local_player.position;
  ImGui::Text("Position: (%.1f, %.1f)", player.x, player.y);
  ImGui::Text("Rotation: %.1f°", player.rotation);

  ImGui::SliderFloat("Zoom", &cfg.range, 50.0f, 1000.0f, "%.0f");
  ImGui::Checkbox("Sync Rotation with Player", &cfg.sync_rotation);
  if (cfg.sync_rotation)
    ImGui::SliderFloat("Manual Rotation", &cfg.manual_rotation, 0.0f, 360.0f, "%.0f°");

  ImGui::Separator();

  ImGui::Text("Show on Radar");
  for (auto& entity_config : cfg.entity_config) {
    auto chkbx_name = "Show " + entity_config.type;
    ImGui::Checkbox(chkbx_name.c_str(), &entity_config.show_on_radar);
  }

  ImGui::Text("Show Direction");
  for (auto& entity_config : cfg.entity_config) {
    if (!entity_config.show_on_radar) continue;
    auto chkbox_name = "Dir " + entity_config.type;
    ImGui::Checkbox(chkbox_name.c_str(), &entity_config.show_direction);
  }

  ImVec2 winPos = ImGui::GetCursorScreenPos();
  ImVec2 winSize = ImGui::GetContentRegionAvail();
  float radarRadius = std::min(winSize.x, winSize.y) * 0.45f;
  ImVec2 radarCenter = ImVec2(winPos.x + winSize.x * 0.5f, winPos.y + winSize.y * 0.5f);

  RenderEntities();

  ImGui::Dummy(winSize);
  ImGui::End();
}

void Drawer::RenderEntities() {
  ImVec2 winPos = ImGui::GetCursorScreenPos();
  ImVec2 winSize = ImGui::GetContentRegionAvail();
  float radarRadius = std::min(winSize.x, winSize.y) * 0.45f;
  ImVec2 radarCenter = ImVec2(winPos.x + winSize.x * 0.5f, winPos.y + winSize.y * 0.5f);
  auto drawList = ImGui::GetWindowDrawList();

  float radarRotation =
      cfg.sync_rotation ? -1 * (local_player.position.rotation + cfg.manual_rotation) : 0.0f;
  float rotRad = radarRotation * 3.1415926f / 180.0f;
  float scale = radarRadius / cfg.range;

  drawList->AddCircle(radarCenter, radarRadius, IM_COL32(0, 255, 0, 255), 64, 2.0f);

  for (const auto& entity_iter : entities) {
    const auto& entity = entity_iter.second;
    auto found_entity_config =
        std::find_if(cfg.entity_config.cbegin(), cfg.entity_config.cend(),
                     [&entity](const EntityConfig& input) { return entity.type == input.type; });
    // Show entity on radar?
    if (found_entity_config != cfg.entity_config.cend() && !found_entity_config->show_on_radar)
      continue;

    // Calculate entity distance to local player
    float dx = entity.position.x - local_player.position.x;
    float dy = entity.position.y - local_player.position.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist > cfg.range) continue;

    // Scale and rotate entity depending on radar config
    float sx = dx * scale;
    float sy = dy * scale;
    float rx = std::cos(rotRad) * sx - std::sin(rotRad) * sy;
    float ry = std::sin(rotRad) * sx + std::cos(rotRad) * sy;

    // draw entity
    ImVec2 objPos = ImVec2(radarCenter.x + rx, radarCenter.y + ry);
    drawList->AddCircleFilled(
        objPos, 4.0f, IM_COL32(entity.color.r, entity.color.g, entity.color.b, entity.color.a));

    // Show entity direction?
    if (found_entity_config == cfg.entity_config.cend() || found_entity_config->show_direction) {
      float dirRad = (entity.position.rotation + radarRotation) * 3.1415926f / 180.0f;
      ImVec2 tip = ImVec2(objPos.x + 10 * std::cos(dirRad), objPos.y + 10 * std::sin(dirRad));
      drawList->AddLine(objPos, tip,
                        IM_COL32(entity.color.r, entity.color.g, entity.color.b, entity.color.a),
                        2.0f);
    }
  }

  // Player Center
  drawList->AddCircleFilled(radarCenter, 5.0f, IM_COL32(255, 255, 255, 255));
  float playerDirRad =
      (!cfg.sync_rotation ? local_player.position.rotation : -cfg.manual_rotation) * 3.1415926f /
      180.0f;
  ImVec2 dirTip = ImVec2(radarCenter.x + radarRadius * std::cos(playerDirRad),
                         radarCenter.y + radarRadius * std::sin(playerDirRad));
  drawList->AddLine(radarCenter, dirTip, IM_COL32(255, 255, 255, 180), 2.0f);
}
