#pragma once

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <cassert>
#include <fstream>
#include <iostream>
#include "misc.h"
#include "math.h"

#define MAGIC_NUMBER 0x00001111

namespace DataFile
{
    enum DataType : u32
    {
        NONE,
        STRING,
        I64,
        F32,
        BYTE_ARRAY,
        ARRAY,
        DICT,
        BOOL,
    };

    template <typename T>
    class OptionalRef
    {
        T& val_;
        bool hasValue_;
    public:
        OptionalRef(T& val_, bool hasValue_) : val_(val_), hasValue_(hasValue_) {}
        T& val()
        {
            if (!hasValue())
            {
                FATAL_ERROR("Attempt to read invalid datafile value.");
            }
            return val_;
        }
        bool hasValue()
        {
            return hasValue_;
        }
    };

    template <typename T>
    class OptionalVal
    {
        T val_;
        bool hasValue_;
    public:
        OptionalVal(T val_, bool hasValue_) : val_(val_), hasValue_(hasValue_) {}
        T& val()
        {
            if (!hasValue())
            {
                FATAL_ERROR("Attempt to read invalid datafile value.");
            }
            return val_;
        }
        bool hasValue()
        {
            return hasValue_;
        }

        operator bool() { return hasValue(); }
    };

    class Value
    {
    public:
        using Dict = std::map<std::string, Value>;
        using Array = std::vector<Value>;
        using ByteArray = std::vector<u8>;
        using String = std::string;

        friend Value load(const char* filename);

    private:
        DataType dataType = DataType::NONE;
        union
        {
            String str_;
            i64 integer_;
            f32 real_;
            ByteArray bytearray_;
            Array array_;
            Dict dict_;
            bool bool_;
        };

    public:
        static Value readValue(std::ifstream& stream);
        static Value readValue(std::string::const_iterator& ch, std::string::const_iterator end);
        void write(std::ofstream& stream) const;

        ~Value()
        {
            switch (dataType)
            {
                case DataType::NONE:
                case DataType::I64:
                case DataType::F32:
                case DataType::BOOL:
                    break;
                case DataType::STRING:
                    str_.~String();
                    break;
                case DataType::BYTE_ARRAY:
                    bytearray_.~ByteArray();
                    break;
                case DataType::ARRAY:
                    array_.~Array();
                    break;
                case DataType::DICT:
                    dict_.~Dict();
                    break;
            }
        }

        Value() {}
        Value(Value const& other) { *this = other; }
        Value(Value&& other) { *this = std::move(other); }
        explicit Value(std::string&& str)
        {
            dataType = DataType::STRING;
            new (&str_) String(std::move(str));
        }
        explicit Value(std::string const& str)
        {
            dataType = DataType::STRING;
            new (&str_) String(str);
        }
        explicit Value(i64 val)
        {
            dataType = DataType::I64;
            integer_ = val;
        }
        explicit Value(f32 val)
        {
            dataType = DataType::F32;
            real_ = val;
        }
        explicit Value(bool val)
        {
            dataType = DataType::BOOL;
            bool_ = val;
        }

        Value& operator = (Value const& rhs)
        {
            this->~Value();
            switch (rhs.dataType)
            {
                case DataType::NONE:
                    break;
                case DataType::I64:
                    this->integer_ = rhs.integer_;
                    break;
                case DataType::F32:
                    this->real_ = rhs.real_;
                    break;
                case DataType::STRING:
                    new (&this->str_) String(rhs.str_);
                    break;
                case DataType::BYTE_ARRAY:
                    new (&this->bytearray_) ByteArray(rhs.bytearray_);
                    break;
                case DataType::ARRAY:
                    new (&this->array_) Array(rhs.array_);
                    break;
                case DataType::DICT:
                    new (&this->dict_) Dict(rhs.dict_);
                    break;
                case DataType::BOOL:
                    this->bool_ = rhs.bool_;
                    break;
            }
            this->dataType = rhs.dataType;
            return *this;
        }

