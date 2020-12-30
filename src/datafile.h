#pragma once

#include "util.h"

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
        using Dict = Map<Str64, Value>;
        using Array = Array<Value>;
        using ByteArray = ::Array<u8>;
        using String = Str64;

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
        static Value readValue(Buffer& buf);
        static Value readValue(const char*& ch, const char* end);
        void write(Buffer& buf) const;

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
        explicit Value(String&& str)
        {
            dataType = DataType::STRING;
            new (&str_) String(std::move(str));
        }
        explicit Value(String const& str)
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

        String string(String const& defaultVal)
        {
            return string().val();
        }

        void setString(String const& val)
        {
            Value v;
            v.dataType = DataType::STRING;
            new (&v.str_) String(val);
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

        OptionalVal<Vec2> vec2()
        {
            if (dataType == DataType::ARRAY && array_.size() >= 2
                    && array_[0].dataType == DataType::F32 && array_[1].dataType == DataType::F32)
            {
                return OptionalVal<Vec2>({ array_[0].real().val(), array_[1].real().val() }, true);
            }
            else
            {
                return OptionalVal<Vec2>({}, false);
            }
        }

        Vec2 vec2(Vec2 defaultVal)
        {
            return vec2().val();
        }

        void setVec2(Vec2 val)
        {
            this->~Value();
            dataType = DataType::ARRAY;
            new (&array_) Array(2);
            array_[0].setReal(val.x);
            array_[1].setReal(val.y);
        }

        OptionalVal<Vec3> vec3()
        {
            if (dataType == DataType::ARRAY && array_.size() >= 3
                    && array_[0].dataType == DataType::F32
                    && array_[1].dataType == DataType::F32
                    && array_[2].dataType == DataType::F32)
            {
                return OptionalVal<Vec3>({
                        array_[0].real().val(), array_[1].real().val(), array_[2].real().val() }, true);
            }
            else
            {
                return OptionalVal<Vec3>({}, false);
            }
        }

        Vec3 vec3(Vec3 const& defaultVal)
        {
            return vec3().val();
        }

        void setVec3(Vec3 const& val)
        {
            this->~Value();
            dataType = DataType::ARRAY;
            new (&array_) Array(3);
            array_[0].setReal(val.x);
            array_[1].setReal(val.y);
            array_[2].setReal(val.z);
        }

        OptionalVal<Vec4> vec4()
        {
            if (dataType == DataType::ARRAY && array_.size() >= 3
                    && array_[0].dataType == DataType::F32
                    && array_[1].dataType == DataType::F32
                    && array_[2].dataType == DataType::F32)
            {
                return OptionalVal<Vec4>({
                        array_[0].real().val(), array_[1].real().val(),
                        array_[2].real().val(), array_[3].real().val() }, true);
            }
            else
            {
                return OptionalVal<Vec4>({}, false);
            }
        }

        Vec4 vec4(Vec4 const& defaultVal)
        {
            return vec4().val();
        }

        void setVec4(Vec4 const& val)
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

        void debugOutput(StrBuf& buf, u32 indent, bool newline) const;

        Value& operator=(String const& val) { setString(val); return *this; }
        Value& operator=(const char* val) { setString(val); return *this; }
        Value& operator=(i64 val) { setInteger(val); return *this; }
        Value& operator=(f32 val) { setReal(val); return *this; }
        Value& operator=(bool val) { setBoolean(val); return *this; }
    };

    Value makeString(Value::String const& val)
    {
        Value v;
        v.setString(val);
        return v;
    }

    Value makeString(Value::String val)
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

    Value makeVec2(Vec2 val)
    {
        Value v;
        v.setVec2(val);
        return v;
    }

    Value makeVec3(Vec3 const& val)
    {
        Value v;
        v.setVec3(val);
        return v;
    }

    Value makeVec4(Vec4 const& val)
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

    Value load(const char* filename);
    void save(Value const& val, const char* filename);
};

#ifndef NDEBUG
//#define DESERIALIZE_ERROR(...) { error("%s: %s", context, Str512::format(__VA_ARGS__).data()); return; }
#define DESERIALIZE_ERROR(...) { return; }
#else
//#define DESERIALIZE_ERROR(...) { error(__VA_ARGS__); return; }
#define DESERIALIZE_ERROR(...) { return; }
#endif

