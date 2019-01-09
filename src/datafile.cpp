#include "datafile.h"

DataFile::Value DataFile::load(const char* filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        error("Failed to load data file: ", filename, '\n');
        return DataFile::Value();
    }

    file.seekg(0);

    u32 header;
    file.read((char*)&header, sizeof(u32));
    if (header != DataFile::MAGIC_NUMBER)
    {
        error("Invalid data file: ", filename, '\n');
        return DataFile::Value();
    }

    return DataFile::Value(file);
}

DataFile::Value::Value(std::ifstream& stream)
{
    *this = readValue(stream);
}

DataFile::Value DataFile::Value::readValue(std::ifstream& stream)
{
    Value value;
    stream.read((char*)&value.dataType, sizeof(DataFile::DataType));
    switch (value.dataType)
    {
        case DataType::I64:
        {
            stream.read((char*)&value.integer_, sizeof(i64));
        } break;
        case DataType::F64:
        {
            stream.read((char*)&value.real_, sizeof(f64));
        } break;
        case DataType::STRING:
        {
            u32 len;
            stream.read((char*)&len, sizeof(len));
            new (&value.str_) std::string(len, ' ');
            stream.read(&value.str_[0], len);
        } break;
        case DataType::BYTE_ARRAY:
        {
            u32 len;
            stream.read((char*)&len, sizeof(len));
            new (&value.bytearray_) ByteArray(len);
            stream.read((char*)value.bytearray_.data(), len);
        } break;
        case DataType::ARRAY:
        {
            u32 len;
            stream.read((char*)&len, sizeof(len));
            new (&value.array_) Array(len);
            for (u32 i=0; i<len; ++i)
            {
                value.array_[i] = readValue(stream);
            }
        } break;
        case DataType::DICT:
        {
            u32 len;
            stream.read((char*)&len, sizeof(len));
            new (&value.dict_) Dict();
            for (u32 i=0; i<len; ++i)
            {
                u32 keyLen;
                stream.read((char*)&keyLen, sizeof(keyLen));
                std::string key(keyLen, ' ');
                stream.read(&key[0], keyLen);
                value.dict_[key] = readValue(stream);
            }
        } break;
        case DataType::BOOL:
        {
            stream.read((char*)&value.bool_, sizeof(u32));
        } break;
        default:
        {
            error("Invalid data type: ", (u32)value.dataType, '\n');
            value.dataType = DataType::NONE;
        } break;
    }

    return value;
}

void DataFile::Value::debugOutput(std::ostream& os, u32 indent, bool newline) const
{
    switch (dataType)
    {
        case DataType::NONE:
            os << "None";
            break;
        case DataType::I64:
            os << integer_;
            break;
        case DataType::F64:
            os << real_;
            break;
        case DataType::STRING:
            os << '"' << str_ << '"';
            break;
        case DataType::BYTE_ARRAY:
            os << "<bytearray>";
            break;
        case DataType::ARRAY:
            if (newline)
            {
                os << std::string(indent * 2, ' ');
            }
            if (array_.size() == 0)
            {
                os << "[]";
            }
            else
            {
                os << "[\n";
                for (u32 i=0; i<array_.size(); ++i)
                {
                    array_[i].debugOutput(os, indent + 1, true);
                    os << ",\n";
                }
                os << std::string(indent * 2, ' ') << "]";
            }
            break;
        case DataType::DICT:
            if (newline)
            {
                os << std::string(indent * 2, ' ');
            }
            if (dict_.size() == 0)
            {
                os << "{}";
            }
            else
            {
                os << "{\n";
                for (auto const& pair : dict_)
                {
                    os << std::string((indent + 1) * 2, ' ');
                    os << pair.first << ": ";
                    pair.second.debugOutput(os, indent + 1, false);
                    os << ",\n";
                }
                os << std::string(indent * 2, ' ') << "}";
            }
            break;
        case DataType::BOOL:
            os << (bool_ ? "true" : "false");
            break;
    }
}

