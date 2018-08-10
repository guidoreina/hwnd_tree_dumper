#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <psapi.h>

#define CLASS_NAME_MAX_LEN 256

typedef struct window_t {
  /* Window handle. */
  HWND hwnd;

  /* Handle of the parent window. */
  HWND parent_hwnd;

  /* Window class name. */
  TCHAR* class_name;

  /* Window text. */
  TCHAR* text;

  /* Path of the executable. */
  TCHAR* path;

  /* Parent window. */
  struct window_t* parent;

  /* Depth relative to the desktop window. */
  size_t depth;

  /* Next window. */
  struct window_t* next;

  /* Children windows. */
  struct window_t* children;
} window_t;

typedef struct {
  window_t** windows;

  size_t size;
  size_t used;
} windows_t;

typedef struct {
  windows_t windows;

  HWND desktop;

  BOOL result;
} parameters_t;

static BOOL CALLBACK enum_child_proc(HWND hwnd, LPARAM lParam);

static window_t* window_create(HWND hwnd, HWND desktop);
static void window_destroy_common(window_t* window);
static void window_destroy(window_t* window);
static void window_destroy_recursive(window_t* window);
static void window_print(const window_t* window);

static void windows_init(windows_t* windows);
static void windows_destroy(windows_t* windows);
static BOOL windows_add_window(windows_t* windows, HWND hwnd, HWND desktop);

static window_t* tree_build(windows_t* windows, window_t* parent);
static BOOL tree_window_has_next(const window_t* window, size_t depth);

static DWORD get_path_executable(HWND hwnd, TCHAR* path, size_t size);
static TCHAR* _tcsndup(const TCHAR* s, size_t n);
static void print_class_name(const TCHAR* class_name);
static void spaces(size_t count);

int main()
{
  HWND hwnd;
  parameters_t params;
  window_t* root;

  /* Get desktop window. */
  if ((hwnd = GetDesktopWindow()) != NULL) {
    /* Initialize windows. */
    windows_init(&params.windows);

    /* Add desktop window. */
    if ((params.result = windows_add_window(&params.windows,
                                            hwnd,
                                            hwnd)) == TRUE) {
      params.desktop = hwnd;

      /* Enumerate child windows. */
      EnumChildWindows(hwnd, enum_child_proc, (LPARAM) &params);

      /* If no error... */
      if (params.result) {
        /* Build tree. */
        root = tree_build(&params.windows, NULL);

        if (root) {
          window_print(root);

          window_destroy_recursive(root);
        }

        windows_destroy(&params.windows);

        return 0;
      }
    }

    windows_destroy(&params.windows);

    _ftprintf_p(stderr, TEXT("Error adding window.\n"));
  } else {
    _ftprintf_p(stderr, TEXT("Error getting desktop window.\n"));
  }

  return -1;
}

BOOL CALLBACK enum_child_proc(HWND hwnd, LPARAM lParam)
{
  parameters_t* params;

  params = (parameters_t*) lParam;

  /* Add current window. */
  if (windows_add_window(&params->windows, hwnd, params->desktop)) {
    return TRUE;
  } else {
    params->result = FALSE;
    return FALSE;
  }
}


/*******************************************************************************
 *******************************************************************************
 **                                                                           **
 ** Window functions.                                                         **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

window_t* window_create(HWND hwnd, HWND desktop)
{
  TCHAR class_name[CLASS_NAME_MAX_LEN + 1];
  TCHAR text[4 * 1024];
  TCHAR path[MAX_PATH];
  window_t* window;
  int len;

  /* Get class name. */
  if ((len = GetClassName(hwnd, class_name, _countof(class_name))) > 0) {
    /* Allocate memory for the window. */
    if ((window = (window_t*) calloc(1, sizeof(window_t))) != NULL) {
      if ((window->class_name = _tcsndup(class_name, len)) != NULL) {
        /* If the window has a text... */
        if ((len = SendMessage(hwnd,
                               WM_GETTEXT,
                               _countof(text),
                               (LPARAM) text)) > 0) {
          if ((window->text = _tcsndup(text, len)) == NULL) {
            free(window->class_name);
            free(window);

            return NULL;
          }
        }

        /* Get handle of the parent window. */
        window->parent_hwnd = GetAncestor(hwnd, GA_PARENT);

        /* If a direct child of the desktop... */
        if (window->parent_hwnd == desktop) {
          /* Get path of the executable. */
          if ((len = get_path_executable(hwnd, path, _countof(path))) > 0) {
            if ((window->path = _tcsndup(path, len)) == NULL) {
              if (window->text) {
                free(window->text);
              }

              free(window->class_name);
              free(window);

              return NULL;
            }
          }
        }

        window->hwnd = hwnd;

        return window;
      } else {
        free(window);
      }
    }
  }

  return NULL;
}

void window_destroy_common(window_t* window)
{
  free(window->class_name);

  if (window->text) {
    free(window->text);
  }

  if (window->path) {
    free(window->path);
  }
}

void window_destroy(window_t* window)
{
  window_destroy_common(window);
  free(window);
}

void window_destroy_recursive(window_t* window)
{
  window_destroy_common(window);

  if (window->next) {
    window_destroy_recursive(window->next);
  }

  if (window->children) {
    window_destroy_recursive(window->children);
  }

  free(window);
}