        Value& operator = (Value&& rhs)
        {
            this->~Value();
            switch (rhs.dataType)
            {
                case DataType::NONE:
                    break;
                case DataType::I64:
                    this->integer_ = rhs.integer_;
                    break;
                case DataType::F32:
                    this->real_ = rhs.real_;
                    break;
                case DataType::STRING:
                    new (&this->str_) String();
                    this->str_ = std::move(rhs.str_);
                    break;
                case DataType::BYTE_ARRAY:
                    new (&this->bytearray_) ByteArray();
                    this->bytearray_ = std::move(rhs.bytearray_);
                    break;
                case DataType::ARRAY:
                    new (&this->array_) Array();
                    this->array_ = std::move(rhs.array_);
                    break;
                case DataType::DICT:
                    new (&this->dict_) Dict();
                    this->dict_ = std::move(rhs.dict_);
                    break;
                case DataType::BOOL:
                    this->bool_ = rhs.bool_;
                    break;
            }
            this->dataType = rhs.dataType;
            rhs.dataType = DataType::NONE;
            return *this;
        }

        bool hasValue() const { return this->dataType != DataType::NONE; }

        OptionalRef<String> string()
        {
            return OptionalRef<String>(str_, dataType == DataType::STRING);
        }

        std::string string(std::string const& defaultVal)
        {
            return string().val();
        }

        void setString(std::string const& val)
        {
            Value v;
            v.dataType = DataType::STRING;
            new (&v.str_) std::string(val);
            *this = std::move(v);
        }

        OptionalRef<i64> integer()
        {
            return OptionalRef<i64>(integer_, dataType == DataType::I64);
        }

        i64 integer(i64 defaultVal)
        {
            if(dataType != DataType::I64) return defaultVal;
            return integer().val();
        }

        void setInteger(i64 val)
        {
            this->~Value();
            dataType = DataType::I64;
            integer_ = val;
        }

        OptionalRef<f32> real()
        {
            return OptionalRef<f32>(real_, dataType == DataType::F32);
        }

        f32 real(f32 defaultVal)
        {
            if(dataType != DataType::F32) return defaultVal;
            return real().val();
        }

        void setReal(f32 val)
        {
            this->~Value();
            dataType = DataType::F32;
            real_ = val;
        }

        OptionalVal<glm::vec2> vec2()
        {
            if (dataType == DataType::ARRAY && array_.size() >= 2
                    && array_[0].dataType == DataType::F32 && array_[1].dataType == DataType::F32)
            {
                return OptionalVal<glm::vec2>({ array_[0].real().val(), array_[1].real().val() }, true);
            }
            else
            {
                return OptionalVal<glm::vec2>({}, false);
            }
        }

        glm::vec2 vec2(glm::vec2 defaultVal)
        {
            return vec2().val();
        }

        void setVec2(glm::vec2 val)
        {
            this->~Value();
            dataType = DataType::ARRAY;
            new (&array_) Array(2);
            array_[0].setReal(val.x);
            array_[1].setReal(val.y);
        }

        OptionalVal<glm::vec3> vec3()
        {
            if (dataType == DataType::ARRAY && array_.size() >= 3
                    && array_[0].dataType == DataType::F32
                    && array_[1].dataType == DataType::F32
                    && array_[2].dataType == DataType::F32)
            {
                return OptionalVal<glm::vec3>({
                        array_[0].real().val(), array_[1].real().val(), array_[2].real().val() }, true);
            }
            else
            {
                return OptionalVal<glm::vec3>({}, false);
            }
        }

        glm::vec3 vec3(glm::vec3 const& defaultVal)
        {
            return vec3().val();
        }

        void setVec3(glm::vec3 const& val)
        {
            this->~Value();
            dataType = DataType::ARRAY;
            new (&array_) Array(3);
            array_[0].setReal(val.x);
            array_[1].setReal(val.y);
            array_[2].setReal(val.z);
        }

        OptionalVal<glm::vec4> vec4()
        {
            if (dataType == DataType::ARRAY && array_.size() >= 3
                    && array_[0].dataType == DataType::F32
                    && array_[1].dataType == DataType::F32
                    && array_[2].dataType == DataType::F32)
            {
                return OptionalVal<glm::vec4>({
                        array_[0].real().val(), array_[1].real().val(),
                        array_[2].real().val(), array_[3].real().val() }, true);
            }
            else
            {
                return OptionalVal<glm::vec4>({}, false);
            }
        }

