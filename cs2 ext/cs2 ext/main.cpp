#include "offsets.h"

#include "Windows.h"
#include "iostream"
#include "Memory.h"
#include "overlay.hpp"
#include <cmath>

struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    float Magnitude() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vector3 Normalize() const {
        float mag = Magnitude();
        return (mag > 0) ? Vector3(x / mag, y / mag, z / mag) : Vector3(0, 0, 0);
    }

    float Dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vector3 Cross(const Vector3& other) const {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    Vector3 operator/(float scalar) const {
        return (scalar != 0) ? Vector3(x / scalar, y / scalar, z / scalar) : Vector3(0, 0, 0);
    }

    bool operator==(const Vector3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Vector3& other) const {
        return !(*this == other);
    }
};

using namespace std;

Memory mem("cs2.exe");
uintptr_t baseAddress = mem.GetModuleBase("client.dll");

template <typename T>
constexpr T clamp(T value, T min, T max) {
    return (value < min) ? min : (value > max) ? max : value;
}

struct CSName {
    char name[228];
};

struct view_matrix_t {
    float matrix[4][4];
};

struct Vector2 {
    float x, y;

    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}

    operator ImVec2() const {
        return ImVec2(x, y);
    }

    static Vector2 FromImVec2(const ImVec2& vec) {
        return Vector2(vec.x, vec.y);
    }

    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    float Length() const {
        return sqrt(x * x + y * y);
    }

    Vector2 Normalize() const {
        float len = Length();
        if (len > 0)
            return Vector2(x / len, y / len);
        return *this;
    }
};

Vector3 worldToScreen(const view_matrix_t& matrix, const Vector3& worldPos) {
    float _x = matrix.matrix[0][0] * worldPos.x + matrix.matrix[0][1] * worldPos.y + matrix.matrix[0][2] * worldPos.z + matrix.matrix[0][3];
    float _y = matrix.matrix[1][0] * worldPos.x + matrix.matrix[1][1] * worldPos.y + matrix.matrix[1][2] * worldPos.z + matrix.matrix[1][3];
    float _w = matrix.matrix[3][0] * worldPos.x + matrix.matrix[3][1] * worldPos.y + matrix.matrix[3][2] * worldPos.z + matrix.matrix[3][3];

    float inv_w = 1.f / _w;
    _x *= inv_w;
    _y *= inv_w;

    int screen_x = static_cast<int>((0.5f * _x + 0.5f) * static_cast<float>(GetSystemMetrics(SM_CXSCREEN)));
    int screen_y = static_cast<int>((-0.5f * _y + 0.5f) * static_cast<float>(GetSystemMetrics(SM_CYSCREEN)));

    return Vector3(static_cast<float>(screen_x), static_cast<float>(screen_y), _w);
}

namespace config {
    bool isAimbotActive = false;
    float fov = 90.0f;
    float smoothingFactor = 4.0f;
    Vector3 aimbotTarget = { 0, 0, 0 };

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    Vector2 screenCenter = { static_cast<float>(screenWidth) / 2.0f, static_cast<float>(screenHeight) / 2.0f };

