﻿#include "UI.h"
#include "Drawing.h"
#include <dwmapi.h>
#include <Windows.h>
#include <iostream>
// font includes
#include "fonts\\FiraCode-Regular.h" // Contains FiraCode_Regular_ttf and FiraCode_Regular_ttf_len
#include "fonts\\Inter.h"            // Contains Inter_ttf and Inter_ttf_len
#include "fonts/font_globals.h"

// from fonts\Inter.h
extern unsigned char Inter_ttf[];
extern unsigned int  Inter_ttf_len;
static ImFont* interFont = nullptr;
ImFont* iconFont = nullptr;

void SetupFonts()
{
    ImGuiIO& io = ImGui::GetIO();

    // configure Inter so ImGui won't free your array
    ImFontConfig cfg{};
    cfg.FontDataOwnedByAtlas = false;

    // load it from your embedded bytes
    interFont = io.Fonts->AddFontFromMemoryTTF(
        Inter_ttf,          // pointer to your font array
        Inter_ttf_len,      // its size
        18.0f,              // pixel size
        &cfg                // <— pass your config here
    );

    iconFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguiemj.tff", 16.0f, nullptr, io.Fonts->GetGlyphRangesDefault());

}

ID3D11Device* UI::pd3dDevice = nullptr;
ID3D11DeviceContext* UI::pd3dDeviceContext = nullptr;
IDXGISwapChain* UI::pSwapChain = nullptr;
ID3D11RenderTargetView* UI::pMainRenderTargetView = nullptr;

HMODULE UI::hCurrentModule = nullptr;


bool UI::CreateDeviceD3D(HWND hWnd)
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
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const UINT createDeviceFlags = 0;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void UI::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer != nullptr)
    {
        pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pMainRenderTargetView);
        pBackBuffer->Release();
    }
}

void UI::CleanupRenderTarget()
{
    if (pMainRenderTargetView)
    {
        pMainRenderTargetView->Release();
        pMainRenderTargetView = nullptr;
    }
}

void UI::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (pSwapChain)
    {
        pSwapChain->Release();
        pSwapChain = nullptr;
    }

    if (pd3dDeviceContext)
    {
        pd3dDeviceContext->Release();
        pd3dDeviceContext = nullptr;
    }

    if (pd3dDevice)
    {
        pd3dDevice->Release();
        pd3dDevice = nullptr;
    }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

LRESULT WINAPI UI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);

        }
        break;

    default:
        break;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

