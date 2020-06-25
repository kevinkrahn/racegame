#include "datafile.h"

using namespace DataFile;

DataFile::Value DataFile::load(const char* filename)
{
    // text format
    if (path::hasExt(filename, ".txt"))
    {
        StrBuf str = readFileString(filename);
        const char* begin = str.begin();
        Value val = Value::readValue(begin, str.end());
        return val;
    }

    // binary format
    Buffer buf = readFileBytes(filename);

    u32 header = *buf.bump<u32>();
    if (header != MAGIC_NUMBER)
    {
        error("Invalid data file: %s", filename);
        return Value();
    }

    Value val = Value::readValue(buf);
    return val;
}

void DataFile::save(DataFile::Value const& val, const char* filename)
{
    // text format
    if (path::hasExt(filename, ".txt"))
    {
        StrBuf buf;
        val.debugOutput(buf, 0, false);
        writeFile(filename, buf.data(), buf.size());
        return;
    }

    // binary format
    u32 magic = MAGIC_NUMBER;
    Buffer buf(megabytes(20));
    buf.write(magic);
    val.write(buf);
    writeFile(filename, buf.data.get(), buf.pos);
}

// TODO: Add line numbers and more descriptive messages to parser errors
Value Value::readValue(const char*& ch, const char* end)
{
    auto eatSpace = [&](const char*& ch) {
        while(ch != end && isspace(*ch))
        {
            ++ch;
        }
    };
    auto readIdentifier = [&](const char*& ch, const char* ident) {
        auto len = strlen(ident);
        for (u32 i=0; i<len; ++i, ++ch)
        {
            if (ch == end || *ch != ident[i])
            {
                return false;
            }
        }
        return true;
    };

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
        const char* identBegin = ch + 1;
        bool hasEnd = false;
        while(++ch != end)
        {
            if (*ch == '"')
            {
                hasEnd = true;
                break;
            }
        }
        ++ch;

        if (!hasEnd)
        {
            error("String has no terminating quotation.");
            return Value();
        }

        Value val;
        val.dataType = DataType::STRING;
        new (&val.str_) String(identBegin, ch - 1);
        return val;
    }
    else if (isdigit(*ch) || *ch == '-')
    {
        bool foundDot = false;
        const char* digitsBegin = ch;
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
            else if (!isdigit(*ch))
            {
                break;
            }
        }
        Str64 digits(digitsBegin, ch);
        Value val;
        if (foundDot)
        {
            val.dataType = DataType::F32;
            val.real_ = (f32)atof(digits.data());
        }
        else
        {
            val.dataType = DataType::I64;
            val.integer_ = atoll(digits.data());
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
                error("Unexpected end of string when parsing Dict.");
                return Value();
            }
            if (isalpha(*ch))
            {
                const char* identBegin = ch;
                while (ch != end && (isalpha(*ch) || isdigit(*ch) || *ch == '-'))
                {
                    ++ch;
                }

                if (ch == end)
                {
                    error("Unexpected end of string when parsing Dict.");
                    return Value();
                }

                if (*ch != ':')
                {
                    error("Expected ':' but found '%c'", *ch);
                    return Value();
                }
                ++ch;

                Str64 identifier(identBegin, ch-1);
                val.dict_[identifier] = readValue(ch, end);

                eatSpace(ch);
                if (ch == end)
                {
                    error("Unexpected end of string when parsing Dict.");
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
                        error("Unexpected end of string when parsing Dict.");
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
                    error("Expected ',' but found '%c'", *ch);
                    return Value();
                }
            }
            else
            {
                error("Unexpected character '%c'", *ch);
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
                val.array_.push_back(std::move(v));
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
                error("Expected ',' but found '%c'", *ch);
                return Value();
            }
        }
    }

    return Value();
}