class Serializer
{
public:
    DataFile::Value::Dict& dict;
    bool deserialize;
    const char* context;

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
    void serializeValue(const char* name, T& field, const char* context)
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
                    DESERIALIZE_ERROR("Failed to read enum value as INTEGER: \"%s\"", name);
                }
                dest = (T)v.val();
            }
            else
            {
                val = (i64)dest;
            }
        }
        else if constexpr (std::is_integral<T>::value)
        {
            if (deserialize)
            {
                auto v = val.integer();
                if (!v.hasValue())
                {
                    DESERIALIZE_ERROR("Failed to read enum value as INTEGER: \"%s\"", name);
                }
                dest = (T)v.val();
                if (v.val() > std::numeric_limits<T>::max())
                {
                    error("%s: deserialized integer overflow", context);
                }
            }
            else
            {
                val = (i64)dest;
            }
        }
        else if constexpr (std::is_array<T>::value)
        {
            auto arraySize = std::extent<T>::value;
            if (deserialize)
            {
                auto v = val.array();
                if (!v.hasValue())
                {
                    DESERIALIZE_ERROR("Failed to read ARRAY field: \"%s\"", name);
                }
                if (v.val().size() < arraySize)
                {
                    DESERIALIZE_ERROR("Failed to read ARRAY field: \"%s\"", name);
                }
                for (u32 i=0; i<(u32)arraySize; ++i)
                {
                    element(name, v.val()[i], dest[i]);
                }
            }
            else
            {
                val = DataFile::makeArray();
                val.array().val().reserve(arraySize);
                for (u32 i=0; i<(u32)arraySize; ++i)
                {
                    DataFile::Value el;
                    element(name, el, dest[i]);
                    val.array().val().push_back(std::move(el));
                }
            }
        }
        else
        {
            if (deserialize)
            {
                auto v = val.dict();
                if (!v.hasValue())
                {
                    DESERIALIZE_ERROR("Failed to read value as DICT: \"%s\"", name);
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

    template<> void element(const char* name, DataFile::Value& val, DataFile::Value::String& dest)
    {
        if (deserialize)
        {
            auto v = val.string();
            if (!v.hasValue()) DESERIALIZE_ERROR("Failed to read value as STRING: \"%s\"", name);
            dest = v.val();
        }
        else val = dest;
    }

    template<> void element(const char* name, DataFile::Value& val, bool& dest)
    {
        if (deserialize)
        {
            auto v = val.boolean();
            if (!v.hasValue()) DESERIALIZE_ERROR("Failed to read value as BOOL: \"%s\"", name);
            dest = v.val();
        }
        else val = dest;
    }

    template<> void element(const char* name, DataFile::Value& val, f32& dest)
    {
        if (deserialize)
        {
            auto v = val.real();
            if (!v.hasValue()) DESERIALIZE_ERROR("Failed to read value as REAL: \"%s\"", name);
            dest = v.val();
        }
        else val = dest;
    }

    template<typename T>
    void realArray(const char* name, DataFile::Value& val, T& dest)
    {
        u32 count = sizeof(dest) / sizeof(f32);
        if (deserialize)
        {
            auto v = val.array();
            if (!v.hasValue() || v.val().size() < count)
            {
                DESERIALIZE_ERROR("Failed to read real ARRAY [%u] field: \"%s\"", count, name);
            }
            for (u32 i=0; i<count; ++i)
            {
                auto optionalValue = v.val()[i].real();
                if (!optionalValue.hasValue())
                {
                    DESERIALIZE_ERROR("Failed to read real ARRAY [%u] field: \"%s\"", count, name);
                }
                ((f32*)&dest)[i] = optionalValue.val();
            }
        }
        else
        {
            val = DataFile::makeArray();
            val.array().val().reserve(count);
            for (u32 i=0; i<count; ++i)
            {
                val.array().val().push_back(DataFile::makeReal(((f32*)&dest)[i]));
            }
        }
    }

    template<> void element(const char* name, DataFile::Value& val, Vec2& dest)
    {
        realArray(name, val, dest);
    }

    template<> void element(const char* name, DataFile::Value& val, Vec3& dest)
    {
        realArray(name, val, dest);
    }

    template<> void element(const char* name, DataFile::Value& val, Vec4& dest)
    {
        realArray(name, val, dest);
    }

    template<> void element(const char* name, DataFile::Value& val, Quat& dest)
    {
        realArray(name, val, dest);
    }

    template<typename T>
    void array(const char* name, DataFile::Value& val, T& dest)
    {
        using V = typename T::value_type;
        if constexpr (std::is_arithmetic<V>::value)
        {
            if (deserialize)
            {
                auto v = val.bytearray();
                if (!v.hasValue())
                {
                    DESERIALIZE_ERROR("Failed to read BYTEARRAY field: \"%s\"", name);
                }
                if (v.val().size() % sizeof(V) != 0)
                {
                    DESERIALIZE_ERROR("Cannot convert BYTEARRAY field: \"%s\"", name);
                }
                dest.assign(reinterpret_cast<V*>(v.val().data()),
                        reinterpret_cast<V*>(v.val().data() + v.val().size()));
            }
            else
            {
                DataFile::Value::ByteArray bytes(reinterpret_cast<u8*>(dest.data()),
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
                    DESERIALIZE_ERROR("Failed to read ARRAY field: \"%s\"", name);
                }
                dest.clear();
                dest.reserve(v.val().size());
                for (auto& item : v.val())
                {
                    V el;
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

    template<typename T> void element(const char* name, DataFile::Value& val, Array<T>& dest)
    {
        array(name, val, dest);
    }

    template<typename T> void element(const char* name, DataFile::Value& val, SmallArray<T>& dest)
    {
        array(name, val, dest);
    }

    template<typename T> void element(const char* name, DataFile::Value& val, OwnedPtr<T>& dest)
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
    static void toFile(T& val, const char* filename)
    {
        auto data = DataFile::makeDict();
        Serializer s(data, false);
        val.serialize(s);
        DataFile::save(data, filename);
    }

    template<typename T>
    static void fromFile(T& val, const char* filename)
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
#define field(FIELD) serializeValue(#FIELD, FIELD, Str512::format("%s: %u", __FILE__, __LINE__).data())
#define fieldName(NAME, FIELD) serializeValue(NAME, FIELD, Str512::format("%s: %u", __FILE__, __LINE__).data())
#else
#define field(FIELD) serializeValue(#FIELD, FIELD, "WARNING")
#define fieldName(NAME, FIELD) serializeValue(NAME, FIELD, "WARNING")
#endif
