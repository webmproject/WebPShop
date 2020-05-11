// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "PIUI.h"

#ifdef __PIWin__

#include <Windows.h>
#include <commctrl.h>

#include <string>

#include "WebPShopUI.h"

//------------------------------------------------------------------------------

static RECT VRectToRECT(const VRect& rect) {
  RECT r;
  r.left = rect.left;
  r.right = rect.right;
  r.top = rect.top;
  r.bottom = rect.bottom;
  return r;
}

//------------------------------------------------------------------------------

void PISlider::SetItem(PIDialogPtr dialog, int16 item_id, int min, int max) {
  item_ = PIGetDialogItem(dialog, item_id);
  SendMessage(item_, (UINT)TBM_SETRANGEMIN, (WPARAM)(BOOL)FALSE, (LPARAM)min);
  SendMessage(item_, (UINT)TBM_SETRANGEMAX, (WPARAM)(BOOL)FALSE, (LPARAM)max);
}
void PISlider::SetValue(int value) {
  SendMessage(item_, (UINT)TBM_SETPOS, (WPARAM)(BOOL)TRUE, (LPARAM)value);
}
int PISlider::GetValue(void) {
  return (int)SendMessage(item_, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0);
}

//------------------------------------------------------------------------------

void PIIntegerField::SetItem(PIDialogPtr dialog, int16 item_id, int min,
                             int max) {
  dialog_ = dialog;
  item_id_ = item_id;
  min_ = min(min, max);
  max_ = max(min, max);
}
void PIIntegerField::SetValue(int value) {
  char txt[4] = {'\0', '\0', '\0', '\0'};
  if (value < 0) {
    txt[0] = '0';
  } else if (value < 10) {
    txt[0] = '0' + value % 10;
  } else if (value < 100) {
    txt[1] = '0' + value % 10;
    value /= 10;
    txt[0] = '0' + value % 10;
  } else {
    txt[2] = '0' + value % 10;
    value /= 10;
    txt[1] = '0' + value % 10;
    value /= 10;
    txt[0] = '0' + value % 10;
  }
  SetDlgItemText(dialog_, item_id_, txt);
}
int PIIntegerField::GetValue(void) {
  char txt[5];
  if (GetDlgItemText(dialog_, item_id_, txt, 4)) {
    int value = 0;
    for (int i = 0; i < 3 && txt[i] >= '0' && txt[i] <= '9'; ++i) {
      value = value * 10 + txt[i] - '0';
    }
    if (value >= min_ && value <= max_) return value;
  }
  return -1;
}

//------------------------------------------------------------------------------

PIItem WebPShopDialog::GetItem(short item) {
  return GetDlgItem(GetDialog(), item);
}
void WebPShopDialog::ShowItem(short item) {
  ShowWindow(GetItem(item), SW_SHOW);
}
void WebPShopDialog::HideItem(short item) {
  ShowWindow(GetItem(item), SW_HIDE);
}
void WebPShopDialog::EnableItem(short item) {
  EnableWindow(GetItem(item), TRUE);
}
void WebPShopDialog::DisableItem(short item) {
  EnableWindow(GetItem(item), FALSE);
}

//------------------------------------------------------------------------------

VRect WebPShopDialog::GetProxyAreaRectInWindow(void) {
  RECT proxy_rect_in_screen;
  GetWindowRect(GetDlgItem(GetDialog(), kDProxy), &proxy_rect_in_screen);
  POINT window_pos;
  window_pos.x = window_pos.y = 0;
  ClientToScreen(GetDialog(), &window_pos);

  VRect proxy_rect_in_window;
  proxy_rect_in_window.left = proxy_rect_in_screen.left - window_pos.x;
  proxy_rect_in_window.right = proxy_rect_in_screen.right - window_pos.x;
  proxy_rect_in_window.top = proxy_rect_in_screen.top - window_pos.y;
  proxy_rect_in_window.bottom = proxy_rect_in_screen.bottom - window_pos.y;
  return proxy_rect_in_window;
}

void WebPShopDialog::BeginPainting(PaintingContext* const painting_context) {
  painting_context->hDC = BeginPaint(GetDialog(), &painting_context->ps);
}
void WebPShopDialog::EndPainting(PaintingContext* const painting_context) {
  EndPaint(GetDialog(), (LPPAINTSTRUCT)&painting_context->ps);
}
void WebPShopDialog::ClearRect(const VRect& rect,
                               PaintingContext* const painting_context) {
  RECT proxy_RECT = VRectToRECT(rect);
  FillRect(painting_context->hDC, &proxy_RECT, GetSysColorBrush(COLOR_MENU));
}
void WebPShopDialog::DrawRectBorder(uint8_t r, uint8_t g, uint8_t b,
                                    int left, int top, int right, int bottom,
                                    PaintingContext* const painting_context) {
  HGDIOBJ original =
      SelectObject(painting_context->hDC, GetStockObject(HOLLOW_BRUSH));
  SelectObject(painting_context->hDC, GetStockObject(DC_PEN));
  SetDCPenColor(painting_context->hDC, RGB(r, g, b));
  Rectangle(painting_context->hDC, left, top, right, bottom);
  SelectObject(painting_context->hDC, original);
}

//------------------------------------------------------------------------------

