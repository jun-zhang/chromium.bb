/*
 * Copyright (C) 2013 Bloomberg Finance L.P.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS," WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <blpwtk2_webviewimpl.h>

#include <blpwtk2_browsercontextimpl.h>
#include <blpwtk2_devtoolsfrontendhostdelegateimpl.h>
#include <blpwtk2_newviewparams.h>
#include <blpwtk2_webframeimpl.h>
#include <blpwtk2_stringref.h>
#include <blpwtk2_webviewdelegate.h>
#include <blpwtk2_statics.h>

#include <base/message_loop.h>
#include <content/public/browser/devtools_agent_host.h>
#include <content/public/browser/devtools_http_handler.h>
#include <content/public/browser/render_view_host.h>
#include <content/public/browser/render_process_host.h>
#include <content/public/browser/web_contents.h>
#include <content/public/browser/web_contents_view.h>
#include <content/public/browser/site_instance.h>
#include <content/public/renderer/render_view.h>
#include <third_party/WebKit/Source/WebKit/chromium/public/WebView.h>

namespace blpwtk2 {


WebViewImpl::WebViewImpl(WebViewDelegate* delegate,
                         gfx::NativeView parent,
                         content::BrowserContext* browserContext,
                         int hostAffinity,
                         bool initiallyVisible)
: d_delegate(delegate)
, d_focusBeforeEnabled(false)
, d_focusAfterEnabled(false)
, d_isReadyForDelete(false)
, d_wasDestroyed(false)
, d_isDeletingSoon(false)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(browserContext);

    content::WebContents::CreateParams createParams(browserContext);
    createParams.render_process_affinity = hostAffinity;
    d_webContents.reset(content::WebContents::Create(createParams));
    d_webContents->SetDelegate(this);

    if (!initiallyVisible)
        ShowWindow(getNativeView(), SW_HIDE);

    d_originalParent = GetParent(getNativeView());
    SetParent(getNativeView(), parent);
}

WebViewImpl::WebViewImpl(content::WebContents* contents)
: d_delegate(0)
, d_focusBeforeEnabled(false)
, d_focusAfterEnabled(false)
, d_isReadyForDelete(false)
, d_wasDestroyed(false)
, d_isDeletingSoon(false)
{
    DCHECK(Statics::isInBrowserMainThread());
    d_webContents.reset(contents);
    d_webContents->SetDelegate(this);

    d_originalParent = GetParent(getNativeView());
}

WebViewImpl::~WebViewImpl()
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(d_wasDestroyed);
    DCHECK(d_isReadyForDelete);
    DCHECK(d_isDeletingSoon);
    SetParent(getNativeView(), d_originalParent);
}

gfx::NativeView WebViewImpl::getNativeView() const
{
    DCHECK(Statics::isInBrowserMainThread());
    return d_webContents->GetView()->GetNativeView();
}

void WebViewImpl::showContextMenu(const ContextMenuParams& params)
{
    DCHECK(Statics::isInBrowserMainThread());
    if (d_wasDestroyed) return;
    if (d_delegate)
        d_delegate->showContextMenu(this, params);
}

void WebViewImpl::destroy()
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    DCHECK(!d_isDeletingSoon);
    d_wasDestroyed = true;
    if (d_isReadyForDelete) {
        d_isDeletingSoon = true;
        MessageLoop::current()->DeleteSoon(FROM_HERE, this);
    }
}

WebFrame* WebViewImpl::mainFrame()
{
    DCHECK(Statics::isRendererMainThreadMode());
    DCHECK(Statics::isInApplicationMainThread());
    DCHECK(!d_wasDestroyed);
    DCHECK(d_webContents->GetRenderProcessHost()->IsInProcess());

    if (!d_mainFrame.get()) {
        int routingId = d_webContents->GetRoutingID();
        content::RenderView* rv = content::RenderView::FromRoutingID(routingId);
        DCHECK(rv);

        WebKit::WebFrame* webFrame = rv->GetWebView()->mainFrame();
        d_mainFrame.reset(new WebFrameImpl(webFrame));
    }

    return d_mainFrame.get();
}

void WebViewImpl::loadUrl(const StringRef& url)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    std::string surl(url.data(), url.length());
    GURL gurl(surl);
    if (!gurl.has_scheme())
        gurl = GURL("http://" + surl);

    d_webContents->GetController().LoadURL(
        gurl,
        content::Referrer(),
        content::PageTransitionFromInt(content::PAGE_TRANSITION_TYPED | content::PAGE_TRANSITION_FROM_ADDRESS_BAR),
        std::string());
}

void WebViewImpl::loadInspector(WebView* inspectedView)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    DCHECK(inspectedView);

    WebViewImpl* inspectedViewImpl
        = static_cast<WebViewImpl*>(inspectedView);
    content::WebContents* inspectedContents = inspectedViewImpl->d_webContents.get();
    DCHECK(inspectedContents);

    scoped_refptr<content::DevToolsAgentHost> agentHost
        = content::DevToolsAgentHost::GetOrCreateFor(inspectedContents->GetRenderViewHost());

    DevToolsFrontendHostDelegateImpl* devtoolsFrontend
        = new DevToolsFrontendHostDelegateImpl(d_webContents.get(), agentHost);

    GURL url = Statics::devToolsHttpHandler->GetFrontendURL(NULL);
    loadUrl(url.spec());
}

void WebViewImpl::reload(bool ignoreCache)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    const bool checkForRepost = false;  // TODO: do we want to make this an argument

    if (ignoreCache)
        d_webContents->GetController().ReloadIgnoringCache(checkForRepost);
    else
        d_webContents->GetController().Reload(checkForRepost);
}

void WebViewImpl::focus()
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    d_webContents->GetView()->Focus();
}

void WebViewImpl::show()
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    ::ShowWindow(getNativeView(), SW_SHOW);
}

void WebViewImpl::hide()
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    ::ShowWindow(getNativeView(), SW_HIDE);
}

void WebViewImpl::setParent(NativeView parent)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    ::SetParent(getNativeView(), parent);
}

void WebViewImpl::move(int left, int top, int width, int height, bool repaint)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    ::MoveWindow(getNativeView(), left, top, width, height, repaint);
}

void WebViewImpl::cutSelection()
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    d_webContents->GetRenderViewHost()->Cut();
}

void WebViewImpl::copySelection()
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    d_webContents->GetRenderViewHost()->Copy();
}

void WebViewImpl::paste()
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    d_webContents->GetRenderViewHost()->Paste();
}

void WebViewImpl::deleteSelection()
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    d_webContents->GetRenderViewHost()->Delete();
}

void WebViewImpl::enableFocusBefore(bool enabled)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    d_focusBeforeEnabled = enabled;
}

void WebViewImpl::enableFocusAfter(bool enabled)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(!d_wasDestroyed);
    d_focusAfterEnabled = enabled;
}

void WebViewImpl::UpdateTargetURL(content::WebContents* source,
                                  int32 page_id,
                                  const GURL& url)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(source == d_webContents);
    if (d_wasDestroyed) return;
    if (d_delegate)
        d_delegate->updateTargetURL(this, url.spec());
}

void WebViewImpl::DidNavigateMainFramePostCommit(content::WebContents* source)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(source == d_webContents);
    d_isReadyForDelete = true;
    if (d_wasDestroyed) {
        if (!d_isDeletingSoon) {
            d_isDeletingSoon = true;
            MessageLoop::current()->DeleteSoon(FROM_HERE, this);
        }
        return;
    }
    if (d_delegate)
        d_delegate->didNavigateMainFramePostCommit(this, source->GetURL().spec());
}

bool WebViewImpl::TakeFocus(content::WebContents* source, bool reverse)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(source == d_webContents);
    if (d_wasDestroyed || !d_delegate) return false;
    if (reverse) {
        if (d_focusBeforeEnabled) {
            d_delegate->focusBefore(this);
            return true;
        }
        return false;
    }
    if (d_focusAfterEnabled) {
        d_delegate->focusAfter(this);
        return true;
    }
    return false;
}

void WebViewImpl::WebContentsFocused(content::WebContents* contents)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(contents == d_webContents);
    if (d_wasDestroyed) return;
    if (d_delegate)
        d_delegate->focused(this);
}

void WebViewImpl::WebContentsCreated(content::WebContents* source_contents,
                                     int64 source_frame_id,
                                     const string16& frame_name,
                                     const GURL& target_url,
                                     const content::ContentCreatedParams& params,
                                     content::WebContents* new_contents)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(source_contents == d_webContents);
    WebViewImpl* newView = new WebViewImpl(new_contents);
    if (d_wasDestroyed || !d_delegate) {
        newView->destroy();
        return;
    }

    NewViewParams delegateParams;
    switch (params.disposition) {
    case SAVE_TO_DISK:
        delegateParams.setDisposition(NewViewDisposition::DOWNLOAD);
        break;
    case CURRENT_TAB:
        delegateParams.setDisposition(NewViewDisposition::CURRENT_TAB);
        break;
    case NEW_BACKGROUND_TAB:
        delegateParams.setDisposition(NewViewDisposition::NEW_BACKGROUND_TAB);
        break;
    case NEW_FOREGROUND_TAB:
        delegateParams.setDisposition(NewViewDisposition::NEW_FOREGROUND_TAB);
        break;
    case NEW_POPUP:
        delegateParams.setDisposition(NewViewDisposition::NEW_POPUP);
        break;
    case NEW_WINDOW:
    default:
        delegateParams.setDisposition(NewViewDisposition::NEW_WINDOW);
        break;
    }
    if (params.x_set) delegateParams.setX(params.x);
    if (params.y_set) delegateParams.setY(params.y);
    if (params.width_set) delegateParams.setWidth(params.width);
    if (params.height_set) delegateParams.setHeight(params.height);
    delegateParams.setTargetUrl(target_url.spec());
    delegateParams.setIsHidden(params.hidden);
    delegateParams.setIsTopMost(params.topmost);
    delegateParams.setIsNoFocus(params.nofocus);

    d_delegate->didCreateNewView(this,
                                 newView,
                                 delegateParams,
                                 &newView->d_delegate);
}

void WebViewImpl::CloseContents(content::WebContents* source)
{
    DCHECK(Statics::isInBrowserMainThread());
    DCHECK(source == d_webContents);
    if (d_wasDestroyed) return;
    if (!d_delegate) {
        destroy();
        return;
    }

    d_delegate->destroyView(this);
}

}  // close namespace blpwtk2

