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
            stream.read((char*)&value.integer, sizeof(i64));
        } break;
        case DataType::F64:
        {
            stream.read((char*)&value.real, sizeof(f64));
        } break;
        case DataType::STRING:
        {
            u32 len;
            stream.read((char*)&len, sizeof(len));
            new (&value.str) std::string(len, ' ');
            stream.read(value.str.data(), len);
        } break;
        case DataType::BYTE_ARRAY:
        {
            u32 len;
            stream.read((char*)&len, sizeof(len));
            new (&value.bytearray) ByteArray(len);
            stream.read((char*)value.bytearray.data(), len);
        } break;
        case DataType::ARRAY:
        {
            u32 len;
            stream.read((char*)&len, sizeof(len));
            new (&value.array) Array(len);
            for (u32 i=0; i<len; ++i)
            {
                value.array[i] = readValue(stream);
            }
        } break;
        case DataType::DICT:
        {
            u32 len;
            stream.read((char*)&len, sizeof(len));
            new (&value.dict) Dict();
            for (u32 i=0; i<len; ++i)
            {
                u32 keyLen;
                stream.read((char*)&keyLen, sizeof(keyLen));
                std::string key(keyLen, ' ');
                stream.read(key.data(), keyLen);
                value.dict[key] = readValue(stream);
            }
        } break;
        default:
        {
            error("Invalid data type: ", (u32)value.dataType, '\n');
            value.dataType = DataType::NONE;
        } break;
    }

    return value;
}

void DataFile::Value::debugOutput(std::ostream& os) const
{
    switch (dataType)
    {
        case DataType::NONE:
            os << "None";
            break;
        case DataType::I64:
            os << integer;
            break;
        case DataType::F64:
            os << real;
            break;
        case DataType::STRING:
            os << str;
            break;
        case DataType::BYTE_ARRAY:
            os << "<bytearray>";
            break;
        case DataType::ARRAY:
            os << "[ ";
            for (u32 i=0; i<array.size(); ++i)
            {
                if (i > 0)
                {
                    os << ", ";
                }
                array[i].debugOutput(os);
            }
            os << " ]";
            break;
        case DataType::DICT:
            os << "{ ";
            u32 i = 0;
            for (auto const& pair : dict)
            {
                if (i > 0)
                {
                    os << ", ";
                }
                os << pair.first << ": ";
                pair.second.debugOutput(os);
                ++i;
            }
            os << " }";
            break;
    }
}