        glm::vec4 vec4(glm::vec4 const& defaultVal)
        {
            return vec4().val();
        }

        void setVec4(glm::vec4 const& val)
        {
            this->~Value();
            dataType = DataType::ARRAY;
            new (&array_) Array(4);
            array_[0].setReal(val.x);
            array_[1].setReal(val.y);
            array_[2].setReal(val.z);
            array_[3].setReal(val.w);
        }

        OptionalRef<bool> boolean()
        {
            return OptionalRef<bool>(bool_, dataType == DataType::BOOL);
        }

        bool boolean(bool defaultVal)
        {
            return dataType == DataType::BOOL ? defaultVal : boolean().val();
        }

        void setBoolean(bool val)
        {
            this->~Value();
            dataType = DataType::BOOL;
            bool_ = val;
        }

        OptionalRef<ByteArray> bytearray(bool emptyArrayIfNone=false)
        {
            if (emptyArrayIfNone && dataType == DataType::NONE)
            {
                setBytearray(ByteArray());
            }
            return OptionalRef<ByteArray>(bytearray_, dataType == DataType::BYTE_ARRAY);
        }

        void setBytearray(ByteArray && val)
        {
            this->~Value();
            dataType = DataType::BYTE_ARRAY;
            new (&bytearray_) ByteArray(std::move(val));
        }

        void setBytearray(ByteArray const& val)
        {
            this->~Value();
            dataType = DataType::BYTE_ARRAY;
            new (&bytearray_) ByteArray(val);
        }

        OptionalRef<Array> array(bool emptyArrayIfNone=false)
        {
            if (emptyArrayIfNone && dataType == DataType::NONE)
            {
                setArray(Array());
            }
            return OptionalRef<Array>(array_, dataType == DataType::ARRAY);
        }

        void setArray(Array const& val)
        {
            this->~Value();
            dataType = DataType::ARRAY;
            new (&array_) Array(val);
        }

        void setArray(Array && val)
        {
            this->~Value();
            dataType = DataType::ARRAY;
            new (&array_) Array(std::move(val));
        }

        OptionalRef<Dict> dict(bool emptyDictIfNone=false)
        {
            if (emptyDictIfNone && dataType == DataType::NONE)
            {
                setDict(Dict());
            }
            return OptionalRef<Dict>(dict_, dataType == DataType::DICT);
        }

        void setDict(Dict && val)
        {
            this->~Value();
            dataType = DataType::DICT;
            new (&dict_) Dict(std::move(val));
        }

        void setDict(Dict const& val)
        {
            this->~Value();
            dataType = DataType::DICT;
            new (&dict_) Dict(val);
        }

        /*
        Value& operator [] (std::string key)
        {
            CHECK_TYPE(DataType::DICT);
            return dict_[key];
        }

        Value& operator [] (std::size_t index)
        {
            CHECK_TYPE(DataType::ARRAY);
            return array_[index];
        }
        */

        template <typename T>
        OptionalVal<T> convertBytes()
        {
            if (dataType != DataType::BYTE_ARRAY)
            {
                return OptionalVal<T>({}, false);
            }
            T val;
            memcpy(&val, bytearray_.data(), bytearray_.size());
            return OptionalVal<T>(std::move(val), true);
        }

        void debugOutput(std::ostream& os, u32 indent, bool newline) const;

        Value& operator=(std::string const& val) { setString(val); return *this; }
        Value& operator=(const char* val) { setString(val); return *this; }
        Value& operator=(i64 val) { setInteger(val); return *this; }
        Value& operator=(f32 val) { setReal(val); return *this; }
        Value& operator=(bool val) { setBoolean(val); return *this; }
    };

    Value makeString(std::string const& val)
    {
        Value v;
        v.setString(val);
        return v;
    }

    Value makeString(std::string&& val)
    {
        Value v;
        v.setString(std::move(val));
        return v;
    }