bool WebPShopDialog::DisplayImage(const ImageMemoryDesc& image,
                                  const VRect& rect,
                                  DisplayPixelsProc display_pixels_proc,
                                  PaintingContext* const painting_context) {
  if (display_pixels_proc == nullptr) {
    LOG("/!\\ No display function.");
    return false;
  }

  PSPixelMap pixels;
  PSPixelMask mask;
  pixels.version = 1;
  pixels.bounds.left = 0;
  pixels.bounds.right = image.width;
  pixels.bounds.top = 0;
  pixels.bounds.bottom = image.height;
  pixels.imageMode = image.mode;
  pixels.planeBytes = image.pixels.depth / 8;
  pixels.colBytes = image.pixels.colBits / 8;
  pixels.rowBytes = image.pixels.rowBits / 8;
  pixels.baseAddr = image.pixels.data;

  pixels.mat = NULL;
  pixels.masks = NULL;
  pixels.maskPhaseRow = 0;
  pixels.maskPhaseCol = 0;

  if (image.num_channels == 4) {
    mask.next = NULL;
    mask.maskData = (uint8_t*)image.pixels.data + (image.pixels.depth / 8) * 3;
    mask.colBytes = image.pixels.colBits / 8;
    mask.rowBytes = image.pixels.rowBits / 8;
    mask.maskDescription = kSimplePSMask;

    pixels.masks = &mask;
  }

  const OSErr err =
      display_pixels_proc(&pixels, &pixels.bounds, rect.top, rect.left,
                          (void*)painting_context->hDC);
  if (err != noErr) {
    LOG("/!\\ DisplayPixelsProc() failed: " << err);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------

void WebPShopDialog::TriggerRepaint() {
  RECT imageRect;
  PIDialogPtr dialog = GetDialog();
  GetWindowRect(GetDlgItem(dialog, kDProxy), &imageRect);
  ScreenToClient(dialog, (LPPOINT)&imageRect);
  ScreenToClient(dialog, (LPPOINT) & (imageRect.right));
  InvalidateRect(dialog, &imageRect, FALSE);
}

DLLExport BOOL WINAPI WindowProc(HWND hDlg, UINT wMsg, WPARAM wParam,
                                 LPARAM lParam) {
  static WebPShopDialog* owner = NULL;

  switch (wMsg) {
    case WM_INITDIALOG: {
      CenterDialog(hDlg);
      owner = static_cast<WebPShopDialog*>((void*)(lParam));
      if (owner == NULL) return FALSE;
      owner->SetDialog(hDlg);
      owner->Init();
      if (owner != NULL) owner->PaintProxy();
      return TRUE;
    }
    case WM_PAINT: {
      if (owner != NULL) owner->PaintProxy();
      return FALSE;
    }
    case WM_COMMAND: {
      int item = LOWORD(wParam);
      int cmd = HIWORD(wParam);
      if (owner != NULL) owner->Notify(item);
      if ((item == 1 || item == 2) && cmd == BN_CLICKED) {
        EndDialog(hDlg, item);
      }
      return TRUE;
    }
    case WM_DESTROY: {
      if (owner != NULL) owner->DeallocateCompressedFrames();
      owner = NULL;
      return FALSE;
    }
    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE: {
      int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
      if (owner != NULL) owner->OnMouseMove(x, y, wParam & MK_LBUTTON);
      return FALSE;
    }
    default:
      // Slider (trackbar) messages arrive here.
      int32 item = LOWORD(wParam);
      if (item == kDQualitySlider || item == kDFrameSlider) {
        if (owner != NULL) owner->Notify(item);
      } else {
        DefWindowProc(hDlg, wMsg, wParam, lParam);
      }
      return FALSE;
  }
}

int WebPShopDialog::Modal(SPPluginRef plugin, const char*, int ID) {
  SetID(ID);
  SetPluginRef(plugin);

  const int itemHit = (int)(DialogBoxParam(
      GetDLLInstance(GetPluginRef()), MAKEINTRESOURCE(GetID()),
      GetActiveWindow(), (DLGPROC)WindowProc, (LPARAM)this));
  return itemHit;
}

//------------------------------------------------------------------------------
// About box

class PIWebPShopAboutBox : public PIDialog {
 private:
  virtual void Init() {}
  virtual void Notify(int32 item) {
    if (item == kDAboutWebPLink) {
      sPSFileList->BrowseUrl("https://developers.google.com/speed/webp");
    }
  }
  virtual void Message(UINT wMsg, WPARAM wParam, LPARAM lParam) {
    switch (wMsg) {
      case WM_CHAR: {
        TCHAR chCharCode = (TCHAR)wParam;
        if (chCharCode == VK_ESCAPE || chCharCode == VK_RETURN)
          EndDialog(GetDialog(), 0);
        break;
      }
      case WM_LBUTTONUP: {
        EndDialog(GetDialog(), 0);
        break;
      }
    }
  }

 public:
  PIWebPShopAboutBox() : PIDialog() {}
  ~PIWebPShopAboutBox() {}
};

void DoAboutBox(SPPluginRef plugin_ref) {
  PIWebPShopAboutBox aboutBox;
  (void)aboutBox.Modal(plugin_ref, "About WebPShop", 16091);
}

#endif  // __PIWin__
