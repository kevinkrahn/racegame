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
            String str;
            i64 integer;
            f64 real;
            ByteArray bytearray;
            Array array;
            Dict dict;
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
                    break;
                case DataType::STRING:
                    str.~String();
                    break;
                case DataType::BYTE_ARRAY:
                    bytearray.~ByteArray();
                    break;
                case DataType::ARRAY:
                    array.~Array();
                    break;
                case DataType::DICT:
                    dict.~Dict();
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
                    this->integer = rhs.integer;
                    break;
                case DataType::F64:
                    this->real = rhs.real;
                    break;
                case DataType::STRING:
                    new (&this->str) String();
                    this->str = std::move(rhs.str);
                    break;
                case DataType::BYTE_ARRAY:
                    new (&this->bytearray) ByteArray();
                    this->bytearray = std::move(rhs.bytearray);
                    break;
                case DataType::ARRAY:
                    new (&this->array) Array();
                    this->array = std::move(rhs.array);
                    break;
                case DataType::DICT:
                    new (&this->dict) Dict();
                    this->dict = std::move(rhs.dict);
                    break;
            }
            this->dataType = rhs.dataType;
            rhs.dataType = DataType::NONE;
            return *this;
        }

        std::string const& getString() const
        {
            assert(dataType == DataType::STRING);
            return str;
        }

        i64 getInt() const
        {
            assert(dataType == DataType::I64);
            return integer;
        }

        f64 getFloat() const
        {
            assert(dataType == DataType::F64);
            return real;
        }

        ByteArray const& getByteArray() const
        {
            assert(dataType == DataType::BYTE_ARRAY);
            return bytearray;
        }

        Array const& getArray() const
        {
            assert(dataType == DataType::ARRAY);
            return array;
        }

        Dict const& getDict() const
        {
            assert(dataType == DataType::DICT);
            return dict;
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
