#include "overlay.hpp"

#include <dwmapi.h>
#include <stdio.h>
#include <thread>
#include <chrono>
#include <d3d9.h>
#include <d3d11.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <io.h>


ID3D11Device* Overlay::device = nullptr;
ID3D11DeviceContext* Overlay::device_context = nullptr;
IDXGISwapChain* Overlay::swap_chain = nullptr;
ID3D11RenderTargetView* Overlay::render_targetview = nullptr;
HWND Overlay::overlay = nullptr;
WNDCLASSEX Overlay::wc = { };

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK window_procedure(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        Overlay::DestroyDevice();
        Overlay::DestroyOverlay();
        Overlay::DestroyImGui();
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        Overlay::DestroyDevice();
        Overlay::DestroyOverlay();
        Overlay::DestroyImGui();
        return 0;
    }

    return DefWindowProc(window, msg, wParam, lParam);
}

bool Overlay::CreateDevice()
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = overlay;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;


    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0U,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &sd,
        &swap_chain,
        &device,
        nullptr,
        &device_context
    );

    if (result == DXGI_ERROR_UNSUPPORTED) {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0U,
            nullptr,
            0, D3D11_SDK_VERSION,
            &sd,
            &swap_chain,
            &device,
            nullptr,
            &device_context
        );
        printf("[>>] DXGI_ERROR | Created with D3D_DRIVER_TYPE_WARP\n");
    }

    if (result != S_OK) {
        printf("Device Not Okay\n");
        return false;
    }

    ID3D11Texture2D* back_buffer{ nullptr };
    swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

    if (back_buffer) {
        device->CreateRenderTargetView(back_buffer, nullptr, &render_targetview);
        back_buffer->Release();
        return true;
    }

    printf("[>>] Failed to create Device\n");
    return false;
}

void Overlay::DestroyDevice()
{
    if (device) {
        device->Release();
        device_context->Release();
        swap_chain->Release();
        render_targetview->Release();
    }
    else
        printf("[>>] Device Not Found when Exiting.\n");
}

void Overlay::CreateOverlay()
{
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = window_procedure;
    wc.hInstance = GetModuleHandleA(0);
    wc.lpszClassName = "Window";

    RegisterClassEx(&wc);

    overlay = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        "window",
        WS_POPUP,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL,
        NULL,
        wc.hInstance,
        NULL
    );

    if (overlay == NULL)
        printf("Failed to create Overlay\n");


    SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);
    SetWindowLong(overlay, GWL_EXSTYLE, GetWindowLong(overlay, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);

  



    {
        RECT client_area{};
        RECT window_area{};

        GetClientRect(overlay, &client_area);
        GetWindowRect(overlay, &window_area);

        POINT diff{};
        ClientToScreen(overlay, &diff);

        const MARGINS margins{
            window_area.left + (diff.x - window_area.left),
            window_area.top + (diff.y - window_area.top),
            client_area.right,
            client_area.bottom
        };

        DwmExtendFrameIntoClientArea(overlay, &margins);
    }

    ShowWindow(overlay, SW_SHOW);
    UpdateWindow(overlay);
}

void Overlay::DestroyOverlay()
{
    DestroyWindow(overlay);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
}

bool Overlay::CreateImGui()
{
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(overlay)) {
        printf("Failed ImGui_ImplWin32_Init\n");
        return false;
    }

    if (!ImGui_ImplDX11_Init(device, device_context)) {
        printf("Failed ImGui_ImplDX11_Init\n");
        return false;
    }

    return true;
}

void Overlay::DestroyImGui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Overlay::StartRender()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();


    if (GetAsyncKeyState(VK_INSERT) & 1) {
        RenderMenu = !RenderMenu;

        if (RenderMenu) {
            SetWindowLong(overlay, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);

           

        }
        else {
            SetWindowLong(overlay, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED);

       
        }
    }
}

void Overlay::EndRender()
{
    ImGui::Render();

    float color[4]{ 0, 0, 0, 0 };

    device_context->OMSetRenderTargets(1, &render_targetview, nullptr);
    device_context->ClearRenderTargetView(render_targetview, color);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


    swap_chain->Present(0U, 0U);
}

void Overlay::SetForeground(HWND window)
{
    if (!IsWindowInForeground(window))
        BringToForeground(window);
}