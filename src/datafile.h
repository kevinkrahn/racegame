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

        static Value readValue(std::ifstream& stream);

    public:
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
        Value(std::ifstream& file);

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

        std::string& string()
        {
            assert(dataType == DataType::STRING);
            return str_;
        }

        i64& integer()
        {
            assert(dataType == DataType::I64);
            return integer_;
        }

        f64& real()
        {
            assert(dataType == DataType::F64);
            return real_;
        }

        bool& boolean()
        {
            assert(dataType == DataType::BOOL);
            return bool_;
        }

        ByteArray& bytearray()
        {
            assert(dataType == DataType::BYTE_ARRAY);
            return bytearray_;
        }

        Array& array()
        {
            assert(dataType == DataType::ARRAY);
            return array_;
        }

        Dict& dict()
        {
            assert(dataType == DataType::DICT);
            return dict_;
        }

        Value& operator [] (std::string key)
        {
            assert(dataType == DataType::DICT);
            return dict_[key];
        }

        Value& operator [] (std::size_t index)
        {
            assert(dataType == DataType::ARRAY);
            return array_[index];
        }

        template <typename T>
        T convertBytes()
        {
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
};
