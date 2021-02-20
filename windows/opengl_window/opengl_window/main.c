#include <windows.h>
#include <gl/gl.h>

struct state {
  int test;
};

LRESULT CALLBACK window_procedure(HWND window, UINT msg, WPARAM w_param,
                                  LPARAM l_param) {
  struct state *state;
  if (msg == WM_CREATE) {
    CREATESTRUCT *create_struct = (CREATESTRUCT *)(l_param);
    state = (struct state *)(create_struct->lpCreateParams);
    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)state);
  } else {
    state = (struct state *)(GetWindowLongPtr(window, GWLP_USERDATA));
  }

  switch (msg) {
  case WM_PAINT: {
    HDC device_context = GetDC(window);

    glViewport(0, 0, 800, 600);
    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    SwapBuffers(device_context);
  } break;

  case WM_CLOSE:
    DestroyWindow(window);
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  default:
    return DefWindowProc(window, msg, w_param, l_param);
  }

  return 0;
}

void initialize_opengl(HWND window) {
  // TODO(taylon): give another more comprehensive look at every single option
  // just for the sake of curiosity
  PIXELFORMATDESCRIPTOR desired_pixel_format = {
      .nSize = sizeof(PIXELFORMATDESCRIPTOR),
      .nVersion = 1,
      .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
      .iPixelType = PFD_TYPE_RGBA,
      .cColorBits = 32,
      .cDepthBits = 24,
      .cStencilBits = 8,
      .iLayerType = PFD_MAIN_PLANE,
  };

  HDC device_context = GetDC(window);
  int pixel_format_index =
      ChoosePixelFormat(device_context, &desired_pixel_format);
  if (pixel_format_index == 0) {
    // TODO(taylon): Diagnostic
    return;
  }

  PIXELFORMATDESCRIPTOR pixel_format;
  DescribePixelFormat(device_context, pixel_format_index, sizeof(pixel_format),
                      &pixel_format);

  if (!SetPixelFormat(device_context, pixel_format_index, &pixel_format)) {
    // TODO(taylon): Diagnostic
    return;
  }

  HGLRC opengl_context = wglCreateContext(device_context);
  if (!wglMakeCurrent(device_context, opengl_context)) {
    // TODO(taylon): Diagnostic
    return;
  }

  ReleaseDC(window, device_context);
}

int WINAPI wWinMain(_In_ HINSTANCE instance,
                    _In_opt_ HINSTANCE previous_instance, _In_ PWSTR cmd_line,
                    _In_ int cmd_show) {
  const wchar_t class_name[] = L"Window Class";
  WNDCLASS window_class = {
      .lpfnWndProc = window_procedure,
      .hInstance = instance,
      .lpszClassName = class_name,
  };

  RegisterClass(&window_class);

  struct state *state = {0};
  HWND window = CreateWindowEx(0, class_name, L"My Window", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               CW_USEDEFAULT, NULL, NULL, instance, state);
  if (window == NULL) {
    // TODO(taylon): diagnostic
    return 0;
  }

  initialize_opengl(window);
  ShowWindow(window, cmd_show);

  MSG message = {0};
  while (GetMessage(&message, NULL, 0, 0)) {
    TranslateMessage(&message);
    DispatchMessage(&message);
  }

  return 0;
}