    Value makeInteger(i64 val)
    {
        Value v;
        v.setInteger(val);
        return v;
    }

    Value makeReal(f32 val)
    {
        Value v;
        v.setReal(val);
        return v;
    }

    Value makeVec2(glm::vec2 val)
    {
        Value v;
        v.setVec2(val);
        return v;
    }

    Value makeVec3(glm::vec3 const& val)
    {
        Value v;
        v.setVec3(val);
        return v;
    }

    Value makeVec4(glm::vec4 const& val)
    {
        Value v;
        v.setVec4(val);
        return v;
    }

    Value makeArray(Value::Array const& val = {})
    {
        Value v;
        v.setArray(val);
        return v;
    }

    Value makeArray(Value::Array&& val)
    {
        Value v;
        v.setArray(std::move(val));
        return v;
    }

    Value makeBytearray(Value::ByteArray const& val = {})
    {
        Value v;
        v.setBytearray(val);
        return v;
    }

    Value makeBytearray(Value::ByteArray&& val)
    {
        Value v;
        v.setBytearray(std::move(val));
        return v;
    }

    Value makeDict(Value::Dict const& val = {})
    {
        Value v;
        v.setDict(val);
        return v;
    }

    Value makeDict(Value::Dict&& val)
    {
        Value v;
        v.setDict(val);
        return v;
    }

    Value makeBool(bool val)
    {
        Value v;
        v.setBoolean(val);
        return v;
    }

    Value load(std::string const& filename);
    void save(Value const& val, std::string const& filename);

    inline std::ostream& operator << (std::ostream& os, Value const& rhs)
    {
        rhs.debugOutput(os, 0, true);
        return os;
    }

    inline std::ostream& operator << (std::ostream& os, DataType dataType)
    {
        switch (dataType)
        {
            case NONE:
                return os << "NONE";
            case STRING:
                return os << "STRING";
            case I64:
                return os << "I64";
            case F32:
                return os << "F32";
            case BYTE_ARRAY:
                return os << "BYTE_ARRAY";
            case ARRAY:
                return os << "ARRAY";
            case DICT:
                return os << "DICT";
            case BOOL:
                return os << "BOOL";
        }
        return os;
    }
};

#ifndef NDEBUG
#define DESERIALIZE_ERROR(...) { error(context, ": ", __VA_ARGS__, '\n'); return; }
#else
#define DESERIALIZE_ERROR(...) { error(__VA_ARGS__, '\n'); return; }
#endif

class Serializer
{
public:
    DataFile::Value::Dict& dict;
    bool deserialize;
    const char* context = "";

    Serializer(DataFile::Value& val, bool deserialize) : dict(val.dict(true).val()),
        deserialize(deserialize) {}

    template<typename T>
    void write(const char* name, T field)
    {
        if (!deserialize)
        {
            element(name, dict[name], field);
        }
    }

    template<typename T>
    void _value(const char* name, T& field, const char* context="")
    {
        if (deserialize)
        {
            element(name, dict[name], field);
            this->context = context;
        }
        else
        {
            element(name, dict[name], field);
        }
    }

    template<typename T>
    void element(const char* name, DataFile::Value& val, T& dest)
    {
        if constexpr (std::is_enum<T>::value)
        {
            if (deserialize)
            {
                auto v = val.integer();
                if (!v.hasValue())
                {
                    DESERIALIZE_ERROR("Failed to read enum value as INTEGER: \"", name, "\"");
                }
                dest = (T)v.val();
            }
            else val = (i64)dest;
        }
        else if constexpr (std::is_integral<T>::value)
        {
            if (deserialize)
            {
                auto v = val.integer();
                if (!v.hasValue())
                {
                    DESERIALIZE_ERROR("Failed to read value as INTEGER: \"", name, "\"");
                }
                dest = (T)v.val();
                if (v.val() > std::numeric_limits<T>::max())
                {
                    error(context, ": deserialized integer overflow");
                }
            }
            else val = (i64)dest;
        }
        else
        {
            if (deserialize)
            {
                auto v = val.dict();
                if (!v.hasValue())
                {
                    DESERIALIZE_ERROR("Failed to read value as DICT: \"", name, "\"");
                }
                Serializer childSerializer(val, true);
                dest.serialize(childSerializer);
            }
            else
            {
                auto childDict = DataFile::makeDict();
                Serializer childSerializer(childDict, false);
                dest.serialize(childSerializer);
                val = childDict;
            }
        }
    }

