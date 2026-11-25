# =============================================================================
#  tools/wokwi_sd.py  -  Monta wokwi-sd/ (cartao do Wokwi) a partir de SD/
# -----------------------------------------------------------------------------
#  Roda antes de cada build (extra_scripts no platformio.ini). Copia SD/ para
#  wokwi-sd/, mas cada MP3 so ate a tag ID3 + AUDIO_BYTES de audio: o Wokwi nao toca som e o SD/
#  inteiro estoura a memoria do simulador. A tag fica inteira (titulo e capa).
# =============================================================================
import os
import shutil

Import("env")  # noqa: F821 (definido pelo PlatformIO)

AUDIO_BYTES = 300 * 1024
CARD_LIMIT = 7 * 1024 * 1024   # cartao padrao do Wokwi: 8 MB

project = env.subst("$PROJECT_DIR")  # noqa: F821
target = os.path.join(project, "wokwi-sd")


def id3_size(head):
    """Bytes da tag ID3v2 no comeco do arquivo (0 se nao tiver)."""
    if len(head) < 10 or head[:3] != b"ID3":
        return 0
    size = (head[6] & 0x7F) << 21 | (head[7] & 0x7F) << 14 | (head[8] & 0x7F) << 7 | (head[9] & 0x7F)
    return 10 + size + (10 if head[5] & 0x10 else 0)


def copy_tree(source):
    total = 0
    for folder, _, files in os.walk(source):
        for name in files:
            src = os.path.join(folder, name)
            dst = os.path.join(target, os.path.relpath(src, source))
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            if name.lower().endswith(".mp3"):
                with open(src, "rb") as f:
                    keep = id3_size(f.read(10)) + AUDIO_BYTES
                    f.seek(0)
                    data = f.read(keep)
                with open(dst, "wb") as f:
                    f.write(data)
            else:
                shutil.copyfile(src, dst)
            total += os.path.getsize(dst)
    return total


shutil.rmtree(target, ignore_errors=True)
size = copy_tree(os.path.join(project, "SD"))
print("wokwi-sd: %.1f MB" % (size / 1048576.0))
if size > CARD_LIMIT:
    print("Aviso: wokwi-sd passa de 7 MB e pode nao caber no cartao do Wokwi (diminua AUDIO_BYTES)")
