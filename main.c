#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <ole2.h>

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

static void toggle_touch_keyboard(void) {
// https://stackoverflow.com/a/40921638
#define MY_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)        \
  static GUID const name = { l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8} }

  MY_DEFINE_GUID(CLSID_UIHostNoLaunch, 0x4CE576FA, 0x83DC, 0x4f88, 0x95, 0x1C,
                 0x9D, 0x07, 0x82, 0xB4, 0xE3, 0x76);

  MY_DEFINE_GUID(IID_ITipInvocation, 0x37c994e7, 0x432b, 0x4834, 0xa2, 0xf7,
                 0xdc, 0xe1, 0xf1, 0x3b, 0x83, 0x4b);

  typedef interface ITipInvocation ITipInvocation;
  typedef struct ITipInvocationVtbl {
    BEGIN_INTERFACE
    HRESULT(STDMETHODCALLTYPE *QueryInterface)
    (ITipInvocation *This, REFIID riid, void **ppvObject);
    ULONG(STDMETHODCALLTYPE *AddRef)(ITipInvocation *This);
    ULONG(STDMETHODCALLTYPE *Release)(ITipInvocation *This);
    HRESULT(STDMETHODCALLTYPE *Toggle)(ITipInvocation *This, HWND window);
    END_INTERFACE
  } ITipInvocationVtbl;
  interface ITipInvocation { CONST_VTBL ITipInvocationVtbl *lpVtbl; };

  ITipInvocation *tip = NULL;
  HRESULT hr = CoCreateInstance(&CLSID_UIHostNoLaunch, 0,
                                CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
                                &IID_ITipInvocation, (void **)&tip);
  if (FAILED(hr) || !tip) {
    printf("failed to create touch keyboard instance.\n");
    return;
  }
  tip->lpVtbl->Toggle(tip, GetDesktopWindow());
  tip->lpVtbl->Release(tip);
}

static float const initial_accel_factor = 5.f;

