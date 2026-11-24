#include "Library.hpp"

#include <algorithm>

namespace {

class FileReader : public ByteReader
{
    private:
        fs::File &file;

    public:
        explicit FileReader(fs::File &f) : file(f) {}

        uint32_t size() override
        {
            return file.size();
        }

        bool readAt(uint32_t pos, uint8_t *buf, size_t len) override
        {
            if (!file.seek(pos)) {
                return false;
            }
            return file.read(buf, len) == len;
        }
};

bool isMp3(const String &name)
{
    String lower = name;
    lower.toLowerCase();
    return lower.endsWith(".mp3") && !lower.startsWith(".");   // ignora "._arquivo" do macOS
}

String baseName(const String &path)
{
    int slash = path.lastIndexOf('/');
    String name = path.substring(slash + 1);
    int dot = name.lastIndexOf('.');
    return dot > 0 ? name.substring(0, dot) : name;
}

} // namespace

size_t Library::scan(fs::FS &fs, const char *folder)
{
    paths.clear();

    File dir = fs.open(folder);
    if (!dir || !dir.isDirectory()) {
        return 0;
    }

    String base = folder;
    if (!base.endsWith("/")) {
        base += "/";
    }

    File entry = dir.openNextFile();
    while (entry && paths.size() < MAX_TRACKS) {
        if (!entry.isDirectory()) {
            String name = entry.name();
            name = name.substring(name.lastIndexOf('/') + 1);
            if (isMp3(name)) {
                paths.push_back(base + name);
            }
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();

    std::sort(paths.begin(), paths.end(), [](const String &a, const String &b) {
        return a.compareTo(b) < 0;
    });
    return paths.size();
}

bool Library::load(fs::FS &fs, size_t index, Mp3Info &info) const
{
    info = Mp3Info();
    if (index >= paths.size()) {
        return false;
    }

    bool ok = false;
    File file = fs.open(paths[index]);
    if (file) {
        FileReader reader(file);
        ok = readMp3Info(reader, info);
        file.close();
    }

    if (info.title.empty())  info.title  = baseName(paths[index]).c_str();
    if (info.artist.empty()) info.artist = "Artista desconhecido";
    if (info.album.empty())  info.album  = "Album desconhecido";
    return ok;
}
