#pragma once

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <cassert>
#include <fstream>
#include <iostream>
#include "misc.h"

namespace DataFile
{
    const u32 MAGIC_NUMBER = 0x00001111;

    enum DataType : u32
    {
        NONE,
        STRING,
        I64,
        F64,
        BYTE_ARRAY,
        ARRAY,
        DICT,
        BOOL,
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
            f64 real_;
            ByteArray bytearray_;
            Array array_;
            Dict dict_;
            bool bool_;
        };

#ifndef NDEBUG
        std::string keyName;
#define CHECK_TYPE(...) {\
    DataType types[] = { __VA_ARGS__ };\
    bool found = false;\
    for (auto& t : types) {\
        if (dataType == t) found = true;\
    }\
    if (!found) {\
        std::string typesStr = "";\
        for (u32 i=0; i<ARRAY_SIZE(types); ++i) typesStr += str(types[i]) + ((i < ARRAY_SIZE(types) - 1) ? ", " : "");\
        FATAL_ERROR("Failed to read value with key '", keyName, "'. Expected data type to be one of [", typesStr, "], but it is ", dataType, '.');\
    }\
}
#else
#define CHECK_TYPE(...)
#endif

    public:
        static Value readValue(std::ifstream& stream);
        static Value readValue(std::string::const_iterator& ch, std::string::const_iterator end);

        ~Value()
        {
            switch (dataType)
            {
                case DataType::NONE:
                case DataType::I64:
                case DataType::F64:
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
        Value(Value&& other) { *this = std::move(other); }

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
                case DataType::F64:
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
        bool hasKey(std::string const& key) const { return this->dataType == DataType::DICT && dict_.count(key) > 0; }

        std::string& string()
        {
            CHECK_TYPE(DataType::STRING);
            return str_;
        }

        i64 integer()
        {
            CHECK_TYPE(DataType::I64, DataType::F64);
            if (dataType == F64) return (i64)real_;
            return integer_;
        }

        f64 real()
        {
            CHECK_TYPE(DataType::I64, DataType::F64);
            if (dataType == I64) return (f64)integer_;
            return real_;
        }

        bool boolean()
        {
            CHECK_TYPE(DataType::BOOL);
            return bool_;
        }

        ByteArray& bytearray()
        {
            CHECK_TYPE(DataType::BYTE_ARRAY);
            return bytearray_;
        }

        Array& array()
        {
            CHECK_TYPE(DataType::ARRAY);
            return array_;
        }

        Dict& dict()
        {
            CHECK_TYPE(DataType::DICT);
            return dict_;
        }

        Value& operator [] (std::string key)
        {
            CHECK_TYPE(DataType::DICT);
#ifndef NDEBUG
            if (dict_.count(key) == 0) { FATAL_ERROR("Key '", key, "' does not exist."); }
#endif
            return dict_[key];
        }

        Value& operator [] (std::size_t index)
        {
            CHECK_TYPE(DataType::ARRAY);
            return array_[index];
        }

        template <typename T>
        T convertBytes()
        {
            CHECK_TYPE(DataType::BYTE_ARRAY);
            assert(dataType == DataType::BYTE_ARRAY);
            T val;
            memcpy(&val, bytearray_.data(), bytearray_.size());
            return val;
        }

        void debugOutput(std::ostream& os, u32 indent, bool newline) const;
    };

    Value load(const char* filename);

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
            case F64:
                return os << "F64";
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
