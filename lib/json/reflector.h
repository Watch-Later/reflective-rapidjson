#ifndef REFLECTIVE_RAPIDJSON_JSON_REFLECTOR_H
#define REFLECTIVE_RAPIDJSON_JSON_REFLECTOR_H

/*!
 * \file reflector.h
 * \brief Contains functions to (de)serialize basic types such as int, double, bool, std::string,
 *        std::vector, ... with RapidJSON.
 */

#include <c++utilities/conversion/types.h>
#include <c++utilities/misc/traits.h>

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <string>

#include "./errorhandling.h"

namespace ReflectiveRapidJSON {

template <typename Type> struct JsonSerializable;

/*!
 * \brief Serializes the specified JSON \a document.
 */
inline RAPIDJSON_NAMESPACE::StringBuffer serializeJsonDocToString(RAPIDJSON_NAMESPACE::Document &document)
{
    RAPIDJSON_NAMESPACE::StringBuffer buffer;
    RAPIDJSON_NAMESPACE::Writer<RAPIDJSON_NAMESPACE::StringBuffer> writer(buffer);
    document.Accept(writer);
    return buffer;
}

/*!
 * \brief Parses the specified JSON string.
 */
inline RAPIDJSON_NAMESPACE::Document parseJsonDocFromString(const char *json, std::size_t jsonSize)
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

/*!
 * \brief Pushes the \a reflectable which has a custom type to the specified array.
 */
template <typename Type,
    Traits::DisableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>,
        Traits::All<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>>...>
void push(const Type &reflectable, RAPIDJSON_NAMESPACE::Value::Array &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator);

/*!
 * \brief Pushes the \a reflectable which has a custom type to the specified object.
 * \remarks The definition of this function must be provided by the code generator or Boost.Hana.
 */
template <typename Type,
    Traits::DisableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>,
        Traits::All<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>>...>
void push(const Type &reflectable, RAPIDJSON_NAMESPACE::Value::Object &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator);

/*!
 * \brief Pushes the specified integer/float/boolean to the specified array.
 */
template <typename Type, Traits::EnableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>>...>
inline void push(Type reflectable, RAPIDJSON_NAMESPACE::Value::Array &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.PushBack(reflectable, allocator);
}

/*!
 * \brief Pushes the specified C-string to the specified array.
 */
template <>
inline void push<const char *>(
    const char *reflectable, RAPIDJSON_NAMESPACE::Value::Array &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.PushBack(RAPIDJSON_NAMESPACE::StringRef(reflectable), allocator);
}

/*!
 * \brief Pushes the specified constant C-string to the specified array.
 */
template <>
inline void push<const char *const &>(
    const char *const &reflectable, RAPIDJSON_NAMESPACE::Value::Array &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.PushBack(RAPIDJSON_NAMESPACE::StringRef(reflectable), allocator);
}

/*!
 * \brief Pushes the specified std::string to the specified array.
 */
template <>
inline void push<std::string>(
    const std::string &reflectable, RAPIDJSON_NAMESPACE::Value::Array &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.PushBack(RAPIDJSON_NAMESPACE::StringRef(reflectable.data(), reflectable.size()), allocator);
}

/*!
 * \brief Pushes the specified iteratable (eg. std::vector, std::list) to the specified array.
 */
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

/*!
 * \brief Pushes the specified \a reflectable which has a custom type as member to the specified object.
 */
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

/*!
 * \brief Pushes the specified integer/float/boolean as member to the specified object.
 */
template <typename Type, Traits::EnableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>>...>
inline void push(
    Type reflectable, const char *name, RAPIDJSON_NAMESPACE::Value::Object &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.AddMember(RAPIDJSON_NAMESPACE::StringRef(name), reflectable, allocator);
}

/*!
 * \brief Pushes the specified C-string as member to the specified object.
 */
template <>
inline void push<const char *>(
    const char *reflectable, const char *name, RAPIDJSON_NAMESPACE::Value::Object &value, RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.AddMember(RAPIDJSON_NAMESPACE::StringRef(name), RAPIDJSON_NAMESPACE::StringRef(reflectable), allocator);
}

/*!
 * \brief Pushes the specified constant C-string as member to the specified object.
 */
template <>
inline void push<const char *const &>(const char *const &reflectable, const char *name, RAPIDJSON_NAMESPACE::Value::Object &value,
    RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.AddMember(RAPIDJSON_NAMESPACE::StringRef(name), RAPIDJSON_NAMESPACE::StringRef(reflectable), allocator);
}

/*!
 * \brief Pushes the specified std::string as member to the specified object.
 */
template <>
inline void push<std::string>(const std::string &reflectable, const char *name, RAPIDJSON_NAMESPACE::Value::Object &value,
    RAPIDJSON_NAMESPACE::Document::AllocatorType &allocator)
{
    value.AddMember(RAPIDJSON_NAMESPACE::StringRef(name), RAPIDJSON_NAMESPACE::StringRef(reflectable.data(), reflectable.size()), allocator);
}

/*!
 * \brief Pushes the specified iteratable without size() method as member to the specified object.
 */
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

/*!
 * \brief Pushes the specified iteratable with size() method (eg. std::vector, std::list) as member to the specified object.
 */
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

/*!
 * \brief Pushes the \a reflectable which has a custom type to the specified array.
 */
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

/*!
 * \brief Pulls the \a reflectable which has a custom type from the specified value.
 */
template <typename Type,
    Traits::DisableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>,
        Traits::All<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>>...>
void pull(
    Type &reflectable, RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ValueIterator &value, JsonDeserializationErrors *errors);

/*!
 * \brief Pulls the \a reflectable which has a custom type from the specified object.
 * \remarks The definition of this function must be provided by the code generator or Boost.Hana.
 */
template <typename Type,
    Traits::DisableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>,
        Traits::All<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>>...>
void pull(Type &reflectable, const RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ConstObject &value,
    JsonDeserializationErrors *errors);

/*!
 * \brief Pulls the \a reflectable which has a custom type from the specified value which is supposed and checked to contain an object.
 */
template <typename Type,
    Traits::DisableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>,
        Traits::All<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>>...>
void pull(Type &reflectable, const RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>> &value, JsonDeserializationErrors *errors)
{
    if (!value.IsObject()) {
        if (errors) {
            errors->reportTypeMismatch<Type>(value.GetType());
        }
        return;
    }
    pull<Type>(reflectable, value.GetObject(), errors);
}

/*!
 * \brief Pulls the integer/float/boolean from the specified value iterator which is supposed and checked to contain the right type.
 */
template <typename Type, Traits::EnableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>>...>
inline void pull(
    Type &reflectable, RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ValueIterator &value, JsonDeserializationErrors *errors)
{
    if (!value->Is<Type>()) {
        if (errors) {
            errors->reportTypeMismatch<Type>(value->GetType());
        }
        return;
    }
    reflectable = value->Get<Type>();
    ++value;
}

/*!
 * \brief Pulls the integer/float/boolean from the specified value which is supposed and checked to contain the right type.
 */
template <typename Type, Traits::EnableIfAny<std::is_integral<Type>, std::is_floating_point<Type>, std::is_pointer<Type>>...>
inline void pull(
    Type &reflectable, const RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>> &value, JsonDeserializationErrors *errors)
{
    if (!value.Is<Type>()) {
        if (errors) {
            errors->reportTypeMismatch<Type>(value.GetType());
        }
        return;
    }
    reflectable = value.Get<Type>();
}

/*!
 * \brief Pulls the std::string from the specified value iterator which is supposed and checked to contain a string.
 */
template <>
inline void pull<std::string>(std::string &reflectable, RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ValueIterator &value,
    JsonDeserializationErrors *errors)
{
    if (!value->IsString()) {
        if (errors) {
            errors->reportTypeMismatch<std::string>(value->GetType());
        }
        return;
    }
    reflectable = value->GetString();
    ++value;
}

/*!
 * \brief Pulls the std::string from the specified value which is supposed and checked to contain a string.
 */
template <>
inline void pull<std::string>(
    std::string &reflectable, const RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>> &value, JsonDeserializationErrors *errors)
{
    if (!value.IsString()) {
        if (errors) {
            errors->reportTypeMismatch<std::string>(value.GetType());
        }
        return;
    }
    reflectable = value.GetString();
}

/*!
 * \brief Pulls the speciified \a reflectable which is an iteratable from the specified array. The \a reflectable is cleared before.
 */
template <typename Type, Traits::EnableIf<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void pull(Type &reflectable, rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ConstArray array, JsonDeserializationErrors *errors);

/*!
 * \brief Pulls the speciified \a reflectable which is an iteratable without reserve() method from the specified value iterator which is checked to contain an array.
 */
template <typename Type,
    Traits::EnableIf<Traits::IsIteratable<Type>, Traits::Not<Traits::IsReservable<Type>>,
        Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void pull(Type &reflectable, rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ValueIterator &value, JsonDeserializationErrors *errors)
{
    if (!value->IsArray()) {
        if (errors) {
            errors->reportTypeMismatch<Type>(value->GetType());
        }
        return;
    }
    pull(reflectable, value->GetArray(), errors);
    ++value;
}

/*!
 * \brief Pulls the speciified \a reflectable which is an iteratable with reserve() method from the specified value iterator which is checked to contain an array.
 */
template <typename Type,
    Traits::EnableIf<Traits::IsIteratable<Type>, Traits::IsReservable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void pull(Type &reflectable, rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ValueIterator &value, JsonDeserializationErrors *errors)
{
    if (!value->IsArray()) {
        if (errors) {
            errors->reportTypeMismatch<Type>(value->GetType());
        }
        return;
    }
    auto array = value->GetArray();
    reflectable.reserve(array.Size());
    pull(reflectable, array, errors);
    ++value;
}

/*!
 * \brief Pulls the speciified \a reflectable which is an iteratable without reserve() method from the specified value which is checked to contain an array.
 */
template <typename Type,
    Traits::EnableIf<Traits::IsIteratable<Type>, Traits::Not<Traits::IsReservable<Type>>,
        Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void pull(Type &reflectable, const rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>> &value, JsonDeserializationErrors *errors)
{
    if (!value.IsArray()) {
        if (errors) {
            errors->reportTypeMismatch<Type>(value.GetType());
        }
        return;
    }
    pull(reflectable, value.GetArray(), errors);
}

/*!
 * \brief Pulls the speciified \a reflectable which is an iteratable with reserve() method from the specified value which is checked to contain an array.
 */
template <typename Type,
    Traits::EnableIf<Traits::IsIteratable<Type>, Traits::IsReservable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void pull(Type &reflectable, const rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>> &value, JsonDeserializationErrors *errors)
{
    if (!value.IsArray()) {
        if (errors) {
            errors->reportTypeMismatch<Type>(value.GetType());
        }
        return;
    }
    auto array = value.GetArray();
    reflectable.reserve(array.Size());
    pull(reflectable, array, errors);
}

/*!
 * \brief Pulls the speciified \a reflectable which is an iteratable from the specified array. The \a reflectable is cleared before.
 */
template <typename Type, Traits::EnableIf<Traits::IsIteratable<Type>, Traits::Not<Traits::IsSpecializationOf<Type, std::basic_string>>>...>
void pull(Type &reflectable, rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ConstArray array, JsonDeserializationErrors *errors)
{
    // clear previous contents of the array
    reflectable.clear();

    // pull all array elements of the specified value
    std::size_t index = 0;
    for (const rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>> &item : array) {
        // set error context for current index
        if (errors) {
            errors->currentIndex = index;
        }
        reflectable.emplace_back();
        pull(reflectable.back(), item, errors);
        ++index;
    }

    // clear error context
    if (errors) {
        errors->currentIndex = JsonDeserializationError::noIndex;
    }
}

/*!
 * \brief Pulls the speciified member of \a reflectable which has a custom type from the specified object.
 * \remarks It is checked whether the object actually contains the member. If not, the missing member is ignored. So currently all members
 *          are optional.
 */
template <typename Type>
inline void pull(Type &reflectable, const char *name, const rapidjson::GenericValue<RAPIDJSON_NAMESPACE::UTF8<char>>::ConstObject &value,
    JsonDeserializationErrors *errors)
{
    // find member
    auto member = value.FindMember(name);
    if (member == value.MemberEnd()) {
        return; // TODO: handle member missing
    }

    // set error context for current member
    const char *previousMember;
    if (errors) {
        previousMember = errors->currentMember;
        errors->currentMember = name;
    }

    // actually pull value for member
    pull<Type>(reflectable, value.FindMember(name)->value, errors);

    // restore previous error context
    if (errors) {
        errors->currentMember = previousMember;
    }
}

// define functions providing high-level JSON serialization

/*!
 * \brief Serializes the specified \a reflectable which has a custom type.
 */
template <typename Type, Traits::EnableIfAny<std::is_base_of<JsonSerializable<Type>, Type>>...>
RAPIDJSON_NAMESPACE::StringBuffer toJson(const Type &reflectable)
{
    RAPIDJSON_NAMESPACE::Document document(RAPIDJSON_NAMESPACE::kObjectType);
    RAPIDJSON_NAMESPACE::Document::Object object(document.GetObject());
    push(reflectable, object, document.GetAllocator());
    return serializeJsonDocToString(document);
}

/*!
 * \brief Serializes the specified \a reflectable which is an integer, float or boolean.
 */
template <typename Type, Traits::EnableIfAny<std::is_integral<Type>, std::is_floating_point<Type>>...>
RAPIDJSON_NAMESPACE::StringBuffer toJson(Type reflectable)
{
    RAPIDJSON_NAMESPACE::Document document(RAPIDJSON_NAMESPACE::kNumberType);
    document.Set(reflectable, document.GetAllocator());
    return serializeJsonDocToString(document);
}

/*!
 * \brief Serializes the specified \a reflectable which is an std::string.
 */
template <typename Type, Traits::EnableIfAny<std::is_same<Type, std::string>>...>
RAPIDJSON_NAMESPACE::StringBuffer toJson(const std::string &reflectable)
{
    RAPIDJSON_NAMESPACE::Document document(RAPIDJSON_NAMESPACE::kStringType);
    document.SetString(RAPIDJSON_NAMESPACE::StringRef(reflectable.data(), reflectable.size()), document.GetAllocator());
    return serializeJsonDocToString(document);
}

/*!
 * \brief Serializes the specified \a reflectable which is a C-string.
 */
template <typename Type, Traits::EnableIfAny<std::is_same<Type, const char *>>...> RAPIDJSON_NAMESPACE::StringBuffer toJson(const char *reflectable)
{
    RAPIDJSON_NAMESPACE::Document document(RAPIDJSON_NAMESPACE::kStringType);
    document.SetString(RAPIDJSON_NAMESPACE::StringRef(reflectable), document.GetAllocator());
    return serializeJsonDocToString(document);
}

// define functions providing high-level JSON deserialization

/*!
 * \brief Deserializes the specified JSON to \tparam Type which is a custom type.
 */
template <typename Type, Traits::EnableIfAny<std::is_base_of<JsonSerializable<Type>, Type>>...>
Type fromJson(const char *json, std::size_t jsonSize, JsonDeserializationErrors *errors = nullptr)
{
    RAPIDJSON_NAMESPACE::Document doc(parseJsonDocFromString(json, jsonSize));
    if (!doc.IsObject()) {
        if (errors) {
            errors->reportTypeMismatch<Type>(doc.GetType());
        }
        return Type();
    }

    Type res;
    pull<Type>(res, doc.GetObject(), errors);
    return res;
}

/*!
 * \brief Deserializes the specified JSON to \tparam Type which is an integer, float or boolean.
 */
template <typename Type, Traits::EnableIfAny<std::is_integral<Type>, std::is_floating_point<Type>>...>
Type fromJson(const char *json, std::size_t jsonSize, JsonDeserializationErrors *errors)
{
    RAPIDJSON_NAMESPACE::Document doc(parseJsonDocFromString(json, jsonSize));
    if (!doc.Is<Type>()) {
        if (errors) {
            errors->reportTypeMismatch<Type>(doc.GetType());
        }
        return Type();
    }

    return doc.Get<Type>();
}

/*!
 * \brief Deserializes the specified JSON to \tparam Type which is a std::string.
 */
template <typename Type, Traits::EnableIfAny<std::is_same<Type, std::string>>...>
Type fromJson(const char *json, std::size_t jsonSize, JsonDeserializationErrors *errors)
{
    RAPIDJSON_NAMESPACE::Document doc(parseJsonDocFromString(json, jsonSize));
    if (!doc.IsString()) {
        if (errors) {
            errors->reportTypeMismatch<Type>(doc.GetType());
        }
        return Type();
    }

    return doc.GetString();
}

/*!
 * \brief Deserializes the specified JSON from an std::string to \tparam Type.
 */
template <typename Type> Type fromJson(const std::string &json)
{
    return fromJson<Type>(json.data(), json.size());
}

} // namespace Reflector
} // namespace ReflectiveRapidJSON

#endif // REFLECTIVE_RAPIDJSON_JSON_REFLECTOR_H