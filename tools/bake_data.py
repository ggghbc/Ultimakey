#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Ultimakey Data Baker
Converts JSON dictionaries and ExtraWords into ultra-compact binary DAWG,
trigram tables, and typo correction tables for direct embedding into Ultimakey.exe resources.
"""

import os
import sys
import json
import struct
import re
from collections import deque

def extract_mac_json(text_mac, filename):
    marker = '## File: Sources/Keyboop/Resources/' + filename
    pos = text_mac.find(marker)
    if pos == -1:
        raise ValueError(f'File not found in mac repomix: {filename}')
    s_bracket = text_mac.find('[', pos)
    s_brace = text_mac.find('{', pos)
    if s_bracket != -1 and (s_brace == -1 or s_bracket < s_brace):
        start = s_bracket
        end = text_mac.find(']', start) + 1
    else:
        start = s_brace
        end = text_mac.find('}', start) + 1
    raw = text_mac[start:]
    obj, _ = json.JSONDecoder().raw_decode(raw)
    return obj

def extract_win_extrawords(text_win):
    pos = text_win.find('## File: ExtraWords.cs/ExtraWords.cs')
    if pos == -1:
        raise ValueError('ExtraWords.cs not found in win repomix')
    next_pos = text_win.find('## File: ', pos + 1)
    content = text_win[pos:next_pos]
    
    sets = {}
    matches = re.finditer(r'public static readonly HashSet<string> (\w+)\s*=\s*new[^{]*\{([^}]+)\}', content, re.DOTALL)
    for m in matches:
        name = m.group(1)
        body = m.group(2)
        items = re.findall(r'"([^"]+)"', body)
        sets[name] = items
    return sets

# -------------------------------------------------------------
# DAWG Construction & Flattening
# -------------------------------------------------------------

class DawgNode:
    _next_id = 0
    def __init__(self):
        self.id = DawgNode._next_id
        DawgNode._next_id += 1
        self.final = False
        self.edges = {} # char_id -> DawgNode

    def __hash__(self):
        return hash((self.final, tuple(sorted((c, child.id) for c, child in self.edges.items()))))

    def __eq__(self, other):
        if self is other: return True
        return self.final == other.final and len(self.edges) == len(other.edges) and \
               all(c in other.edges and self.edges[c].id == other.edges[c].id for c in self.edges)

class DawgBuilder:
    def __init__(self, char_to_id):
        self.char_to_id = char_to_id
        self.root = DawgNode()
        self.unchecked_nodes = []
        self.minimized_nodes = {}
        self.previous_word = ''

    def insert(self, word):
        common_prefix = 0
        min_len = min(len(word), len(self.previous_word))
        for i in range(min_len):
            if word[i] != self.previous_word[i]: break
            common_prefix += 1

        self._minimize(common_prefix)

        curr = self.root if not self.unchecked_nodes else self.unchecked_nodes[-1][2]

        for ch in word[common_prefix:]:
            cid = self.char_to_id[ch]
            next_node = DawgNode()
            curr.edges[cid] = next_node
            self.unchecked_nodes.append((curr, cid, next_node))
            curr = next_node
        curr.final = True
        self.previous_word = word

    def finish(self):
        self._minimize(0)

    def _minimize(self, down_to):
        while len(self.unchecked_nodes) > down_to:
            parent, cid, child = self.unchecked_nodes.pop()
            if child in self.minimized_nodes:
                parent.edges[cid] = self.minimized_nodes[child]
            else:
                self.minimized_nodes[child] = child

def serialize_dawg(builder, alphabet):
    """
    Serializes minimized DAWG into binary:
    Header:
      uint32_t magic = 0x44415747 ('DAWG')
      uint16_t alphabet_len
      uint16_t root_offset
      wchar_t  alphabet[alphabet_len] (null-terminated)
      uint32_t num_transitions
      uint32_t transitions[num_transitions]
    Each transition (32 bits):
      bits 26..31: char_id (6 bits, 0..63)
      bit 25:      is_final (1 bit)
      bit 24:      is_last_edge (1 bit)
      bits 0..23:  target_edge_index (24 bits)
    """
    # BFS traversal to assign offsets
    queue = deque([builder.root])
    ordered_nodes = []
    visited = {builder.root.id}
    while queue:
        node = queue.popleft()
        ordered_nodes.append(node)
        for cid, child in sorted(node.edges.items()):
            if child.id not in visited:
                visited.add(child.id)
                queue.append(child)

    node_edge_offset = {}
    offset = 0
    for node in ordered_nodes:
        node_edge_offset[node.id] = offset
        offset += len(node.edges)

    transitions = []
    for node in ordered_nodes:
        edge_list = sorted(node.edges.items())
        for i, (cid, child) in enumerate(edge_list):
            is_last = 1 if (i == len(edge_list) - 1) else 0
            is_final = 1 if child.final else 0
            child_offset = node_edge_offset[child.id]
            entry = (cid << 26) | (is_final << 25) | (is_last << 24) | (child_offset & 0x00FFFFFF)
            transitions.append(entry)

    # Encode binary blob
    # Magic 'DAWG'
    out = bytearray(b'DAWG')
    out += struct.pack('<H', len(alphabet))
    root_offset = node_edge_offset[builder.root.id]
    out += struct.pack('<H', root_offset)
    for c in alphabet:
        out += struct.pack('<H', ord(c))
    out += struct.pack('<I', len(transitions))
    for entry in transitions:
        out += struct.pack('<I', entry)

    return bytes(out), transitions

def verify_dawg(blob, word_list):
    """Verifies that all words are present and non-words are not."""
    magic = blob[0:4]
    assert magic == b'DAWG'
    alpha_len, root_offset = struct.unpack('<HH', blob[4:8])
    alphabet = [chr(struct.unpack('<H', blob[8+i*2:10+i*2])[0]) for i in range(alpha_len)]
    char_to_id = {c: i for i, c in enumerate(alphabet)}
    
    pos = 8 + alpha_len * 2
    num_trans = struct.unpack('<I', blob[pos:pos+4])[0]
    pos += 4
    transitions = struct.unpack(f'<{num_trans}I', blob[pos:pos+num_trans*4])

    def contains(w):
        if not w: return False
        node = root_offset
        for i, c in enumerate(w):
            cid = char_to_id.get(c, -1)
            if cid < 0: return False
            found = False
            edge_idx = node
            while True:
                entry = transitions[edge_idx]
                edge_char = (entry >> 26) & 0x3F
                is_final = (entry >> 25) & 1
                is_last = (entry >> 24) & 1
                next_node = entry & 0x00FFFFFF
                if edge_char == cid:
                    if i == len(w) - 1:
                        return bool(is_final)
                    node = next_node
                    found = True
                    break
                if is_last:
                    break
                edge_idx += 1
            if not found:
                return False
        return False

    # Sample test 1000 words + boundary
    sample = word_list[::max(1, len(word_list)//2000)]
    for w in sample:
        assert contains(w), f'Failed to find word: {w}'
    assert not contains('абвгдxyz123'), 'False positive detected!'
    return True

# -------------------------------------------------------------
# Trigrams Serialization (Compact 32-bit / Cache-friendly)
# -------------------------------------------------------------

def serialize_trigrams(trigrams_dict):
    """
    Format (32-bit compact entry):
      uint32_t magic = 0x54524732 ('TRG2')
      uint16_t alpha_len
      wchar_t  alphabet[alpha_len]
      uint32_t count
      entries:
        uint32_t packed (18 bits char keys [3 x 6-bit], 14 bits fixed-point score)
    Sorted strictly by (entry >> 14).
    """
    chars = set()
    for tri in trigrams_dict.keys():
        chars.update(tri)
    
    alphabet = [' '] + sorted([c for c in chars if c != ' '])
    assert len(alphabet) <= 64, f"Alphabet too large for 6 bits: {len(alphabet)}"
    char_to_id = {c: i for i, c in enumerate(alphabet)}

    entries = []
    for tri, score in trigrams_dict.items():
        if len(tri) != 3: continue
        id0 = char_to_id.get(tri[0], 0)
        id1 = char_to_id.get(tri[1], 0)
        id2 = char_to_id.get(tri[2], 0)
        prefix = (id0 << 12) | (id1 << 6) | id2
        mag = min(16383, max(0, int(round(-float(score) * 1000.0))))
        packed = (prefix << 14) | (mag & 0x3FFF)
        entries.append(packed)

    entries.sort(key=lambda x: x >> 14)

    out = bytearray(b'TRG2')
    out += struct.pack('<H', len(alphabet))
    for c in alphabet:
        out += struct.pack('<H', ord(c))
    out += struct.pack('<I', len(entries))
    for e in entries:
        out += struct.pack('<I', e)
    return bytes(out)

# -------------------------------------------------------------
# Typo Rules Serialization (Zero-Allocation Flat RCDATA Pool)
# -------------------------------------------------------------

def serialize_typo_rules(typo_dict):
    """
    Format:
      uint32_t magic = 0x54595032 ('TYP2')
      uint32_t count
      struct TypoIndex {
        uint32_t typo_offset; // in wchar_t
        uint16_t typo_len;
        uint32_t fix_offset;  // in wchar_t
        uint16_t fix_len;
      } index[count];
      wchar_t pool[]; // packed wchar_t characters
    Sorted lexicographically by typo.
    """
    all_rules = {}
    for lang in ['en', 'ru']:
        for k, v in typo_dict.get(lang, {}).items():
            all_rules[k] = v

    items = sorted(all_rules.items(), key=lambda x: x[0])
    indices = []
    pool = []

    for typo, fix in items:
        typo_offset = len(pool)
        typo_len = len(typo)
        pool.extend([ord(c) for c in typo])

        fix_offset = len(pool)
        fix_len = len(fix)
        pool.extend([ord(c) for c in fix])

        indices.append((typo_offset, typo_len, fix_offset, fix_len))

    out = bytearray(b'TYP2')
    out += struct.pack('<I', len(items))
    for t_off, t_len, f_off, f_len in indices:
        out += struct.pack('<IHIH', t_off, t_len, f_off, f_len)
    for c in pool:
        out += struct.pack('<H', c)
    return bytes(out)

# -------------------------------------------------------------
# ExtraWords Header Generation
# -------------------------------------------------------------

def generate_extrawords_header(extra_sets, target_file):
    """
    Generates C++ static constexpr sorted arrays of wstring_view
    for high-speed binary search lookup without allocations.
    """
    lines = [
        '#pragma once',
        '// Generated by bake_data.py - do not edit directly',
        '#include <string_view>',
        '#include <array>',
        '#include <algorithm>',
        '',
        'namespace Ultimakey::Data {',
        ''
    ]

    for name, items in sorted(extra_sets.items()):
        unique_sorted = sorted(list(set(items)))
        lines.append(f'inline constexpr std::array<std::wstring_view, {len(unique_sorted)}> {name} = {{')
        for item in unique_sorted:
            # escape backslashes and quotes
            esc = item.replace('\\', '\\\\').replace('"', '\\"')
            lines.append(f'    L"{esc}",')
        lines.append('};')
        lines.append('')
        lines.append(f'inline bool IsIn{name}(std::wstring_view s) noexcept {{')
        lines.append(f'    return std::binary_search({name}.begin(), {name}.end(), s);')
        lines.append('}')
        lines.append('')

    # Builtin hardcoded sets from LayoutDetector
    force_swap_builtin = [
        "http","https","url","uri","api","rest","json","xml","yaml","csv","html","css",
        "sdk","cli","gui","ide","ssh","ftp","tcp","udp","ip","dns","vpn","ssl","tls",
        "smtp","jwt","cors","sql","nosql","git","npm","yarn","k8s","aws","gcp","kpi",
        "crm","seo","smm","mvp","cpu","gpu","ram","ssd","hdd","usb","pdf","mp3","mp4",
        "png","jpg","jpeg","svg","gif","ddos","iot","llm","gpt","ml","ai","ui","ux",
        "db","os","io","qa","ci","cd","webp","mvc","orm","cdn","dom"
    ]
    force_swap_sorted = sorted(list(set(force_swap_builtin)))
    lines.append(f'inline constexpr std::array<std::wstring_view, {len(force_swap_sorted)}> ForceSwapBuiltin = {{')
    for item in force_swap_sorted:
        lines.append(f'    L"{item}",')
    lines.append('};')
    lines.append('inline bool IsInForceSwapBuiltin(std::wstring_view s) noexcept {')
    lines.append('    return std::binary_search(ForceSwapBuiltin.begin(), ForceSwapBuiltin.end(), s);')
    lines.append('}')
    lines.append('')

    common_en_two = sorted(["am","an","as","at","be","by","do","go","he","hi","if","in","is","it","me","my",
                           "no","of","ok","on","or","so","to","up","us","we","id","ai","ui","ux"])
    lines.append(f'inline constexpr std::array<std::wstring_view, {len(common_en_two)}> CommonEnTwoLetter = {{')
    for item in common_en_two:
        lines.append(f'    L"{item}",')
    lines.append('};')
    lines.append('inline bool IsInCommonEnTwoLetter(std::wstring_view s) noexcept {')
    lines.append('    return std::binary_search(CommonEnTwoLetter.begin(), CommonEnTwoLetter.end(), s);')
    lines.append('}')
    lines.append('')

    lines.append('} // namespace Ultimakey::Data')
    
    with open(target_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
    print(f'Wrote {target_file}')

# -------------------------------------------------------------
# Main Baking Pipeline
# -------------------------------------------------------------

def main():
    mac_dump = r"C:\Users\oakmaster\Desktop\repomix-output-iffuno-keyboop.md"
    win_dump = r"C:\Users\oakmaster\Desktop\repomix-output-KeyboopWin.zip.md"
    out_dir = r"E:\DEV\Ultimakey\src\data"
    os.makedirs(out_dir, exist_ok=True)

    print("Reading repomix dumps...")
    with open(mac_dump, 'r', encoding='utf-8', errors='ignore') as f:
        text_mac = f.read()
    with open(win_dump, 'r', encoding='utf-8', errors='ignore') as f:
        text_win = f.read()

    print("Extracting ExtraWords...")
    extra_sets = extract_win_extrawords(text_win)
    
    # RU Words: words_ru + RuAbbr + RuLoanNames + RuDev + RuShort + RuCommonForms + Ru
    print("Preparing Russian dictionary...")
    w_ru = set(extract_mac_json(text_mac, 'words_ru.json'))
    for sname in ['RuAbbr', 'RuLoanNames', 'RuDev', 'RuShort', 'RuCommonForms', 'Ru']:
        if sname in extra_sets:
            w_ru.update(extra_sets[sname])
    w_ru_list = sorted([w.lower() for w in w_ru if w and all(c >= 'а' and c <= 'я' or c == 'ё' or c == '-' for c in w.lower())])
    print(f"Total RU words: {len(w_ru_list)}")

    # EN Words: words_en + En
    print("Preparing English dictionary...")
    w_en = set(extract_mac_json(text_mac, 'words_en.json'))
    if 'En' in extra_sets:
        w_en.update(extra_sets['En'])
    w_en_list = sorted([w.lower() for w in w_en if w and all(c >= 'a' and c <= 'z' or c == '\'' or c == '-' for c in w.lower())])
    print(f"Total EN words: {len(w_en_list)}")

    # Build RU DAWG
    print("Building RU DAWG...")
    alpha_ru = sorted(list(set(''.join(w_ru_list))))
    char_to_id_ru = {c: i for i, c in enumerate(alpha_ru)}
    builder_ru = DawgBuilder(char_to_id_ru)
    for w in w_ru_list:
        builder_ru.insert(w)
    builder_ru.finish()
    blob_ru, _ = serialize_dawg(builder_ru, alpha_ru)
    print(f"Serialized RU DAWG: {len(blob_ru)} bytes (~{round(len(blob_ru)/1024)} KB)")
    verify_dawg(blob_ru, w_ru_list)
    ru_dawg_file = os.path.join(out_dir, "ru_dawg.bin")
    with open(ru_dawg_file, 'wb') as f:
        f.write(blob_ru)
    print(f"Saved {ru_dawg_file}")

    # Build EN DAWG
    print("Building EN DAWG...")
    alpha_en = sorted(list(set(''.join(w_en_list))))
    char_to_id_en = {c: i for i, c in enumerate(alpha_en)}
    builder_en = DawgBuilder(char_to_id_en)
    for w in w_en_list:
        builder_en.insert(w)
    builder_en.finish()
    blob_en, _ = serialize_dawg(builder_en, alpha_en)
    print(f"Serialized EN DAWG: {len(blob_en)} bytes (~{round(len(blob_en)/1024)} KB)")
    verify_dawg(blob_en, w_en_list)
    en_dawg_file = os.path.join(out_dir, "en_dawg.bin")
    with open(en_dawg_file, 'wb') as f:
        f.write(blob_en)
    print(f"Saved {en_dawg_file}")

    # Trigrams
    print("Serializing Trigrams...")
    tri_ru = extract_mac_json(text_mac, 'trigrams_ru.json')
    tri_ru_blob = serialize_trigrams(tri_ru)
    tri_ru_file = os.path.join(out_dir, "ru_trigrams.bin")
    with open(tri_ru_file, 'wb') as f:
        f.write(tri_ru_blob)
    print(f"Saved {tri_ru_file}: {len(tri_ru_blob)} bytes (~{round(len(tri_ru_blob)/1024)} KB)")

    tri_en = extract_mac_json(text_mac, 'trigrams_en.json')
    tri_en_blob = serialize_trigrams(tri_en)
    tri_en_file = os.path.join(out_dir, "en_trigrams.bin")
    with open(tri_en_file, 'wb') as f:
        f.write(tri_en_blob)
    print(f"Saved {tri_en_file}: {len(tri_en_blob)} bytes (~{round(len(tri_en_blob)/1024)} KB)")

    # Typo Rules
    print("Serializing Typo Rules...")
    typo_rules = extract_mac_json(text_mac, 'typo_rules.json')
    typo_blob = serialize_typo_rules(typo_rules)
    typo_file = os.path.join(out_dir, "typo_rules.bin")
    with open(typo_file, 'wb') as f:
        f.write(typo_blob)
    print(f"Saved {typo_file}: {len(typo_blob)} bytes (~{round(len(typo_blob)/1024)} KB)")

    # Generate extrawords_data.hpp
    print("Generating extrawords_data.hpp...")
    header_file = os.path.join(out_dir, "extrawords_data.hpp")
    # remove word sets that are now inside DAWG
    sets_for_header = {k: v for k, v in extra_sets.items() if k not in ['RuAbbr', 'RuLoanNames', 'RuDev', 'RuShort', 'RuCommonForms', 'Ru', 'En']}
    generate_extrawords_header(sets_for_header, header_file)

    print("\nData Baking complete! All resources ready.")

if __name__ == '__main__':
    main()
