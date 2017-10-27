#ifndef REFLECTIVE_RAPIDJSON_JSON_REFLECTOR_H
#define REFLECTIVE_RAPIDJSON_JSON_REFLECTOR_H

/*!
 * \file jsonreflector.h
 * \brief Contains functions to (de)serialize basic types such as int, double, bool, std::string,
 *        std::vector, ... with RapidJSON.
 */

#include <c++utilities/conversion/types.h>
#include <c++utilities/misc/traits.h>

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <list>
#include <string>
#include <vector>

namespace ReflectiveRapidJSON {

enum class ErrorFlags : unsigned char { TypeMismatch, MemberMissing };

constexpr ErrorFlags operator&(ErrorFlags lhs, ErrorFlags rhs)
{
    return static_cast<ErrorFlags>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs));
}

constexpr ErrorFlags operator|(ErrorFlags lhs, ErrorFlags rhs)
{
    return static_cast<ErrorFlags>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs));
}

template <typename Type> struct JSONSerializable;

inline RAPIDJSON_NAMESPACE::StringBuffer documentToString(RAPIDJSON_NAMESPACE::Document &document)
{
    RAPIDJSON_NAMESPACE::StringBuffer buffer;
    RAPIDJSON_NAMESPACE::Writer<RAPIDJSON_NAMESPACE::StringBuffer> writer(buffer);
    document.Accept(writer);
    return buffer;
}

inline RAPIDJSON_NAMESPACE::Document parseDocumentFromString(const char *json, std::size_t jsonSize)
{
    RAPIDJSON_NAMESPACE::Document document(RAPIDJSON_NAMESPACE::kObjectType);
    const RAPIDJSON_NAMESPACE::ParseResult parseRes = document.Parse(json, jsonSize);
    if (parseRes.IsError()) {
        throw parseRes;
    }
    return document;
}

