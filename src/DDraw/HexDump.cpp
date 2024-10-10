#include <cctype>
#include <iomanip>

#include "HexDump.h"

void taflib::HexDump(const void* _buff, std::size_t size, std::ostream& s)
{
    const unsigned char* buff = (const unsigned char*)_buff;
    for (std::size_t base = 0; base < size; base += 16)
    {
        s << std::setw(4) << std::setfill('0') << std::uppercase << std::hex << base << ": ";
        for (std::size_t ofs = 0; ofs < 16; ++ofs)
        {
            std::size_t idx = base + ofs;
            if (idx < size)
            {
                unsigned byte = buff[idx] & 0x0ff;
                s << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << byte << ' ';
            }
            else
            {
                s << "   ";
            }
        }
        for (std::size_t ofs = 0; ofs < 16; ++ofs)
        {
            std::size_t idx = base + ofs;
            if (idx < size && std::isprint(buff[idx]))
            {
                s << buff[idx];
            }
            else
            {
                s << " ";
            }
        }
        s << std::endl;
    }
}

void taflib::StrHexDump(const void* _buff, std::size_t size, std::ostream& s)
{
    const unsigned char* buff = (const unsigned char*)_buff;
    const unsigned bytesPerLine = 32;

    for (std::size_t n = 0u; n < size;)
    {
        if (n % bytesPerLine == 0)
        {
            s << '"';
        }

        unsigned byte = buff[n] & 0x0ff;
        s << '\\' << 'x' << std::setfill('0') << std::setw(2) << std::hex << byte;

        ++n;
        if (n % bytesPerLine == 0 || n == size)
        {
            s << '"' << '\n';
        }
    }
}
