// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by {{code_generator}}. DO NOT MODIFY!

#include "config.h"
#include "{{header_filename}}"

{% macro assign_and_return_if_hasinstance(member) %}
if (V8{{member.type_name}}::hasInstance(v8Value, isolate)) {
    {{member.cpp_local_type}} cppValue = V8{{member.type_name}}::toImpl(v8::Local<v8::Object>::Cast(v8Value));
    impl.set{{member.type_name}}(cppValue);
    return;
}
{% endmacro %}
{% for filename in cpp_includes %}
#include "{{filename}}"
{% endfor %}

namespace blink {

{% for container in containers %}
{{container.cpp_class}}::{{container.cpp_class}}()
    : m_type(SpecificTypeNone)
{
}

{% for member in container.members %}
{{member.rvalue_cpp_type}} {{container.cpp_class}}::getAs{{member.type_name}}() const
{
    ASSERT(is{{member.type_name}}());
    return m_{{member.cpp_name}};
}

void {{container.cpp_class}}::set{{member.type_name}}({{member.rvalue_cpp_type}} value)
{
    ASSERT(isNull());
    {% if member.enum_validation_expression %}
    String string = value;
    if (!({{member.enum_validation_expression}})) {
        ASSERT_NOT_REACHED();
        return;
    }
    {% endif %}
    m_{{member.cpp_name}} = value;
    m_type = {{member.specific_type_enum}};
}

{{container.cpp_class}} {{container.cpp_class}}::from{{member.type_name}}({{member.rvalue_cpp_type}} value)
{
    {{container.cpp_class}} container;
    container.set{{member.type_name}}(value);
    return container;
}

{% endfor %}
{% if container.needs_trace %}
void {{container.cpp_class}}::trace(Visitor* visitor)
{
    {% for member in container.members if member.is_traceable %}
    visitor->trace(m_{{member.cpp_name}});
    {% endfor %}
}

{% endif %}
void V8{{container.cpp_class}}::toImpl(v8::Isolate* isolate, v8::Local<v8::Value> v8Value, {{container.cpp_class}}& impl, ExceptionState& exceptionState)
{
    if (v8Value.IsEmpty())
        return;

    {# The numbers in the following comments refer to the steps described in
       http://heycam.github.io/webidl/#es-union
       NOTE: Step 1 (null or undefined) is handled in *OrNull::toImpl()
       FIXME: Implement all necessary steps #}
    {# 3. Platform objects (interfaces) #}
    {% for interface in container.interface_types %}
    {{assign_and_return_if_hasinstance(interface) | indent}}

    {% endfor %}
    {# 8. ArrayBuffer #}
    {% if container.array_buffer_type %}
    {{assign_and_return_if_hasinstance(container.array_buffer_type) | indent}}

    {% endif %}
    {# 9., 10. ArrayBufferView #}
    {# FIXME: Individual typed arrays (e.g. Uint8Array) aren't supported yet. #}
    {% if container.array_buffer_view_type %}
    {{assign_and_return_if_hasinstance(container.array_buffer_view_type) | indent}}

    {% endif %}
    {% if container.dictionary_type %}
    {# 12. Dictionaries #}
    {# FIXME: This should also check "object but not Date or RegExp". Add checks
       when we implement conversions for Date and RegExp. #}
    if (isUndefinedOrNull(v8Value) || v8Value->IsObject()) {
        {% if container.dictionary_type.type_name != 'Dictionary' %}
        {{container.dictionary_type.cpp_local_type}} cppValue;
        {% endif %}
        {{container.dictionary_type.v8_value_to_local_cpp_value}};
        impl.set{{container.dictionary_type.type_name}}(cppValue);
        return;
    }

    {% endif %}
    {% if container.array_or_sequence_type %}
    {# 13. Arrays/Sequences #}
    {# FIXME: This should also check "object but not Date or RegExp". Add checks
       when we implement conversions for Date and RegExp. #}
    {# FIXME: Should check for sequences too, not just Array instances. #}
    if (v8Value->IsArray()) {
        {{container.array_or_sequence_type.v8_value_to_local_cpp_value}};
        impl.set{{container.array_or_sequence_type.type_name}}(cppValue);
        return;
    }

    {% endif %}
    {# FIXME: In some cases, we can omit boolean and numeric type checks because
       we have fallback conversions. (step 17 and 18) #}
    {% if container.boolean_type %}
    {# 14. Boolean #}
    if (v8Value->IsBoolean()) {
        impl.setBoolean(v8Value->ToBoolean()->Value());
        return;
    }

    {% endif %}
    {% if container.numeric_type %}
    {# 15. Number #}
    if (v8Value->IsNumber()) {
        {{container.numeric_type.v8_value_to_local_cpp_value}};
        impl.set{{container.numeric_type.type_name}}(cppValue);
        return;
    }

    {% endif %}
    {% if container.string_type %}
    {# 16. String #}
    {
        {{container.string_type.v8_value_to_local_cpp_value}};
        {% if container.string_type.enum_validation_expression %}
        String string = cppValue;
        if (!({{container.string_type.enum_validation_expression}})) {
            exceptionState.throwTypeError("'" + string + "' is not a valid enum value.");
            return;
        }
        {% endif %}
        impl.set{{container.string_type.type_name}}(cppValue);
        return;
    }

    {# 17. Number (fallback) #}
    {% elif container.numeric_type %}
    {
        {{container.numeric_type.v8_value_to_local_cpp_value}};
        impl.set{{container.numeric_type.type_name}}(cppValue);
        return;
    }

    {# 18. Boolean (fallback) #}
    {% elif container.boolean_type %}
    {
        impl.setBoolean(v8Value->ToBoolean()->Value());
        return;
    }

    {% else %}
    {# 19. TypeError #}
    exceptionState.throwTypeError("The provided value is not of type '{{container.type_string}}'");
    {% endif %}
}

v8::Local<v8::Value> toV8(const {{container.cpp_class}}& impl, v8::Local<v8::Object> creationContext, v8::Isolate* isolate)
{
    switch (impl.m_type) {
    case {{container.cpp_class}}::SpecificTypeNone:
        {# FIXME: We might want to return undefined in some cases #}
        return v8::Null(isolate);
    {% for member in container.members %}
    case {{container.cpp_class}}::{{member.specific_type_enum}}:
        return {{member.cpp_value_to_v8_value}};
    {% endfor %}
    default:
        ASSERT_NOT_REACHED();
    }
    return v8::Local<v8::Value>();
}

{{container.cpp_class}} NativeValueTraits<{{container.cpp_class}}>::nativeValue(const v8::Local<v8::Value>& value, v8::Isolate* isolate, ExceptionState& exceptionState)
{
    {{container.cpp_class}} impl;
    V8{{container.cpp_class}}::toImpl(isolate, value, impl, exceptionState);
    return impl;
}

{% endfor %}
} // namespace blink
