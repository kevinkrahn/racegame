#include "datafile.h"
#include <iomanip>
#include <cctype>

using namespace DataFile;

Value DataFile::load(const char* filename)
{
    // text format
    if (fs::path(filename).extension() == ".txt")
    {
        std::ifstream file(filename);
        if (!file)
        {
            error("Failed to load data file: ", filename, '\n');
            return Value();
        }

        std::ostringstream stream;
        stream << file.rdbuf();
        file.close();

        std::string str = stream.str();

        // strip comments
        for (;;)
        {
            auto pos = str.find("//");
            if (pos == std::string::npos)
            {
                break;
            }
            auto endOfLine = str.find('\n', pos);
            str = str.replace(pos,
                    endOfLine == std::string::npos ? std::string::npos : endOfLine - pos, "");
        }

        auto begin = str.cbegin();
        Value val = Value::readValue(begin, str.cend());
#ifndef NDEBUG
        val.keyName = "root";
#endif
        return val;
    }

    // binary format
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        error("Failed to load data file: ", filename, '\n');
        return Value();
    }

    file.seekg(0);

    u32 header;
    file.read((char*)&header, sizeof(u32));
    if (header != MAGIC_NUMBER)
    {
        error("Invalid data file: ", filename, '\n');
        return Value();
    }

    Value val = Value::readValue(file);
#ifndef NDEBUG
    val.keyName = "root";
#endif
    return val;
}

Value Value::readValue(std::string::const_iterator& ch, std::string::const_iterator end)
{
    auto eatSpace = [&](std::string::const_iterator& ch) {
        while(ch != end && std::isspace(*ch))
        {
            ++ch;
        }
    };
    auto readIdentifier = [&](std::string::const_iterator& ch, std::string const& ident) {
        for (u32 i=0; i<ident.size(); ++i, ++ch)
        {
            if (ch == end || *ch != ident[i])
            {
                return false;
            }
        }
        return true;
    };

    // TODO: add support for math expressions

    eatSpace(ch);

    if (ch == end)
    {
        return Value();
    }

    if (*ch == 't')
    {
        if (readIdentifier(ch, "true"))
        {
            Value val;
            val.dataType = DataType::BOOL;
            val.bool_ = true;
            return val;
        }
        return Value();
    }
    else if (*ch == 'f')
    {
        if (readIdentifier(ch, "false"))
        {
            Value val;
            val.dataType = DataType::BOOL;
            val.bool_ = false;
            return val;
        }
        return Value();
    }
    else if (*ch == '"')
    {
        std::string string = "";
        bool hasEnd = false;
        while(++ch != end)
        {
            if (*ch == '"')
            {
                hasEnd = true;
                ++ch;
                break;
            }
            string += *ch;
        }

        if (!hasEnd)
        {
            error("String has no terminating quotation.\n");
            return Value();
        }

        Value val;
        val.dataType = DataType::STRING;
        new (&val.str_) String(std::move(string));
        return val;
    }
    else if (std::isdigit(*ch) || *ch == '-')
    {
        bool foundDot = false;
        std::string digits = "";
        digits += *ch;
        while(++ch != end)
        {
            if (*ch == '.')
            {
                if (foundDot)
                {
                    break;
                }
                foundDot = true;
            }
            else if (!std::isdigit(*ch))
            {
                break;
            }
            digits += *ch;
        }
        Value val;
        if (foundDot)
        {
            val.dataType = DataType::F64;
            val.real_ = std::stod(digits);
        }
        else
        {
            val.dataType = DataType::I64;
            val.integer_ = std::stoll(digits);
        }
        return val;
    }
    else if (*ch == '{')
    {
        Value val;
        val.dataType = DataType::DICT;
        new (&val.dict_) Dict();
        ++ch;
        while (ch != end)
        {
            eatSpace(ch);
            if (ch == end)
            {
                error("Unexpected end of string when parsing Dict. \n");
                return Value();
            }
            if (std::isalpha(*ch))
            {
                std::string identifier = "";
                while (ch != end && (std::isalpha(*ch) || std::isdigit(*ch) || *ch == '-'))
                {
                    identifier += *ch++;
                }

                if (ch == end)
                {
                    error("Unexpected end of string when parsing Dict. \n");
                    return Value();
                }

                if (*ch != ':')
                {
                    error("Expected ':' but found '", *ch, "'\n");
                    return Value();
                }
                ++ch;

                val.dict_[identifier] = readValue(ch, end);
#ifndef NDEBUG
                val.dict_[identifier].keyName = identifier;
#endif

                eatSpace(ch);
                if (ch == end)
                {
                    error("Unexpected end of string when parsing Dict. \n");
                    return Value();
                }

                bool comma = false;
                if (*ch == ',')
                {
                    comma = true;
                    ++ch;
                    eatSpace(ch);
                    if (ch == end)
                    {
                        error("Unexpected end of string when parsing Dict. \n");
                        return Value();
                    }
                }

                if (*ch == '}')
                {
                    ++ch;
                    return val;
                }

                if (!comma && *ch != ',')
                {
                    error("Expected ',' but found '", *ch, "'\n");
                    return Value();
                }
            }
            else
            {
                error("Unexpected character '", *ch, "'\n");
                return Value();
            }
        }
        return val;
    }
    else if (*ch == '[')
    {
        Value val;
        val.dataType = DataType::ARRAY;
        new (&val.array_) Array();
        while (++ch != end)
        {
            eatSpace(ch);
            if (ch == end)
            {
                return Value();
            }

            Value v = readValue(ch, end);
            bool foundValue = v.dataType != DataType::NONE;
            if (foundValue)
            {
                val.array_.emplace_back(std::move(v));
#ifndef NDEBUG
                val.array_.back().keyName = std::to_string(val.array_.size() - 1);
#endif
            }

            eatSpace(ch);
            if (ch == end)
            {
                return Value();
            }

            if (*ch == ']')
            {
                ++ch;
                return val;
            }

            if (foundValue && *ch != ',')
            {
                error("Expected ',' but found '", *ch, "'\n");
                return Value();
            }
        }
    }

    return Value();
}

Value Value::readValue(std::ifstream& stream)
{
    Value value;
    stream.read((char*)&value.dataType, sizeof(DataType));
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
#ifndef NDEBUG
                value.array_[i].keyName = std::to_string(i);
#endif
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
#ifndef NDEBUG
                value.dict_[key].keyName = key;
#endif
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

void Value::debugOutput(std::ostream& os, u32 indent, bool newline) const
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
            os << std::fixed << std::setprecision(4);
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
                    os << std::string((indent + 1) * 2, ' ');
                    array_[i].debugOutput(os, indent + 1, array_[i].dataType != DataType::DICT);
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