    bool aimbot = false;
    bool Box = false;
    bool FilledBox = false;
    bool HealthBar = false;
    bool Name = false;
    bool snapline = false;
    bool FovCircle = false;
}
void Loop(ImDrawList* drawList) {
    view_matrix_t viewMatrix = mem.ReadMemory<view_matrix_t>(baseAddress + offsets::dwViewMatrix);
    uintptr_t localplayer = mem.ReadMemory<uintptr_t>(baseAddress + offsets::dwLocalPlayerPawn);

    config::isAimbotActive = GetAsyncKeyState(VK_LSHIFT) & 0x8000;
    drawList->AddCircle(config::screenCenter, config::fov, IM_COL32(255, 255, 255, 150), 50, 2.0f);

    float bestDist = FLT_MAX;
    Vector3 bestTarget = { 0, 0, 0 };

    for (int i = 0; i < 64; i++) {
        uintptr_t entityList = mem.ReadMemory<uintptr_t>(baseAddress + offsets::dwEntityList);
        uintptr_t listEntry1 = mem.ReadMemory<uintptr_t>(entityList + ((8 * (i & 0x7FFF) >> 9) + 16));
        if (!listEntry1) continue;

        uintptr_t playerController = mem.ReadMemory<uintptr_t>(listEntry1 + 120 * (i & 0x1FF));
        if (!playerController) continue;

        uint32_t playerPawnHandle = mem.ReadMemory<uint32_t>(playerController + offsets::m_hPlayerPawn);
        if (!playerPawnHandle) continue;

        uintptr_t listEntry2 = mem.ReadMemory<uintptr_t>(entityList + 0x8 * ((playerPawnHandle & 0x1FFF) >> 9) + 16);
        if (!listEntry2) continue;

        uintptr_t entityPawn = mem.ReadMemory<uintptr_t>(listEntry2 + 120 * (playerPawnHandle & 0x1FF));
        if (!entityPawn || entityPawn == localplayer) continue;

        int health = mem.ReadMemory<int>(entityPawn + offsets::m_iHealth);
        if (health <= 0) continue;

        int teamnum = mem.ReadMemory<int>(entityPawn + offsets::m_iTeamNum);
        uintptr_t gameScene = mem.ReadMemory<uintptr_t>(entityPawn + offsets::m_pGameSceneNode);
        uintptr_t boneArray = mem.ReadMemory<uintptr_t>(gameScene + offsets::m_modelState + 0x80);

        Vector3 Headpos = mem.ReadMemory<Vector3>(boneArray + 6 * 32);
        Vector3 FeetPos = mem.ReadMemory<Vector3>(entityPawn + offsets::m_vOldOrigin);

        uintptr_t NameAddr = mem.ReadMemory<uintptr_t>(playerController + offsets::m_sSanitizedPlayerName);
        CSName Name = mem.ReadMemory<CSName>(NameAddr);

        Vector3 headScreen = worldToScreen(viewMatrix, Headpos);
        Vector3 feetScreen = worldToScreen(viewMatrix, FeetPos);
        if (headScreen.z < 0.01f || feetScreen.z < 0.01f) continue;

        float distance = sqrt(pow(headScreen.x - config::screenCenter.x, 2) + pow(headScreen.y - config::screenCenter.y, 2));

        {
            int height = abs(feetScreen.y - headScreen.y) * 1.2f;
            int width = height / 1.8f;
            int x = feetScreen.x - width / 2;
            int y = headScreen.y - (height * 0.1f);

            if (config::Box)
                drawList->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), IM_COL32(255, 255, 255, 255), 3.0f);

            if (config::FilledBox)
                drawList->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x + width, y + height),
                    IM_COL32(255, 0, 0, 50), IM_COL32(255, 0, 255, 50),
                    IM_COL32(255, 0, 255, 50), IM_COL32(255, 0, 0, 50));

            if (config::HealthBar) {
                float healthRatio = clamp(health / 100.0f, 0.0f, 1.0f);
                int healthHeight = static_cast<int>(healthRatio * height);
                int healthBarX = x - 6;
                int healthBarY = y + (height - healthHeight);

                drawList->AddRectFilled(ImVec2(healthBarX, y), ImVec2(healthBarX + 4, y + height), IM_COL32(50, 50, 50, 200));
                drawList->AddRectFilled(ImVec2(healthBarX, healthBarY), ImVec2(healthBarX + 4, y + height), IM_COL32(0, 255, 0, 255));
            }

            if (config::Name)
                drawList->AddText(ImVec2(x + width / 2 - ImGui::CalcTextSize(Name.name).x / 2, y - 15),
                    IM_COL32(255, 255, 255, 255), Name.name);

            if (config::snapline)
                drawList->AddLine(ImVec2(config::screenWidth / 2, config::screenHeight), ImVec2(feetScreen.x, feetScreen.y),
                    IM_COL32(255, 255, 255, 150), 1.5f);
        }

        if (config::isAimbotActive && distance < config::fov && config::aimbot) {
            if (distance < bestDist) {
                bestDist = distance;
                bestTarget = headScreen;
            }
        }
    }

    if (config::isAimbotActive && bestTarget.x != 0 && bestTarget.y != 0) {
        int aimX = static_cast<int>(bestTarget.x - config::screenCenter.x);
        int aimY = static_cast<int>(bestTarget.y - config::screenCenter.y);

        aimX = static_cast<int>(aimX / config::smoothingFactor);
        aimY = static_cast<int>(aimY / config::smoothingFactor);

        aimX = clamp(aimX, -10, 10);
        aimY = clamp(aimY, -10, 10);

        SetCursorPos(config::screenCenter.x + aimX, config::screenCenter.y + aimY);
        mouse_event(MOUSEEVENTF_MOVE, aimX, aimY, 0, 0);
    }
}

