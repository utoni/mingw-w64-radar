#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifdef USE_EA_STL
#include <EASTL/utility.h>

using namespace eastl;
#else
#include <utility>

using namespace std;
#endif

extern "C" {
  struct Entity {
    char name[64];
    float pos_x, pos_y, pos_z;
    float pitch, yaw;
  };

  struct GlobalGame {
    const unsigned char magic[8];
    unsigned int max_entites;
    unsigned int amount_of_entites;
    struct Entity * entities;
  };
};

static struct GlobalGame g_Game = {
  {0xCA, 0xFE, 0xCA, 0xFE, 0xBE, 0xEF, 0xBE, 0xEF},
  0, 0, nullptr
};

static void init_entites(unsigned int max_entites)
{
  g_Game.max_entites = max_entites;
  g_Game.amount_of_entites = 0;
  g_Game.entities = reinterpret_cast<struct Entity *>(calloc(max_entites, sizeof(struct Entity)));
}

static void add_entity(Entity && ent)
{
  if (g_Game.amount_of_entites == g_Game.max_entites)
    return;
  g_Game.entities[g_Game.amount_of_entites++] = std::move(ent);
}

static auto get_entity(size_t index) -> Entity*
{
  if (index >= g_Game.amount_of_entites)
    return nullptr;
  return &g_Game.entities[index];
}

int main()
{
  static constexpr auto max_entities = 32u;

  init_entites(max_entities);

  {
    Entity local_player = {};
    strncpy(&local_player.name[0], "LocalPlayer", sizeof(local_player.name));
    local_player.pos_x = 0.0f;
    local_player.pos_y = 0.0f;
    local_player.pos_z = 0.0f;
    add_entity(std::move(local_player));
  }
  {
    Entity smth = {};
    strncpy(&smth.name[0], "Something", sizeof(smth.name));
    smth.pos_x = 100.0f;
    smth.pos_y = 200.0f;
    smth.pos_z = 50.0f;
    add_entity(std::move(smth));
  }
  {
    Entity anot = {};
    strncpy(&anot.name[0], "Another", sizeof(anot.name));
    anot.pos_x = 0.0f;
    anot.pos_y = -50.0f;
    anot.pos_z = 200.0f;
    add_entity(std::move(anot));
  }

  printf("sizeof(Entity)=%zu entities=%p [1]=%p\n",
         sizeof(Entity), (void*)g_Game.entities, (void*)&g_Game.entities[1]);

  while (1) {
    {
      auto local_player = get_entity(0);
      local_player->pos_x++;
      local_player->pos_y++;
      local_player->yaw -= 5.0f;
    }
    float smth_x, anot_y;
    {
      auto smth = get_entity(1);
      smth->yaw -= 1.0f;
      smth_x = ++smth->pos_x;
    }
    {
      auto anot = get_entity(2);
      anot->yaw += 10.0f;
      anot_y = ++anot->pos_y;
    }
    printf("[Tick!][smth x: %0.0f][anot y: %0.0f]\n",
           smth_x, anot_y);
    Sleep(1000);
  }

  return 0;
}
