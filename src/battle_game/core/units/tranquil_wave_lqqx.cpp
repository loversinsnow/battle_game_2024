#include "tranquil_wave_lqqx.h"

#include "battle_game/core/bullets/bullets.h"
#include "battle_game/core/game_core.h"
#include "battle_game/graphics/graphics.h"

namespace battle_game::unit {

namespace {
uint32_t wave_body_model_index = 0xffffffffu;
uint32_t wave_turret_model_index = 0xffffffffu;
}  // namespace

Wave::Wave(GameCore *game_core, uint32_t id, uint32_t player_id)
    : Unit(game_core, id, player_id) {
  if (!~wave_body_model_index) {
    auto mgr = AssetsManager::GetInstance();
    {
      /* Wave Body */
      wave_body_model_index = mgr->RegisterModel(
          {
              {{-0.8f, 0.8f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
              {{-0.8f, -1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
              {{0.8f, 0.8f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
              {{0.8f, -1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
              // distinguish front and back
              {{0.6f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
              {{-0.6f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
          },
          {0, 1, 2, 1, 2, 3, 0, 2, 5, 2, 4, 5});
    }
    {
      /* Wave Turret */
      std::vector<ObjectVertex> turret_vertices;
      std::vector<uint32_t> turret_indices;
      const int precision = 60;
      const float inv_precision = 1.0f / float(precision);
      for (int i = 0; i < precision; i++) {
        auto theta = (float(i) + 0.5f) * inv_precision;
        theta *= glm::pi<float>() * 2.0f;
        auto sin_theta = std::sin(theta);
        auto cos_theta = std::cos(theta);
        turret_vertices.push_back({{sin_theta * 0.5f, cos_theta * 0.5f},
                                   {0.0f, 0.0f},
                                   {0.7f, 0.7f, 0.7f, 1.0f}});
        turret_indices.push_back(i);
        turret_indices.push_back((i + 1) % precision);
        turret_indices.push_back(precision);
      }
      turret_vertices.push_back(
          {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_vertices.push_back(
          {{-0.1f, 0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_vertices.push_back(
          {{0.1f, 0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_vertices.push_back(
          {{-0.1f, 1.2f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_vertices.push_back(
          {{0.1f, 1.2f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_indices.push_back(precision + 1 + 0);
      turret_indices.push_back(precision + 1 + 1);
      turret_indices.push_back(precision + 1 + 2);
      turret_indices.push_back(precision + 1 + 1);
      turret_indices.push_back(precision + 1 + 2);
      turret_indices.push_back(precision + 1 + 3);
      wave_turret_model_index =
          mgr->RegisterModel(turret_vertices, turret_indices);
    }
  }
}



void Wave::Render() {
    battle_game::SetTransformation(position_, rotation_);
    battle_game::SetTexture(0);
    if (game_core_->GetRenderPerspective() == 0) {
        battle_game::SetColor(glm::vec4{0.5f, 1.0f, 0.5f, 1.0f});
    } else if (game_core_->GetRenderPerspective() == player_id_) {
        battle_game::SetColor(glm::vec4{1.0f, 1.0f, 0.5f, 0.5f});
    } else {
        battle_game::SetColor(glm::vec4{1.0f, 0.5f, 0.5f, 1.0f});
    }
    
    battle_game::DrawModel(wave_body_model_index);
    battle_game::SetRotation(point_rotation_);
    battle_game::DrawModel(wave_turret_model_index);
}

float Wave::BasicMaxHealth() const {
    return 40.0f;
}

bool Wave::IsHit(glm::vec2 position) const {
  position = WorldToLocal(position);
  float p_x = game_core_->RandomFloat();
  float p_y = game_core_->RandomFloat();
  float q_x = game_core_->RandomFloat();
  float q_y = game_core_->RandomFloat();
  return position.x > -p_x * 0.8f && position.x < q_x * 0.8f && position.y > -p_y &&
         position.y < q_y && position.x + position.y < (std::min(p_x,q_x) * 0.8f + std::max(p_y,q_y)) &&
         position.y - position.x < std::abs(std::max(p_x,q_x) + std::min(p_y,q_y) * 0.8) ;
};

void Wave::Update() {
  WaveMove(3.0f * (1.0f + game_core_->RandomFloat()), glm::radians(180.0f));
  PointMe();
  Fire();
}

void Wave::WaveMove(float move_speed, float rotate_angular_speed) {
  auto player = game_core_->GetPlayer(player_id_);
  if (player) {
    auto &input_data = player->GetInputData();
    glm::vec2 offset{0.0f};
    if (input_data.key_down[GLFW_KEY_W]) {
      offset.y += 1.0f;
    }
    if (input_data.key_down[GLFW_KEY_S]) {
      offset.y -= 1.0f;
    }
    float speed = move_speed * GetSpeedScale();
    offset *= kSecondPerTick * speed;
    auto new_position =
        position_ + glm::vec2{glm::rotate(glm::mat4{1.0f}, rotation_,
                                          glm::vec3{0.0f, 0.0f, 1.0f}) *
                              glm::vec4{offset, 0.0f, 0.0f}};
    if (!game_core_->IsBlockedByObstacles(new_position)) {
      game_core_->PushEventMoveUnit(id_, new_position);
    }
    float rotation_offset = 0.0f;
    if (input_data.key_down[GLFW_KEY_A]) {
      rotation_offset += 1.0f;
    }
    if (input_data.key_down[GLFW_KEY_D]) {
      rotation_offset -= 1.0f;
    }
    rotation_offset *= kSecondPerTick * rotate_angular_speed * GetSpeedScale();
    game_core_->PushEventRotateUnit(id_, rotation_ + rotation_offset);
  }
}
void Wave::PointMe() {
  auto player = game_core_->GetPlayer(player_id_);
  if (player) {
    auto &input_data = player->GetInputData();
    auto diff = input_data.mouse_cursor_position - position_;
    if (glm::length(diff) < 1e-4) {
      point_rotation_ = rotation_;
    } else {
      point_rotation_ = std::atan2(diff.y, diff.x) - glm::radians(90.0f);
    }
  }
}

void Wave::Fire() {
  if (fire_count_down_ == 0) {
    auto player = game_core_->GetPlayer(player_id_);
    if (player) {
      auto &input_data = player->GetInputData();
      if (input_data.mouse_button_down[GLFW_MOUSE_BUTTON_LEFT]) {
        float x = glm::radians(37.0f);
        float lim = glm::radians(180.0f);
        auto velocity = Rotate(glm::vec2{0.0f, 20.0f}, point_rotation_);
        GenerateBullet<bullet::CannonBall>(
            position_ + Rotate({0.0f, 1.2f}, point_rotation_),
            point_rotation_, GetDamageScale(), velocity);
        auto velocity1 = Rotate(glm::vec2{0.0f, 20.0f}, (point_rotation_ + x) > 2.0 * lim ? (point_rotation_ + x) :(point_rotation_ + x - 2 * lim));
        GenerateBullet<bullet::CannonBall>(
            position_ + Rotate({0.0f, 1.2f}, (point_rotation_ + x) <= 2.0 * lim ? (point_rotation_ + x) : (point_rotation_ + x - 2 * lim)),
            (point_rotation_ + x) > 2.0 * lim ? (point_rotation_ + x) : (point_rotation_ + x - 2 * lim), GetDamageScale() * 0.8, velocity1);
        auto velocity2 = Rotate(glm::vec2{0.0f, 20.0f}, (point_rotation_ - x) >= 0 ? (point_rotation_ - x) :(point_rotation_ - x + 2 * lim));
        GenerateBullet<bullet::CannonBall>(
            position_ + Rotate({0.0f, 1.2f}, (point_rotation_ - x) >= 0 ? (point_rotation_ - x) :(point_rotation_ - x + 2 * lim)),
            (point_rotation_ - x) >= 0 ? (point_rotation_ - x) :(point_rotation_ - x + 2 * lim), GetDamageScale() *0.8, velocity2);
        fire_count_down_ = kTickPerSecond / 3; 
      }
    }
  }
  if (fire_count_down_) {
    fire_count_down_--;
  }
}


const char *Wave::UnitName() const {
  return "Tranquil Wave";
}

const char *Wave::Author() const {
  return "lqqx";
}

} // namespace battle_game::unit
