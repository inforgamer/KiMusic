// =============================================================================
//  storage/FileReader.hpp  -  ByteReader sobre um arquivo do cartao SD
// -----------------------------------------------------------------------------
//  Liga os leitores de lib/media (que so conhecem ByteReader) ao fs::File
//  do Arduino. O arquivo precisa continuar aberto enquanto o reader e usado.
// =============================================================================
#ifndef STORAGE_FILEREADER_HPP
#define STORAGE_FILEREADER_HPP

#include <FS.h>

#include "ByteReader.hpp"

class FileReader : public ByteReader
{
    public:
        explicit FileReader(fs::File &f) : file(f) {}

        uint32_t size() override
        {
            return file.size();
        }

        bool readAt(uint32_t pos, uint8_t *buf, size_t len) override
        {
            return file.seek(pos) && file.read(buf, len) == len;
        }

    private:
        fs::File &file;
};

#endif