void UI::Render()
{
    // Wait for Destiny 2 to show up in a real position (not the minimized placeholder at -32000,-32000)
    HWND destinyWindow = nullptr;
    RECT destinyRect{};
    AllocConsole();       // Redirect stdout and stderr to console
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    std::cout << "Waiting for Destiny 2 to start..." << std::endl;
    while (true)
    {
        AllocConsole();       // Redirect stdout and stderr to console
        FILE* stream;
        freopen_s(&stream, "CONOUT$", "w", stdout);
        freopen_s(&stream, "CONOUT$", "w", stderr);
        HWND cand = FindWindow(NULL, L"Destiny 2");
        if (cand && GetWindowRect(cand, &destinyRect))
        {
            // Skip the “minimized” placeholder
            if (destinyRect.left == -32000 && destinyRect.top == -32000)
            {
                Sleep(500);
                continue;
            }

            // Found a real, on-screen window!
            destinyWindow = cand;
            break;
        }
        Sleep(500);
    }
    std::cout << "[+] Destiny 2 found!\n";
    FreeConsole();
    HWND hwnd = FindWindowW(L"ConsoleWindowClass", nullptr);
    if (hwnd) PostMessageW(hwnd, WM_CLOSE, 0, 0);

    ImGui_ImplWin32_EnableDpiAwareness();
    
    // Now use destinyRect for creating your overlay:
    int destinyX = destinyRect.left;
    int destinyY = destinyRect.top;
    int destinyWidth = destinyRect.right - destinyRect.left;
    int destinyHeight = destinyRect.bottom - destinyRect.top;

    std::cout
        << "[Overlay-Create] destinyWindow=" << destinyWindow
        << " Pos=(" << destinyX << "," << destinyY << ")"
        << " Size=" << destinyWidth << "×" << destinyHeight
        << std::endl;
    
    // Create overlay window class
    const WNDCLASSEXW wc = { 
        sizeof(WNDCLASSEXW), 
        CS_CLASSDC, 
        WndProc, 
        0L, 
        0L, 
        GetModuleHandle(nullptr), 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        L"IUIC_Overlay", 
        nullptr 
    };
    ::RegisterClassExW(&wc);
    
    // Create transparent overlay window that matches Destiny 2 window bounds
    hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED, 
        wc.lpszClassName, 
        L"Hatemob", 
        WS_POPUP, 
        destinyX, destinyY, 
        destinyWidth, destinyHeight, 
        nullptr, 
        nullptr, 
        wc.hInstance, 
        nullptr
    );

    // Set up layered window attributes for transparency
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS margin = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margin);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    SetupFonts();
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines;

    // Configure ImGui for overlay use (disable viewports for external overlay)
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Note: Removed ViewportsEnable for external overlay compatibility

    ImGui::StyleColorsDark();

    // Apply custom styling
    ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(0.97f, 0.74f, 0.02f, 1.0f);
    ImGui::GetStyle().WindowRounding = 15.0f;

    // Handle DPI scaling
    const HMONITOR monitor = MonitorFromWindow(destinyWindow, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = {};
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &info);
    const int monitor_height = info.rcMonitor.bottom - info.rcMonitor.top;

    if (monitor_height > 1080)
    {
        const float fScale = 1.3f;
        ImFontConfig cfg;
        cfg.SizePixels = 13 * fScale;
        ImGui::GetIO().Fonts->AddFontDefault(&cfg);
    }

    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(pd3dDevice, pd3dDeviceContext);

    // Transparent clear color for overlay
    const ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);    bool bDone = false;
    bool overlayEnabled = true;
    RECT lastDestinyRect = destinyRect;
    
    while (!bDone)
    {
        // Check if Destiny 2 is still running
        if (!IsWindow(destinyWindow))
        {
            destinyWindow = FindWindow(NULL, L"Destiny 2");
            if (!destinyWindow)
                break; // Exit if Destiny 2 is closed
        }

        // Update overlay position to match Destiny 2 window only if it changed
        RECT currentDestinyRect;
        if (GetWindowRect(destinyWindow, &currentDestinyRect))
        {
            // Only update if window position/size changed
            if (memcmp(&lastDestinyRect, &currentDestinyRect, sizeof(RECT)) != 0)
            {
                int currentX = currentDestinyRect.left;
                int currentY = currentDestinyRect.top;
                int currentWidth = currentDestinyRect.right - currentDestinyRect.left;
                int currentHeight = currentDestinyRect.bottom - currentDestinyRect.top;
                
                // Update overlay window position and size to match Destiny 2
                SetWindowPos(hwnd, HWND_TOPMOST, currentX, currentY, currentWidth, currentHeight, 
                            SWP_NOACTIVATE/* | SWP_NOZORDER*/);

                RECT overlayRect;
                if (GetWindowRect(hwnd, &overlayRect)) {
                    std::cout
                        << "[Overlay] Pos: ("
                        << overlayRect.left << "," << overlayRect.top
                        << ") Size: "
                        << (overlayRect.right - overlayRect.left)
                        << "×"
                        << (overlayRect.bottom - overlayRect.top)
                        << std::endl;
                }
                
                lastDestinyRect = currentDestinyRect;
            }
        }

        // Handle overlay toggle (INSERT key)
        if (GetAsyncKeyState(VK_INSERT) & 1)
        {
            overlayEnabled = !overlayEnabled;
        }

        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                bDone = true;
        }

        if (GetAsyncKeyState(VK_END) & 1)
            bDone = true;

        if (bDone)
            break;

        // Handle window transparency based on overlay state
        if (overlayEnabled)
        {
            SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST);
        }
        else
        {
            SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        Drawing::Poll(); // fix for keybinds not working

        if (overlayEnabled)
        {
            ImGui::PushFont(interFont);
            Drawing::Draw();
            ImGui::PopFont();
        }

        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        pd3dDeviceContext->OMSetRenderTargets(1, &pMainRenderTargetView, nullptr);
        pd3dDeviceContext->ClearRenderTargetView(pMainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        pSwapChain->Present(1, 0);

#ifndef _WINDLL
        if (!Drawing::isActive())
            break;
#endif
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

#ifdef _WINDLL
    CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)FreeLibrary, hCurrentModule, NULL, nullptr);
#endif

    // Ensure FiraCode is the default font after all font additions
    if (ImGui::GetIO().Fonts->Fonts.Size > 0) {
        ImGui::GetIO().FontDefault = ImGui::GetIO().Fonts->Fonts[0];
    }
}