# =============================================================================
#  tools/fetch_covers.py  -  Baixa as capas que faltam para as musicas do SD/
# -----------------------------------------------------------------------------
#  Uso (na pasta KiMusic):   python tools/fetch_covers.py [pasta do cartao]
#
#  Para cada MP3 de <cartao>/music sem capa embutida e sem <cartao>/cover/<nome>.jpg,
#  busca artista + titulo (da tag ID3, ou do nome do arquivo) na busca publica do
#  iTunes e salva a capa em <cartao>/cover/<nome>.jpg, que o player ja procura.
#  So usa a biblioteca padrao do Python.
#
#  shortcut: pega o 1o resultado da busca; titulos de video ("[Official Music Video]",
#  "| canal") sao limpos antes, mas a capa pode vir errada. Apague o .jpg e
#  renomeie a musica/tag para tentar de novo.
# =============================================================================
import json
import os
import re
import sys
import time
import urllib.parse
import urllib.request

SIZE = 140  # tamanho da capa na tela: maior so faz o ESP32 decodificar mais (1120 px levou 4 s)


def syncsafe(b):
    return (b[0] & 0x7F) << 21 | (b[1] & 0x7F) << 14 | (b[2] & 0x7F) << 7 | (b[3] & 0x7F)


def read_tag(path):
    """(titulo, artista, tem_capa) da tag ID3v2.3/2.4 (vazios se nao houver)."""
    with open(path, "rb") as f:
        head = f.read(10)
        if len(head) < 10 or head[:3] != b"ID3" or head[3] not in (3, 4):
            return "", "", False
        tag = f.read(syncsafe(head[6:10]))
    fields, pos = {}, 0
    while pos + 10 <= len(tag) and tag[pos] != 0:
        fid = tag[pos:pos + 4].decode("latin-1")
        size = syncsafe(tag[pos + 4:pos + 8]) if head[3] == 4 else int.from_bytes(tag[pos + 4:pos + 8], "big")
        data = tag[pos + 10:pos + 10 + size]
        pos += 10 + size
        if fid in ("TIT2", "TPE1") and data:
            enc = {0: "latin-1", 1: "utf-16", 2: "utf-16-be", 3: "utf-8"}.get(data[0], "latin-1")
            fields[fid] = data[1:].decode(enc, "ignore").strip("\x00 ")
        elif fid == "APIC":
            fields["APIC"] = True
    return fields.get("TIT2", ""), fields.get("TPE1", ""), "APIC" in fields


def clean(text):
    """Tira o que atrapalha a busca: [..], (..), '| canal', @usuario, 'Official Video'.
    O iTunes exige todas as palavras, entao "(Cowboy Bebop)" ja zera o resultado."""
    text = re.sub(r"\[[^\]]*\]|\([^)]*\)", " ", text)
    text = re.sub(r"official (music )?video|lyric video|clipe oficial", " ", text, flags=re.I)
    text = text.split("|")[0]
    text = re.sub(r"@\S+", " ", text)
    return re.sub(r"\s+", " ", text).strip(" -")


def search_cover(query):
    url = "https://itunes.apple.com/search?" + urllib.parse.urlencode(
        {"term": query, "media": "music", "entity": "song", "limit": 1})
    with urllib.request.urlopen(url, timeout=15) as r:
        results = json.load(r).get("results", [])
    if not results or "artworkUrl100" not in results[0]:
        return None
    return results[0]["artworkUrl100"].replace("100x100bb", "%dx%dbb" % (SIZE, SIZE))


def main(card):
    music, covers = os.path.join(card, "music"), os.path.join(card, "cover")
    os.makedirs(covers, exist_ok=True)
    for name in sorted(os.listdir(music)):
        if not name.lower().endswith(".mp3"):
            continue
        base = os.path.splitext(name)[0]
        if any(os.path.exists(os.path.join(covers, base + ext)) for ext in (".jpg", ".jpeg", ".png")):
            continue
        title, artist, embedded = read_tag(os.path.join(music, name))
        if embedded:
            continue
        if artist and title.startswith(artist + " - "):
            title = title[len(artist) + 3:]   # titulo de video: "Artista - Musica"
        query = clean(" ".join(x for x in (artist, title) if x) or base)
        try:
            url = search_cover(query)
            # Baixa inteiro antes de gravar: arquivo pela metade nunca seria baixado de novo.
            data = urllib.request.urlopen(url, timeout=15).read() if url else None
        except OSError as e:
            print("erro   %s: %s" % (name, e))
            continue
        finally:
            time.sleep(3)   # a busca do iTunes aceita ~20 pedidos por minuto
        if not data:
            print("nada   %s  (busca: %s)" % (name, query))
            continue
        with open(os.path.join(covers, base + ".jpg"), "wb") as f:
            f.write(data)
        print("capa   %s  (busca: %s)" % (name, query))


assert clean("YuutaMusic Horizonte (Cowboy Bebop)  | YuutaMusic x @blk.blackk  [Official Music Video]") == "YuutaMusic Horizonte"
assert clean("Slow GM Florescer (Kamisama Kiss) Official Music Video") == "Slow GM Florescer"

if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "SD"))