    template<> void element(const char* name, DataFile::Value& val, std::string& dest)
    {
        if (deserialize)
        {
            auto v = val.string();
            if (!v.hasValue()) DESERIALIZE_ERROR("Failed to read value as STRING: \"", name, "\"");
            dest = v.val();
        }
        else val = dest;
    }

    template<> void element(const char* name, DataFile::Value& val, bool& dest)
    {
        if (deserialize)
        {
            auto v = val.boolean();
            if (!v.hasValue()) DESERIALIZE_ERROR("Failed to read value as BOOL: \"", name, "\"");
            dest = v.val();
        }
        else val = dest;
    }

    template<> void element(const char* name, DataFile::Value& val, f32& dest)
    {
        if (deserialize)
        {
            auto v = val.real();
            if (!v.hasValue()) DESERIALIZE_ERROR("Failed to read value as REAL: \"", name, "\"");
            dest = v.val();
        }
        else val = dest;
    }

    template<> void element(const char* name, DataFile::Value& val, glm::vec2& dest)
    {
        if (deserialize)
        {
            auto v = val.array();
            if (!v.hasValue() || v.val().size() < 2
                    || !v.val()[0].real().hasValue() || !v.val()[1].real().hasValue())
            {
                DESERIALIZE_ERROR("Failed to read ARRAY (vec2) field: \"", name, "\"");
            }
            dest = { v.val()[0].real().val(), v.val()[1].real().val() };
        }
        else
        {
            val = DataFile::makeArray(DataFile::Value::Array(
                        { DataFile::makeReal(dest.x), DataFile::makeReal(dest.y) }));
        }
    }

    template<> void element(const char* name, DataFile::Value& val, glm::vec3& dest)
    {
        if (deserialize)
        {
            auto v = val.array();
            if (!v.hasValue() || v.val().size() < 3
                    || !v.val()[0].real().hasValue() || !v.val()[1].real().hasValue()
                    || !v.val()[2].real().hasValue())
            {
                DESERIALIZE_ERROR("Failed to read ARRAY (vec3) field: \"", name, "\"");
            }
            dest = { v.val()[0].real().val(), v.val()[1].real().val(), v.val()[2].real().val() };
        }
        else
        {
            val = DataFile::makeArray(DataFile::Value::Array({
                        DataFile::makeReal(dest.x),
                        DataFile::makeReal(dest.y),
                        DataFile::makeReal(dest.z),
                    }));
        }
    }

    template<> void element(const char* name, DataFile::Value& val, glm::vec4& dest)
    {
        if (deserialize)
        {
            auto v = val.array();
            if (!v.hasValue() || v.val().size() < 4
                    || !v.val()[0].real().hasValue() || !v.val()[1].real().hasValue()
                    || !v.val()[2].real().hasValue() || !v.val()[3].real().hasValue())
            {
                DESERIALIZE_ERROR("Failed to read ARRAY (vec4) field: \"", name, "\"");
            }
            dest = {
                v.val()[0].real().val(),
                v.val()[1].real().val(),
                v.val()[2].real().val(),
                v.val()[3].real().val()
            };
        }
        else
        {
            val = DataFile::makeArray(DataFile::Value::Array({
                        DataFile::makeReal(dest.x),
                        DataFile::makeReal(dest.y),
                        DataFile::makeReal(dest.z),
                        DataFile::makeReal(dest.w)
                    }));
        }
    }