Value Value::readValue(Buffer& buf)
{
    Value value;
    value.dataType = (DataType)*buf.bump<u32>();
    switch (value.dataType)
    {
        case DataType::I64:
        {
            value.integer_ = *buf.bump<i64>();
        } break;
        case DataType::F32:
        {
            value.real_ = *buf.bump<f32>();
        } break;
        case DataType::STRING:
        {
            u32 len = *buf.bump<u32>();
            char* chars = buf.bump<char>(len);
            new (&value.str_) String(chars, chars+len);
        } break;
        case DataType::BYTE_ARRAY:
        {
            // TODO: should byte array be 4-byte aligned?
            u32 len = *buf.bump<u32>();
            u8* bytes = buf.bump<u8>(len);
            new (&value.bytearray_) ByteArray(bytes, bytes+len);
        } break;
        case DataType::ARRAY:
        {
            u32 len = *buf.bump<u32>();
#if 1
            new (&value.array_) Array(len);
            for (u32 i=0; i<len; ++i)
            {
                value.array_[i] = readValue(buf);
            }
#else
            new (&value.array_) Array();
            value.array_.reserve(len);
            for (u32 i=0; i<len; ++i)
            {
                value.array_.push_back(readValue(buf));
            }
#endif
        } break;
        case DataType::DICT:
        {
            u32 len = *buf.bump<u32>();
            new (&value.dict_) Dict();
            for (u32 i=0; i<len; ++i)
            {
                u32 keyLen = *buf.bump<u32>();
                char* chars = buf.bump<char>(keyLen);
                Str64 key(chars, chars+keyLen);
                value.dict_.set(key, readValue(buf));
            }
        } break;
        case DataType::BOOL:
        {
            //value.bool_ = *buf.bump<u32>();
            value.integer_ = *buf.bump<u32>();
        } break;
        default:
        {
            error("Invalid data type: %u", (u32)value.dataType);
            value.dataType = DataType::NONE;
        } break;
    }

    return value;
}

void Value::write(Buffer& buf) const
{
    buf.write(dataType);
    switch (dataType)
    {
        case DataType::I64:
        {
            buf.write(integer_);
        } break;
        case DataType::F32:
        {
            buf.write(real_);
        } break;
        case DataType::STRING:
        {
            buf.write((u32)str_.size());
            buf.writeBytes((void*)str_.data(), (u32)str_.size());
        } break;
        case DataType::BYTE_ARRAY:
        {
            buf.write((u32)bytearray_.size());
            buf.writeBytes(bytearray_.data(), (u32)bytearray_.size());
        } break;
        case DataType::ARRAY:
        {
            buf.write((u32)array_.size());
            for (u32 i=0; i<array_.size(); ++i)
            {
                array_[i].write(buf);
            }
        } break;
        case DataType::DICT:
        {
            buf.write((u32)dict_.size());
            for (auto& pair : dict_)
            {
                u32 keyLen = (u32)pair.key.size();
                buf.write(keyLen);
                buf.writeBytes((void*)pair.key.data(), keyLen);
                pair.value.write(buf);
            }
        } break;
        case DataType::BOOL:
        {
            buf.write((u32)bool_);
        } break;
        default:
        {
            error("Cannot save value: Invalid data type: %u", (u32)dataType);
        } break;
    }
}

void Value::debugOutput(StrBuf& buf, u32 indent, bool newline) const
{
    switch (dataType)
    {
        case DataType::NONE:
            buf.write("None");
            break;
        case DataType::I64:
            buf.writef("%i", integer_);
            break;
        case DataType::F32:
            buf.writef("%.4f", real_);
            break;
        case DataType::STRING:
            buf.writef("\"%s\"", str_.data());
            break;
        case DataType::BYTE_ARRAY:
            buf.write("<bytearray>");
            break;
        case DataType::ARRAY:
            if (newline)
            {
                buf.write(' ', indent * 2);
            }
            if (array_.size() == 0)
            {
                buf.write("[]");
            }
            else
            {
                buf.write("[\n");
                for (u32 i=0; i<array_.size(); ++i)
                {
                    buf.write(' ', (indent + 1) * 2);
                    bool elementOnNewline =
                        array_[i].dataType != DataType::DICT && array_[i].dataType != DataType::ARRAY;
                    array_[i].debugOutput(buf, indent + 1, elementOnNewline);
                    buf.write(",\n");
                }
                buf.write(' ', indent * 2);
                buf.write(']');
            }
            break;
        case DataType::DICT:
            if (newline)
            {
                buf.write(' ', indent * 2);
            }
            if (dict_.size() == 0)
            {
                buf.write("{}");
            }
            else
            {
                buf.write("{\n");
                for (auto const& pair : dict_)
                {
                    buf.write(' ', (indent + 1) * 2);
                    buf.writef("%s: ", pair.key.data());
                    pair.value.debugOutput(buf, indent + 1, false);
                    buf.write(",\n");
                }
                buf.write(' ', indent * 2);
                buf.write('}');
            }
            break;
        case DataType::BOOL:
            buf.write(bool_ ? "true" : "false");
            break;
    }
}