void window_print(const window_t* window)
{
  static const size_t tab_size = 2; /* Spaces. */

  size_t i;

  do {
    if (window->depth > 0) {
      /* Print first line. */
      for (i = 1; i < window->depth; i++) {
        spaces(tab_size + 2);

        _tprintf(TEXT("%c"), tree_window_has_next(window, i) ? '|' : ' ');
      }

      spaces(tab_size + 2);
      _tprintf(TEXT("|\n"));

      /* Print second line. */
      for (i = 1; i < window->depth; i++) {
        spaces(tab_size + 2);

        _tprintf(TEXT("%c"), tree_window_has_next(window, i) ? '|' : ' ');
      }

      spaces(tab_size + 2);

      _tprintf(TEXT("%c-- [0x%x] "),
               tree_window_has_next(window, window->depth) ? '+' : '\\',
               window->hwnd);

      print_class_name(window->class_name);

      if (window->text) {
        _tprintf(TEXT(", text: '%s'"), window->text);
      }

      if (window->path) {
        _tprintf(TEXT(", exe: '%s'"), window->path);
      }

      _tprintf(TEXT("\n"));
    } else {
      _tprintf(TEXT("/- [0x%x] "), window->hwnd);

      print_class_name(window->class_name);

      _tprintf(TEXT("\n"));
    }

    if (window->children) {
      window_print(window->children);
    }
  } while ((window = window->next) != NULL);
}


/*******************************************************************************
 *******************************************************************************
 **                                                                           **
 ** Windows functions.                                                        **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

void windows_init(windows_t* windows)
{
  windows->windows = NULL;

  windows->size = 0;
  windows->used = 0;
}

void windows_destroy(windows_t* windows)
{
  size_t i;

  if (windows->windows) {
    for (i = 0; i < windows->used; i++) {
      window_destroy(windows->windows[i]);
    }

    free(windows->windows);
    windows->windows = NULL;
  }

  windows->size = 0;
  windows->used = 0;
}

BOOL windows_add_window(windows_t* windows, HWND hwnd, HWND desktop)
{
  window_t** w;
  size_t size;

  /* If we have to allocate space for more pointers... */
  if (windows->used == windows->size) {
    size = (windows->size > 0) ? 2 * windows->size : (4 * 1024);

    /* If not overflow and we can allocate space for more pointers... */
    if ((size > windows->size) &&
        ((w = (window_t**) realloc(windows->windows, 
                                   size * sizeof(window_t*))) != NULL)) {
      windows->windows = w;
      windows->size = size;
    } else {
      return FALSE;
    }
  }

  /* Create window. */
  if ((windows->windows[windows->used] = window_create(hwnd,
                                                       desktop)) != NULL) {
    windows->used++;

    return TRUE;
  }

  return FALSE;
}


/*******************************************************************************
 *******************************************************************************
 **                                                                           **
 ** Tree functions.                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

window_t* tree_build(windows_t* windows, window_t* parent)
{
  HWND parent_hwnd;
  size_t depth;
  window_t* res;
  window_t* window;
  window_t* w;
  size_t i;

  if (parent) {
    parent_hwnd = parent->hwnd;
    depth = parent->depth + 1;
  } else {
    parent_hwnd = NULL;
    depth = 0;
  }

  res = NULL;

  i = 0;

  while (i < windows->used) {
    if (windows->windows[i]->parent_hwnd == parent_hwnd) {
      /* Save window. */
      window = windows->windows[i];

      /* Set parent. */
      window->parent = parent;

      /* Set depth. */
      window->depth = depth;

      windows->used--;

      /* If not the last window... */
      if (i < windows->used) {
        memmove(windows->windows + i,
                windows->windows + i + 1,
                (windows->used - i) * sizeof(window_t*));
      }

      if (!res) {
        res = window;
      } else {
        w = res;

        while (w->next) {
          w = w->next;
        }

        w->next = window;
      }

      window->children = tree_build(windows, window);
    } else {
      i++;
    }
  }

  return res;
}

BOOL tree_window_has_next(const window_t* window, size_t depth)
{
  size_t n;

  n = window->depth;

  while (n > depth) {
    if (window->parent) {
      window = window->parent;
      n--;
    } else {
      return FALSE;
    }
  }

  return window->next ? TRUE : FALSE;
}


/*******************************************************************************
 *******************************************************************************
 **                                                                           **
 ** Helper functions.                                                         **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

DWORD get_path_executable(HWND hwnd, TCHAR* path, size_t size)
{
  DWORD pid;
  HANDLE process;
  DWORD len;

  /* Get process id of the window. */
  if (GetWindowThreadProcessId(hwnd, &pid)) {
    /* Open process. */
    if ((process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                               FALSE,
                               pid)) != NULL) {
      /* Get name of the executable. */
      len = GetModuleFileNameEx(process, NULL, path, size);

      CloseHandle(process);

      return len;
    }
  }

  return 0;
}

TCHAR* _tcsndup(const TCHAR* s, size_t n)
{
  TCHAR* res;

  if ((res = (TCHAR*) malloc((n + 1) * sizeof(TCHAR))) != NULL) {
    memcpy(res, s, n * sizeof(TCHAR));
    res[n] = 0;

    return res;
  }

  return NULL;
}

void print_class_name(const TCHAR* class_name)
{
  _tprintf(TEXT("Class name: '%s'"), class_name);

  if (_tcscmp(class_name, TEXT("#32768")) == 0) {
    _tprintf(TEXT(" (Menu)"));
  } else if (_tcscmp(class_name, TEXT("#32769")) == 0) {
    _tprintf(TEXT(" (Desktop)"));
  } else if (_tcscmp(class_name, TEXT("#32770")) == 0) {
    _tprintf(TEXT(" (Dialog box)"));
  } else if (_tcscmp(class_name, TEXT("#32771")) == 0) {
    _tprintf(TEXT(" (Task switch window)"));
  } else if (_tcscmp(class_name, TEXT("#32772")) == 0) {
    _tprintf(TEXT(" (Icon title)"));
  }
}

void spaces(size_t count)
{
  size_t i;

  for (i = 0; i < count; i++) {
    _tprintf(TEXT(" "));
  }
}