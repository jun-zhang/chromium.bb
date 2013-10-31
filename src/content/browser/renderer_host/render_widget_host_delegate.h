// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDER_WIDGET_HOST_DELEGATE_H_
#define CONTENT_BROWSER_RENDER_WIDGET_HOST_DELEGATE_H_

#include "build/build_config.h"
#include "content/common/content_export.h"
#include "ui/gfx/native_widget_types.h"
#include "base/string16.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebTextDirection.h"

namespace WebKit {
class WebMouseWheelEvent;
}

namespace content {

class RenderWidgetHostImpl;
struct NativeWebKeyboardEvent;

//
// RenderWidgetHostDelegate
//
//  An interface implemented by an object interested in knowing about the state
//  of the RenderWidgetHost.
class CONTENT_EXPORT RenderWidgetHostDelegate {
 public:
  // The RenderWidgetHost is going to be deleted.
  virtual void RenderWidgetDeleted(RenderWidgetHostImpl* render_widget_host) {}

  // Callback to give the browser a chance to handle the specified keyboard
  // event before sending it to the renderer.
  // Returns true if the |event| was handled. Otherwise, if the |event| would
  // be handled in HandleKeyboardEvent() method as a normal keyboard shortcut,
  // |*is_keyboard_shortcut| should be set to true.
  virtual bool PreHandleKeyboardEvent(const NativeWebKeyboardEvent& event,
                                      bool* is_keyboard_shortcut);

  // Callback to inform the browser that the renderer did not process the
  // specified events. This gives an opportunity to the browser to process the
  // event (used for keyboard shortcuts).
  virtual void HandleKeyboardEvent(const NativeWebKeyboardEvent& event) {}

  // Callback to give the browser a chance to handle the specified mouse wheel
  // event before sending it to the renderer.
  // Returns true if the |event| was handled.
  virtual bool PreHandleWheelEvent(const WebKit::WebMouseWheelEvent& event);

  // Allows delegate to show a custom tooltip. If the delegate doesn't want a
  // custom tooltip, it should just return 'false'. Otherwise, it should show
  // the tooltip and return 'true'. By default, the delegate doesn't provide a
  // custom tooltip.
  virtual bool ShowTooltip(const string16& tooltip_text,
                           WebKit::WebTextDirection text_direction_hint);

#if defined(OS_WIN) && defined(USE_AURA)
  // Returns the widget's parent's NativeViewAccessible.
  virtual gfx::NativeViewAccessible GetParentNativeViewAccessible();
#endif

 protected:
  virtual ~RenderWidgetHostDelegate() {}
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDER_WIDGET_HOST_DELEGATE_H_