void Overlay::Render() {
    static int selectedTab = 0;
    ImGui::SetNextWindowSize(ImVec2(450, 350), ImGuiCond_Always);
    ImGui::Begin("##CS2External", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
    ImGui::BeginChild("TitleBar", ImVec2(0, 40), true);
    ImGui::SetCursorPos(ImVec2(15, 10));
    ImGui::TextColored(ImVec4(0.7f, 0.3f, 1.0f, 1.0f), "Impulsive");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    const char* tabs[] = { "Combat", "Visuals", "Misc" };
    ImVec2 tabSize = ImVec2(140, 30);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.2f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.3f, 1.0f, 1.0f));

    ImGui::BeginChild("TabBar", ImVec2(0, 35), false);
    for (int i = 0; i < 3; i++) {
        bool isActive = (i == selectedTab);

        if (isActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 1.0f, 1.0f));

        if (ImGui::Button(tabs[i], tabSize)) selectedTab = i;

        if (isActive) ImGui::PopStyleColor();

        ImGui::SameLine();
    }
    ImGui::EndChild();

    ImGui::PopStyleColor(3);

    ImGui::Separator();

    switch (selectedTab) {
    case 0: {
        ImGui::Checkbox("Enable Aimbot", &config::aimbot);
        ImGui::Text("FOV: %.1f", config::fov);
        ImGui::SliderFloat("##FOV", &config::fov, 1.0f, 180.0f, "%.1f");

        ImGui::Text("Smoothing: %.1f", config::smoothingFactor);
        ImGui::SliderFloat("##Smoothing", &config::smoothingFactor, 1.0f, 10.0f, "%.1f");

        break;
    }
    case 1: {

        ImGui::Checkbox("Boxes", &config::Box);
        ImGui::Checkbox("Filled Box", &config::FilledBox);
        ImGui::Checkbox("Health Bar", &config::HealthBar);
        ImGui::Checkbox("Names", &config::Name);
        ImGui::Checkbox("Snaplines", &config::snapline);
        ImGui::Checkbox("FOV Circle", &config::FovCircle);

        break;
    }
    case 2: {

        break;
    }
    }

    ImGui::End();
}

int main() {

    if (baseAddress == 0)
    {
        cout << "CS2 Not Found!" << endl;
    }
    else
    {
        cout << "Attached!" << endl;
    }

    overlay.shouldRun = true;
    overlay.RenderMenu = false;
    overlay.CreateOverlay();
    overlay.CreateDevice();
    overlay.CreateImGui();
    overlay.SetForeground(GetConsoleWindow());
    while (overlay.shouldRun) {
        overlay.StartRender();

        if (overlay.RenderMenu) {
            overlay.Render();
        }

        Loop(ImGui::GetBackgroundDrawList());

        overlay.EndRender();
    }
    overlay.DestroyImGui();
    overlay.DestroyDevice();
    overlay.DestroyOverlay();

}