namespace Reflector {

// define functions to "push" values to a RapidJSON array or object

template <typename Type,
    Traits::DisableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>,
        Traits::All<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>>...>
void push(const Type &reflectable, RAPIDJSON_NAMESPACE::Value::Array &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator);

template <typename Type,
    Traits::DisableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>,
        Traits::All<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>>...>
void push(const Type &reflectable, RAPIDJSON_NAMESPACE::Value::Object &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator);

template <typename Type, Traits::EnableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>>...>
inline void push(Type reflectable, RAPIDJSON_NAMESPACE::Value::Array &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.PushBack(reflectable, allocator);
}

template <>
inline void push<const char *>(
    const char *reflectable, RAPIDJSON_NAMESPACE::Value::Array &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.PushBack(RAPIDJSON_NAMESPACE::StringRef(reflectable), allocator);
}

template <>
inline void push<std::string>(
    const std::string &reflectable, RAPIDJSON_NAMESPACE::Value::Array &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.PushBack(RAPIDJSON_NAMESPACE::StringRef(reflectable.data(), reflectable.size()), allocator);
}

template <>
inline void push<const char *const &>(
    const char *const &reflectable, RAPIDJSON_NAMESPACE::Value::Array &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.PushBack(RAPIDJSON_NAMESPACE::StringRef(reflectable), allocator);
}

template <typename Type, Traits::EnableIf<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void push(const Type &reflectable, RAPIDJSON_NAMESPACE::Value::Array &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    RAPIDJSON_NAMESPACE::Value arrayValue(RAPIDJSON_NAMESPACE::kArrayType);
    RAPIDJSON_NAMESPACE::Value::Array array(arrayValue.GetArray());
    array.Reserve(reflectable.size(), allocator);
    for (const auto &item : reflectable) {
        push<decltype(item)>(item, array, allocator);
    }
    value.PushBack(array, allocator);
}

template <typename Type,
    Traits::DisableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>,
        Traits::All<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>>...>
inline void push(
    const Type &reflectable, const char *name, RAPIDJSON_NAMESPACE::Value::Object &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    RAPIDJSON_NAMESPACE::Value objectValue(RAPIDJSON_NAMESPACE::kObjectType);
    RAPIDJSON_NAMESPACE::Value::Object object(objectValue.GetObject());
    push(reflectable, object, allocator);
    value.AddMember(RAPIDJSON_NAMESPACE::StringRef(name), object, allocator);
}

template <typename Type, Traits::EnableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>>...>
inline void push(
    Type reflectable, const char *name, RAPIDJSON_NAMESPACE::Value::Object &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.AddMember(RAPIDJSON_NAMESPACE::StringRef(name), reflectable, allocator);
}

template <>
inline void push<std::string>(const std::string &reflectable, const char *name, RAPIDJSON_NAMESPACE::Value::Object &value,
    RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.AddMember(RAPIDJSON_NAMESPACE::StringRef(name), RAPIDJSON_NAMESPACE::StringRef(reflectable.data(), reflectable.size()), allocator);
}

template <>
inline void push<const char *>(
    const char *reflectable, const char *name, RAPIDJSON_NAMESPACE::Value::Object &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.AddMember(RAPIDJSON_NAMESPACE::StringRef(name), RAPIDJSON_NAMESPACE::StringRef(reflectable), allocator);
}

template <>
inline void push<const char *const &>(const char *const &reflectable, const char *name, RAPIDJSON_NAMESPACE::Value::Object &value,
    RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.AddMember(RAPIDJSON_NAMESPACE::StringRef(name), RAPIDJSON_NAMESPACE::StringRef(reflectable), allocator);
}

template <typename Type,
    Traits::EnableIf<Traits::IsIteratable<Type>, Traits::Not<Traits::HasSize<Type>>,
        Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void push(
    const Type &reflectable, const char *name, RAPIDJSON_NAMESPACE::Value::Object &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    RAPIDJSON_NAMESPACE::Value arrayValue(RAPIDJSON_NAMESPACE::kArrayType);
    RAPIDJSON_NAMESPACE::Value::Array array(arrayValue.GetArray());
    for (const auto &item : reflectable) {
        push(item, array, allocator);
    }
    value.AddMember(RAPIDJSON_NAMESPACE::StringRef(name), array, allocator);
}

template <typename Type,
    Traits::EnableIf<Traits::IsIteratable<Type>, Traits::HasSize<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void push(
    const Type &reflectable, const char *name, RAPIDJSON_NAMESPACE::Value::Object &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    RAPIDJSON_NAMESPACE::Value arrayValue(RAPIDJSON_NAMESPACE::kArrayType);
    RAPIDJSON_NAMESPACE::Value::Array array(arrayValue.GetArray());
    array.Reserve(reflectable.size(), allocator);
    for (const auto &item : reflectable) {
        push(item, array, allocator);
    }
    value.AddMember(RAPIDJSON_NAMESPACE::StringRef(name), array, allocator);
}

template <typename Type,
    Traits::DisableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>,
        Traits::All<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>>...>
void push(const Type &reflectable, RAPIDJSON_NAMESPACE::Value::Array &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    RAPIDJSON_NAMESPACE::Value objectValue(RAPIDJSON_NAMESPACE::kObjectType);
    RAPIDJSON_NAMESPACE::Value::Object object(objectValue.GetObject());
    push(reflectable, object, allocator);
    value.PushBack(objectValue, allocator);
}

// define functions to "pull" values from a RapidJSON array or object

template <typename Type,
    Traits::DisableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>,
        Traits::All<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>>...>
void pull(Type &reflectable, RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ValueIterator &value);

template <typename Type,
    Traits::DisableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>,
        Traits::All<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>>...>
void pull(Type &reflectable, const RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ConstObject &value);

template <typename Type,
    Traits::DisableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>,
        Traits::All<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>>...>
void pull(Type &reflectable, const RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>> &value)
{
    if (!value.IsObject()) {
        return; // TODO: handle type mismatch
    }
    pull<Type>(reflectable, value.GetObject());
}

template <typename Type, Traits::EnableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>>...>
inline void pull(Type &reflectable, RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ValueIterator &value)
{
    if (!value->Is<Type>()) {
        return; // TODO: handle type mismatch
    }
    reflectable = value->Get<Type>();
    ++value;
}

template <typename Type, Traits::EnableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>>...>
inline void pull(Type &reflectable, const RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>> &value)
{
    if (!value.Is<Type>()) {
        return; // TODO: handle type mismatch
    }
    reflectable = value.Get<Type>();
}

template <>
inline void pull<std::string>(std::string &reflectable, RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ValueIterator &value)
{
    if (!value->IsString()) {
        return; // TODO: handle type mismatch
    }
    reflectable = value->GetString();
    ++value;
}

template <> inline void pull<std::string>(std::string &reflectable, const RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>> &value)
{
    if (!value.IsString()) {
        return; // TODO: handle type mismatch
    }
    reflectable = value.GetString();
}

template <typename Type, Traits::EnableIf<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void pull(Type &reflectable, rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ConstArray array);

template <typename Type,
    Traits::EnableIf<Traits::IsIteratable<Type>, Traits::Not<Traits::IsReservable<Type>>,
        Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void pull(Type &reflectable, rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ValueIterator &value)
{
    if (!value->IsArray()) {
        return; // TODO: handle type mismatch
    }
    pull(reflectable, value->GetArray());
    ++value;
}

template <typename Type,
    Traits::EnableIf<Traits::IsIteratable<Type>, Traits::IsReservable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void pull(Type &reflectable, rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ValueIterator &value)
{
    if (!value->IsArray()) {
        return; // TODO: handle type mismatch
    }
    auto array = value->GetArray();
    reflectable.reserve(array.Size());
    pull(reflectable, array);
    ++value;
}

template <typename Type,
    Traits::EnableIf<Traits::IsIteratable<Type>, Traits::Not<Traits::IsReservable<Type>>,
        Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void pull(Type &reflectable, const rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>> &value)
{
    if (!value.IsArray()) {
        return; // TODO: handle type mismatch
    }
    pull(reflectable, value.GetArray());
}

template <typename Type,
    Traits::EnableIf<Traits::IsIteratable<Type>, Traits::IsReservable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void pull(Type &reflectable, const rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>> &value)
{
    if (!value.IsArray()) {
        return; // TODO: handle type mismatch
    }
    auto array = value.GetArray();
    reflectable.reserve(array.Size());
    pull(reflectable, array);
}

template <typename Type, Traits::EnableIf<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void pull(Type &reflectable, rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ConstArray array)
{
    // clear previous contents of the array (TODO: make this configurable)
    reflectable.clear();
    // pull all array elements of the specified value
    for (const rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>> &item : array) {
        reflectable.emplace_back();
        pull(reflectable.back(), item);
    }
}

template <typename Type>
inline void pull(Type &reflectable, const char *name, const rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ConstObject &value)
{
    auto member = value.FindMember(name);
    if (member == value.MemberEnd()) {
        return; // TODO: handle member missing
    }
    pull<Type>(reflectable, value.FindMember(name)->value);
}

// define functions providing high-level JSON serialization

template <typename Type, Traits::EnableIfAny<std::is_base_of<JSONSerializable<Type>, Type>>...>
RAPIDJSON_NAMESPACE::StringBuffer toJson(const Type &reflectable)
{
    RAPIDJSON_NAMESPACE::Document document(RAPIDJSON_NAMESPACE::kObjectType);
    RAPIDJSON_NAMESPACE::Document::Object object(document.GetObject());
    push(reflectable, object, document.GetAllocator());
    return documentToString(document);
}

template <typename Type, Traits::EnableIfAny<std::is_integral<Type>, std::is_floating_point<Type>>...>
RAPIDJSON_NAMESPACE::StringBuffer toJson(Type reflectable)
{
    RAPIDJSON_NAMESPACE::Document document(RAPIDJSON_NAMESPACE::kNumberType);
    document.Set(reflectable, document.GetAllocator());
    return documentToString(document);
}

template <typename Type, Traits::EnableIfAny<std::is_same<Type, std::string>>...>
RAPIDJSON_NAMESPACE::StringBuffer toJson(const std::string &reflectable)
{
    RAPIDJSON_NAMESPACE::Document document(RAPIDJSON_NAMESPACE::kStringType);
    document.SetString(RAPIDJSON_NAMESPACE::StringRef(reflectable.data(), reflectable.size()), document.GetAllocator());
    return documentToString(document);
}

template <typename Type, Traits::EnableIfAny<std::is_same<Type, const char *>>...> RAPIDJSON_NAMESPACE::StringBuffer toJson(const char *reflectable)
{
    RAPIDJSON_NAMESPACE::Document document(RAPIDJSON_NAMESPACE::kStringType);
    document.SetString(RAPIDJSON_NAMESPACE::StringRef(reflectable), document.GetAllocator());
    return documentToString(document);
}

// define functions providing high-level JSON deserialization

template <typename Type, Traits::EnableIfAny<std::is_base_of<JSONSerializable<Type>, Type>>...> Type fromJson(const char *json, std::size_t jsonSize)
{
    RAPIDJSON_NAMESPACE::Document doc(parseDocumentFromString(json, jsonSize));
    if (!doc.IsObject()) {
        return Type();
    }

    Type res;
    pull<Type>(res, doc.GetObject());
    return res;
}

template <typename Type, Traits::EnableIfAny<std::is_integral<Type>, std::is_floating_point<Type>>...>
Type fromJson(const char *json, std::size_t jsonSize)
{
    RAPIDJSON_NAMESPACE::Document doc(parseDocumentFromString(json, jsonSize));
    if (!doc.Is<Type>()) {
        return Type();
    }

    return doc.Get<Type>();
}

template <typename Type, Traits::EnableIfAny<std::is_same<Type, std::string>>...> Type fromJson(const char *json, std::size_t jsonSize)
{
    RAPIDJSON_NAMESPACE::Document doc(parseDocumentFromString(json, jsonSize));
    if (!doc.IsString()) {
        return Type();
    }

    return doc.GetString();
}

template <typename Type> Type fromJson(const std::string &json)
{
    return fromJson<Type>(json.data(), json.size());
}

} // namespace Reflector
} // namespace ReflectiveRapidJSON

#endif // REFLECTIVE_RAPIDJSON_JSON_REFLECTOR_H
