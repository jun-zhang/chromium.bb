/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebDocument.h"
#include "WebElement.h"
#include "core/css/CSSStyleDeclaration.h"
#include "core/dom/Element.h"
#include "core/dom/NamedNodeMap.h"
#include "core/dom/NodeTraversal.h"
#include "core/dom/Position.h"
#include "core/dom/Range.h"
#include "core/dom/ShadowRoot.h"
#include "core/editing/Editor.h"
#include "core/editing/SpellChecker.h"
#include "core/html/HTMLElement.h"
#include "core/html/HTMLFrameOwnerElement.h"
#include "core/html/HTMLTextFormControlElement.h"
#include "core/html/DOMTokenList.h"
#include "core/page/Frame.h"
#include "core/rendering/RenderBoxModelObject.h"
#include "core/rendering/RenderObject.h"
#include "core/platform/text/TextChecking.h"
#include <public/WebRect.h>
#include <wtf/PassRefPtr.h>
#include <bindings/V8Element.h>


using namespace WebCore;

namespace WebKit {

bool WebElement::isFormControlElement() const
{
    return constUnwrap<Element>()->isFormControlElement();
}

bool WebElement::isTextFormControlElement() const
{
    return constUnwrap<Element>()->isTextFormControl();
}

WebString WebElement::tagName() const
{
    return constUnwrap<Element>()->tagName();
}

bool WebElement::hasTagName(const WebString& tagName) const
{
    return equalIgnoringCase(constUnwrap<Element>()->tagName(),
                             tagName.operator String());
}

bool WebElement::hasHTMLTagName(const WebString& tagName) const
{
    // How to create                     class              nodeName localName
    // createElement('input')            HTMLInputElement   INPUT    input
    // createElement('INPUT')            HTMLInputElement   INPUT    input
    // createElementNS(xhtmlNS, 'input') HTMLInputElement   INPUT    input
    // createElementNS(xhtmlNS, 'INPUT') HTMLUnknownElement INPUT    INPUT
    const Element* element = constUnwrap<Element>();
    return HTMLNames::xhtmlNamespaceURI == element->namespaceURI() && element->localName() == String(tagName).lower();
}

bool WebElement::hasAttribute(const WebString& attrName) const
{
    return constUnwrap<Element>()->hasAttribute(attrName);
}

void WebElement::removeAttribute(const WebString& attrName)
{
    unwrap<Element>()->removeAttribute(attrName);
}

WebString WebElement::getAttribute(const WebString& attrName) const
{
    return constUnwrap<Element>()->getAttribute(attrName);
}

bool WebElement::setAttribute(const WebString& attrName, const WebString& attrValue)
{
    ExceptionCode exceptionCode = 0;
    unwrap<Element>()->setAttribute(attrName, attrValue, exceptionCode);
    return !exceptionCode;
}

unsigned WebElement::attributeCount() const
{
    if (!constUnwrap<Element>()->hasAttributes())
        return 0;
    return constUnwrap<Element>()->attributeCount();
}

WebNode WebElement::shadowRoot() const
{
    Node* shadowRoot = constUnwrap<Element>()->shadowRoot()->toNode();
    return WebNode(shadowRoot);
}

WebString WebElement::attributeName(unsigned index) const
{
    if (index >= attributeCount())
        return WebString();
    return constUnwrap<Element>()->attributeItem(index)->name().toString();
}

WebString WebElement::attributeLocalName(unsigned index) const
{
    if (index >= attributeCount())
        return WebString();
    return constUnwrap<Element>()->attributeItem(index)->localName();
}

WebString WebElement::attributeValue(unsigned index) const
{
    if (index >= attributeCount())
        return WebString();
    return constUnwrap<Element>()->attributeItem(index)->value();
}

WebString WebElement::innerText()
{
    return unwrap<Element>()->innerText();
}

WebString WebElement::computeInheritedLanguage() const
{
    return WebString(constUnwrap<Element>()->computeInheritedLanguage());
}

void WebElement::requestFullScreen()
{
    unwrap<Element>()->webkitRequestFullScreen(Element::ALLOW_KEYBOARD_INPUT);
}

WebDocument WebElement::document() const
{
    return WebDocument(constUnwrap<Element>()->document());
}

WebRect WebElement::boundsInViewportSpace()
{
    return unwrap<Element>()->boundsInRootViewSpace();
}

bool WebElement::setCssProperty(const WebString& name, const WebString& value, const WebString& priority)
{
    ExceptionCode exceptionCode = 0;
    unwrap<Element>()->style()->setProperty(name, value, priority, exceptionCode);
    return !exceptionCode;
}

bool WebElement::removeCssProperty(const WebString& name)
{
    ExceptionCode exceptionCode = 0;
    unwrap<Element>()->style()->removeProperty(name, exceptionCode);
    return !exceptionCode;
}

bool WebElement::addClass(const WebString& name)
{
    ExceptionCode exceptionCode = 0;
    unwrap<Element>()->classList()->add(name, exceptionCode);
    return !exceptionCode;
}

bool WebElement::removeClass(const WebString& name)
{
    ExceptionCode exceptionCode = 0;
    unwrap<Element>()->classList()->remove(name, exceptionCode);
    return !exceptionCode;
}

bool WebElement::containsClass(const WebString& name)
{
    ExceptionCode exceptionCode = 0;
    return unwrap<Element>()->classList()->contains(name, exceptionCode);
}

bool WebElement::toggleClass(const WebString& name)
{
    ExceptionCode exceptionCode = 0;
    unwrap<Element>()->classList()->toggle(name, exceptionCode);
    return !exceptionCode;
}

WebString WebElement::innerHTML() const
{
    const WebCore::Element* webCoreElemPtr = constUnwrap<Element>();
    const WebCore::HTMLElement* htmlElemPtr =(const WebCore::HTMLElement*) webCoreElemPtr;
    if (htmlElemPtr) {
        return htmlElemPtr->innerHTML();
    }
    return WebString();
}

void WebElement::requestSpellCheck()
{
    Element* element = unwrap<Element>();
    if (!element ||
        !element->document() ||
        !element->document()->frame() ||
        !element->document()->frame()->editor() ||
        !element->document()->frame()->editor()->isContinuousSpellCheckingEnabled() ||
        !element->document()->frame()->editor()->spellChecker()) {
        return;
    }

    SpellChecker* spellChecker = element->document()->frame()->editor()->spellChecker();
    Node* stayWithin = element;
    while (element) {
        if (element->isFrameOwnerElement()) {
            Document* contentDocument = toFrameOwnerElement(element)->contentDocument();
            if (contentDocument && contentDocument->documentElement()) {
                WebElement documentElement = contentDocument->documentElement();
                documentElement.requestSpellCheck();
            }
            element = ElementTraversal::nextSkippingChildren(element, stayWithin);
        }
        else if (element->isTextFormControl()) {
            HTMLElement* innerElement = toHTMLTextFormControlElement(element)->innerTextElement();
            if (innerElement && innerElement->rendererIsEditable()) {
                RefPtr<Range> rangeToCheck = Range::create(innerElement->document(), firstPositionInNode(innerElement), lastPositionInNode(innerElement));
                spellChecker->requestCheckingFor(SpellCheckRequest::create(TextCheckingTypeSpelling | TextCheckingTypeGrammar, TextCheckingProcessBatch, rangeToCheck, rangeToCheck));
            }
            element = ElementTraversal::nextSkippingChildren(element, stayWithin);
        }
        else if (element->rendererIsEditable()) {
            RefPtr<Range> rangeToCheck = Range::create(element->document(), firstPositionInNode(element), lastPositionInNode(element));
            spellChecker->requestCheckingFor(SpellCheckRequest::create(TextCheckingTypeSpelling | TextCheckingTypeGrammar, TextCheckingProcessBatch, rangeToCheck, rangeToCheck));
            element = ElementTraversal::nextSkippingChildren(element, stayWithin);
        }
        else {
            element = ElementTraversal::next(element, stayWithin);
        }
    }
}

bool WebElement::isWebElement(v8::Handle<v8::Value> handle)
{
    if (!handle->IsObject()) {
        return false;
    }
    v8::TryCatch tryCatch;
    v8::Handle<v8::Object> obj = handle->ToObject();
    if (!WebCore::V8Element::HasInstance(obj, obj->CreationContext()->GetIsolate(), WebCore::MainWorld)) {
        return false;
    }
    WebCore::V8Element::toNative(obj);
    return !tryCatch.HasCaught();
}

WebElement WebElement::fromV8Handle(v8::Handle<v8::Value> handle)
{
    return WebCore::V8Element::toNative(handle->ToObject());
}

WebElement::WebElement(const PassRefPtr<Element>& elem)
    : WebNode(elem)
{
}

WebElement& WebElement::operator=(const PassRefPtr<Element>& elem)
{
    m_private = elem;
    return *this;
}

WebElement::operator PassRefPtr<Element>() const
{
    return toElement(m_private.get());
}

} // namespace WebKit