    template<> void element(const char* name, DataFile::Value& val, glm::quat& dest)
    {
        if (deserialize)
        {
            auto v = val.array();
            if (!v.hasValue() || v.val().size() < 4
                    || !v.val()[0].real().hasValue() || !v.val()[1].real().hasValue()
                    || !v.val()[2].real().hasValue() || !v.val()[3].real().hasValue())
            {
                DESERIALIZE_ERROR("Failed to read ARRAY (quat) field: \"", name, "\"");
            }
            dest = {
                v.val()[3].real().val(),
                v.val()[0].real().val(),
                v.val()[1].real().val(),
                v.val()[2].real().val()
            };
        }
        else
        {
            val = DataFile::makeArray(DataFile::Value::Array({
                        DataFile::makeReal(dest.x),
                        DataFile::makeReal(dest.y),
                        DataFile::makeReal(dest.z),
                        DataFile::makeReal(dest.w)
                    }));
        }
    }

    template<typename T> void element(const char* name, DataFile::Value& val, std::vector<T>& dest)
    {
        if constexpr (std::is_arithmetic<T>::value)
        {
            if (deserialize)
            {
                auto v = val.bytearray();
                if (!v.hasValue())
                {
                    DESERIALIZE_ERROR("Failed to read BYTEARRAY field: \"", name, "\"");
                }
                if (v.val().size() % sizeof(T) != 0)
                {
                    DESERIALIZE_ERROR("Cannot convert BYTEARRAY field: \"", name, "\"");
                }
                dest.assign(reinterpret_cast<T*>(v.val().data()),
                        reinterpret_cast<T*>(v.val().data() + v.val().size()));
            }
            else
            {
                DataFile::Value::ByteArray bytes;
                bytes.assign(reinterpret_cast<u8*>(dest.data()),
                             reinterpret_cast<u8*>(dest.data() + dest.size()));
                val.setBytearray(std::move(bytes));
            }
        }
        else
        {
            if (deserialize)
            {
                auto v = val.array();
                if (!v.hasValue())
                {
                    DESERIALIZE_ERROR("Failed to read ARRAY field: \"", name, "\"");
                }
                dest.clear();
                dest.reserve(v.val().size());
                for (auto& item : v.val())
                {
                    T el;
                    element(name, item, el);
                    dest.push_back(std::move(el));
                }
            }
            else
            {
                DataFile::Value::Array array;
                array.reserve(dest.size());
                for (auto& item : dest)
                {
                    DataFile::Value el;
                    element(name, el, item);
                    array.push_back(std::move(el));
                }
                val.setArray(std::move(array));
            }
        }
    }

    template<> void element(const char* name, DataFile::Value& val, std::vector<u8>& dest)
    {
        if (deserialize)
        {
            auto v = val.bytearray();
            if (!v.hasValue())
            {
                DESERIALIZE_ERROR("Failed to read BYTEARRAY field: \"", name, "\"");
            }
            dest.clear();
            dest.assign(v.val().begin(), v.val().end());
        }
        else
        {
            val.setBytearray(DataFile::Value::ByteArray(dest.begin(), dest.end()));
        }
    }

    template<typename T> void element(const char* name, DataFile::Value& val, std::unique_ptr<T>& dest)
    {
        if (deserialize)
        {
            dest.reset(new T);
            element(name, val, *dest);
        }
        else
        {
            element(name, val, *dest);
        }
    }

    template<typename T>
    static void toFile(T& val, std::string const& filename)
    {
        auto data = DataFile::makeDict();
        Serializer s(data, false);
        val.serialize(s);
        DataFile::save(data, filename);
    }

    template<typename T>
    static void fromFile(T& val, std::string const& filename)
    {
        auto data = DataFile::load(filename);
        if (data.hasValue())
        {
            Serializer s(data, true);
            val.serialize(s);
        }
    }
};

#undef DESERIALIZE_ERROR

#ifndef NDEBUG
#define field(FIELD) _value(#FIELD, FIELD, str(__FILE__, ": ", __LINE__).c_str())
#define value(NAME, FIELD) _value(NAME, FIELD, str(__FILE__, ": ", __LINE__).c_str())
#else
#define field(FIELD) _value(#FIELD, FIELD, "WARNING")
#define value(NAME, FIELD) _value(NAME, FIELD, "WARNING")
#endif
