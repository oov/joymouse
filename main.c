#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "JoyShockLibrary.h"

#define JOYMOUSE_DEBUG 0

#if JOYMOUSE_DEBUG
static void clear_console(COORD location) {
  HANDLE const h = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO info;
  GetConsoleScreenBufferInfo(h, &info);
  DWORD numChars;
  FillConsoleOutputCharacterW(h, L' ', (DWORD)(info.dwSize.X * info.dwSize.Y),
                              location, &numChars);
  SetConsoleCursorPosition(h, location);
}
#endif

static float const initial_accel_factor = 5.f;

static int g_active_device_handle = -1;
static float g_accel_factor = 0.f;
static struct JOY_SHOCK_STATE g_prev_state = {0};

static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam,
                                   LPARAM lparam) {
  (void)wparam;
  (void)lparam;
  switch (message) {
  case WM_CREATE: {
    SetTimer(window, 1, 1000 / 60, NULL);
    PostMessageW(window, WM_DEVICECHANGE, 0, 0);
  } break;
  case WM_DESTROY: {
    PostQuitMessage(0);
    return 0;
  }
  case WM_TIMER: {
    if (g_active_device_handle == -1 ||
        !JslStillConnected(g_active_device_handle)) {
      return 0;
    }

    INPUT inputs[4] = {0};
    int num_inputs = 0;

    INPUT *mouse = &inputs[num_inputs++];
    *mouse = (INPUT){
        .type = INPUT_MOUSE,
    };

    struct JOY_SHOCK_STATE const ss = JslGetSimpleState(g_active_device_handle);
    float x = ss.stickLX;
    float y = ss.stickLY;
    if (fabsf(x) < .1f && fabsf(y) < .1f) {
      x = 0.f;
      y = 0.f;
      g_accel_factor = initial_accel_factor;
    } else {
      x = fmaxf(-1.f, fminf(1.f, x * (1.f / .9f)));
      y = fmaxf(-1.f, fminf(1.f, y * (1.f / .9f)));
      g_accel_factor = fminf(20.f, g_accel_factor + .25f);
      if (ss.buttons & JSMASK_RIGHT) {
        mouse->mi.dwFlags |= MOUSEEVENTF_WHEEL;
        mouse->mi.mouseData = (DWORD)(x * 2.f * g_accel_factor);
      } else {
        mouse->mi.dwFlags |= MOUSEEVENTF_MOVE;
        mouse->mi.dx = (LONG)(-y * g_accel_factor);
        mouse->mi.dy = (LONG)(-x * g_accel_factor);
      }
    }
    if ((ss.buttons & JSMASK_LCLICK) !=
        (g_prev_state.buttons & JSMASK_LCLICK)) {
      mouse->mi.dwFlags |= ss.buttons & JSMASK_LCLICK ? MOUSEEVENTF_MIDDLEDOWN
                                                      : MOUSEEVENTF_MIDDLEUP;
    }
    if ((ss.buttons & JSMASK_LEFT) != (g_prev_state.buttons & JSMASK_LEFT)) {
      mouse->mi.dwFlags |=
          ss.buttons & JSMASK_LEFT ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    }
    if ((ss.buttons & JSMASK_DOWN) != (g_prev_state.buttons & JSMASK_DOWN)) {
      mouse->mi.dwFlags |= ss.buttons & JSMASK_DOWN ? MOUSEEVENTF_RIGHTDOWN
                                                    : MOUSEEVENTF_RIGHTUP;
    }
    if ((ss.buttons & JSMASK_L) != (g_prev_state.buttons & JSMASK_L)) {
      inputs[num_inputs++] = (INPUT){
          .type = INPUT_KEYBOARD,
          .ki =
              {
                  .wVk = L'F',
                  .dwFlags = ss.buttons & JSMASK_L ? 0 : KEYEVENTF_KEYUP,
              },
      };
    }
    if ((ss.buttons & JSMASK_MINUS) != (g_prev_state.buttons & JSMASK_MINUS)) {
      inputs[num_inputs++] = (INPUT){
          .type = INPUT_KEYBOARD,
          .ki =
              {
                  .wVk = VK_BROWSER_BACK,
                  .dwFlags = ss.buttons & JSMASK_MINUS ? 0 : KEYEVENTF_KEYUP,
              },
      };
    }
    if ((ss.buttons & JSMASK_SL) != (g_prev_state.buttons & JSMASK_SL)) {
      inputs[num_inputs++] = (INPUT){
          .type = INPUT_KEYBOARD,
          .ki =
              {
                  .wVk = VK_VOLUME_DOWN,
                  .dwFlags = ss.buttons & JSMASK_SL ? 0 : KEYEVENTF_KEYUP,
              },
      };
    }
    if ((ss.buttons & JSMASK_SR) != (g_prev_state.buttons & JSMASK_SR)) {
      inputs[num_inputs++] = (INPUT){
          .type = INPUT_KEYBOARD,
          .ki =
              {
                  .wVk = VK_VOLUME_UP,
                  .dwFlags = ss.buttons & JSMASK_SR ? 0 : KEYEVENTF_KEYUP,
              },
      };
    }
    SendInput((UINT)num_inputs, inputs, sizeof(INPUT));
    g_prev_state = ss;
#if JOYMOUSE_DEBUG
    clear_console((COORD){0, 0});
    printf("left stick: %f, %f\n", (double)x, (double)y);
    printf("buttons: %d\n", ss.buttons);
#endif
  } break;
  case WM_DEVICECHANGE: {
    g_active_device_handle = -1;
    enum {
      max_devices = 16,
    };
    int devices[max_devices];
    int const num_devices = JslConnectDevices();
#if JOYMOUSE_DEBUG
    printf("num_devices: %d\n", num_devices);
#endif
    if (num_devices <= 0) {
      break;
    }
    if (num_devices > max_devices) {
      printf("too many devices.\n");
      break;
    }
    JslGetConnectedDeviceHandles(devices, max_devices);
    for (int i = 0; i < num_devices; ++i) {
      struct JSL_SETTINGS settings =
          JslGetControllerInfoAndSettings(devices[i]);
#if JOYMOUSE_DEBUG
      printf("device: %d\n", devices[i]);
      printf("  controllerType: %d\n", settings.controllerType);
      printf("  splitType: %d\n", settings.splitType);
#endif
      if (settings.controllerType != JS_TYPE_JOYCON_LEFT &&
          settings.controllerType != JS_TYPE_JOYCON_RIGHT) {
        continue;
      }
      if (settings.splitType != JS_SPLIT_TYPE_LEFT &&
          settings.splitType != JS_SPLIT_TYPE_RIGHT) {
        continue;
      }
      g_active_device_handle = devices[i];
#if JOYMOUSE_DEBUG
      printf("select device: %d\n", g_active_device_handle);
#endif
      break;
    }
  } break;
  default:
    return DefWindowProcW(window, message, wparam, lparam);
  }
  return 0;
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  HINSTANCE const hInstance = GetModuleHandleW(NULL);
  static wchar_t const window_class_name[] = L"JoyMouse";
  if (!RegisterClassExW(&(WNDCLASSEXW){
          .cbSize = sizeof(WNDCLASSEXW),
          .lpfnWndProc = WindowProc,
          .hInstance = hInstance,
          .lpszClassName = window_class_name,
      })) {
    printf("failed to register window class.\n");
    return 1;
  }

  HWND const window = CreateWindowExW(0, window_class_name, L"", 0, 0, 0, 0, 0,
                                      HWND_MESSAGE, NULL, hInstance, NULL);
  if (!window) {
    printf("failed to create window.\n");
    goto cleanup;
  }

  ShowWindow(window, SW_SHOWDEFAULT);
  RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

cleanup:
  UnregisterClassW(window_class_name, hInstance);
  JslDisconnectAndDisposeAll();
  return 0;
}