static bool g_joycon_horz = false;

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

    INPUT *mouse = &inputs[0];
    *mouse = (INPUT){
        .type = INPUT_MOUSE,
    };

    struct JOY_SHOCK_STATE const ss = JslGetSimpleState(g_active_device_handle);

    // stick
    float x = ss.stickLX;
    float y = ss.stickLY;
    static float const min_deadzone = .1f;
    static float const max_deadzone = 1.f / (1.f - min_deadzone);
    static float const max_accel_factor = 20.f;
    if (fabsf(x) < min_deadzone && fabsf(y) < min_deadzone) {
      x = 0.f;
      y = 0.f;
      g_accel_factor = initial_accel_factor;
    } else {
      x = fmaxf(-1.f, fminf(1.f, x * max_deadzone));
      y = fmaxf(-1.f, fminf(1.f, y * max_deadzone));
      float const len = fminf(1.f, sqrtf(x * x + y * y));
      g_accel_factor =
          fminf(max_accel_factor, g_accel_factor + len * len * len);
#if JOYMOUSE_DEBUG
      printf("x: %f, y: %f, len: %f, accel: %f\n", (double)x, (double)y,
             (double)len, (double)g_accel_factor);
#endif
      num_inputs = 1;
      if (g_joycon_horz) {
        if (ss.buttons & JSMASK_RIGHT) {
          mouse->mi.dwFlags |= MOUSEEVENTF_WHEEL;
          mouse->mi.mouseData = (DWORD)(x * 2.f * g_accel_factor);
        } else {
          mouse->mi.dwFlags |= MOUSEEVENTF_MOVE;
          mouse->mi.dx = (LONG)(-y * g_accel_factor);
          mouse->mi.dy = (LONG)(-x * g_accel_factor);
        }
      } else {
        if (ss.buttons & JSMASK_L) {
          mouse->mi.dwFlags |= MOUSEEVENTF_WHEEL;
          mouse->mi.mouseData = (DWORD)(y * 2.f * g_accel_factor);
        } else {
          mouse->mi.dwFlags |= MOUSEEVENTF_MOVE;
          mouse->mi.dx = (LONG)(x * g_accel_factor);
          mouse->mi.dy = (LONG)(-y * g_accel_factor);
        }
      }
    }

    // wheel click
    int const wheel_click_button = JSMASK_LCLICK;
    if ((ss.buttons & wheel_click_button) !=
        (g_prev_state.buttons & wheel_click_button)) {
      mouse->mi.dwFlags |= ss.buttons & wheel_click_button
                               ? MOUSEEVENTF_MIDDLEDOWN
                               : MOUSEEVENTF_MIDDLEUP;
      num_inputs = 1;
    }

    // mouse left and right click
    int const left_click_button = g_joycon_horz ? JSMASK_LEFT : JSMASK_ZL;
    int const right_click_button = g_joycon_horz ? JSMASK_DOWN : JSMASK_MINUS;
    if ((ss.buttons & left_click_button) !=
        (g_prev_state.buttons & left_click_button)) {
      mouse->mi.dwFlags |= ss.buttons & left_click_button ? MOUSEEVENTF_LEFTDOWN
                                                          : MOUSEEVENTF_LEFTUP;
      num_inputs = 1;
    }
    if ((ss.buttons & right_click_button) !=
        (g_prev_state.buttons & right_click_button)) {
      mouse->mi.dwFlags |= ss.buttons & right_click_button
                               ? MOUSEEVENTF_RIGHTDOWN
                               : MOUSEEVENTF_RIGHTUP;
      num_inputs = 1;
    }

    // show/hide touch keyboard
    int const touch_keyboard_button = g_joycon_horz ? JSMASK_UP : JSMASK_DOWN;
    if ((ss.buttons & touch_keyboard_button) !=
        (g_prev_state.buttons & touch_keyboard_button)) {
      if (ss.buttons & touch_keyboard_button) {
        toggle_touch_keyboard();
      }
    }

    // full screen
    int const full_screen_button = g_joycon_horz ? JSMASK_L : JSMASK_UP;
    if ((ss.buttons & full_screen_button) !=
        (g_prev_state.buttons & full_screen_button)) {
      inputs[num_inputs++] = (INPUT){
          .type = INPUT_KEYBOARD,
          .ki =
              {
                  .wVk = L'F',
                  .dwFlags =
                      ss.buttons & full_screen_button ? 0 : KEYEVENTF_KEYUP,
              },
      };
    }

    // browser back
    int const browser_back_button = g_joycon_horz ? JSMASK_MINUS : JSMASK_LEFT;
    if ((ss.buttons & browser_back_button) !=
        (g_prev_state.buttons & browser_back_button)) {
      inputs[num_inputs++] = (INPUT){
          .type = INPUT_KEYBOARD,
          .ki =
              {
                  .wVk = VK_BROWSER_BACK,
                  .dwFlags =
                      ss.buttons & browser_back_button ? 0 : KEYEVENTF_KEYUP,
              },
      };
    }

    // volume up and down
    int const volume_up_button = g_joycon_horz ? JSMASK_SR : JSMASK_SL;
    int const volume_down_button = g_joycon_horz ? JSMASK_SL : JSMASK_SR;
    if ((ss.buttons & volume_down_button) !=
        (g_prev_state.buttons & volume_down_button)) {
      inputs[num_inputs++] = (INPUT){
          .type = INPUT_KEYBOARD,
          .ki =
              {
                  .wVk = VK_VOLUME_DOWN,
                  .dwFlags =
                      ss.buttons & volume_down_button ? 0 : KEYEVENTF_KEYUP,
              },
      };
    }
    if ((ss.buttons & volume_up_button) !=
        (g_prev_state.buttons & volume_up_button)) {
      inputs[num_inputs++] = (INPUT){
          .type = INPUT_KEYBOARD,
          .ki =
              {
                  .wVk = VK_VOLUME_UP,
                  .dwFlags =
                      ss.buttons & volume_up_button ? 0 : KEYEVENTF_KEYUP,
              },
      };
    }

    if (num_inputs > 0) {
      SendInput((UINT)num_inputs, inputs, sizeof(INPUT));
    }
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
  CoInitialize(NULL);

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--horz") == 0) {
      g_joycon_horz = true;
      break;
    }
  }

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
                                      NULL, NULL, hInstance, NULL);
  if (!window) {
    printf("failed to create window.\n");
    goto cleanup;
  }

  ShowWindow(window, SW_HIDE);

  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

cleanup:
  UnregisterClassW(window_class_name, hInstance);
  JslDisconnectAndDisposeAll();
  CoUninitialize();
  return 0;
}
