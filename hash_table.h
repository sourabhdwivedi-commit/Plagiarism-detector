/*
 * hash_table.h  --  FILE 2 of 4 : TEXT PROCESSING
 * --------------------------------------------------------------
 * Contains : HashTable  (separate chaining, auto rehash)
 *            Tokenizer  (lower-case, strip punctuation, split words)
 *            Document   (word-frequency table + re-buildable shingle set)
 * Concepts : hashing, linked lists, load factor, static utility class
 * Depends  : string_array.h
 */
#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "string_array.h"

/* ============ 3. HASH TABLE (SEPARATE CHAINING) ============== */
/* Maps String -> int. Used as a SET (Jaccard) and as a
 * FREQUENCY MAP (Cosine).  Average O(1) insert / lookup.        */
struct Node {
    String key;
    int value;
    Node* next;
    Node(const String& k, int v, Node* nx) : key(k), value(v), next(nx) {}
};

class HashTable {
    Node** buckets;
    int cap;
    int count;

    void rehash() {                       // keep load factor <= 0.75
        int ncap = cap * 2;
        Node** nb = new Node*[ncap];
        for (int i = 0; i < ncap; i++) nb[i] = 0;
        for (int i = 0; i < cap; i++) {
            Node* cur = buckets[i];
            while (cur) {
                Node* nx = cur->next;
                int idx = (int)(cur->key.hash() % (unsigned long)ncap);
                cur->next = nb[idx];
                nb[idx] = cur;
                cur = nx;
            }
        }
        delete[] buckets;
        buckets = nb;
        cap = ncap;
    }
    HashTable(const HashTable&);              // non-copyable (private)
    HashTable& operator=(const HashTable&);
public:
    HashTable() : cap(16), count(0) {
        buckets = new Node*[cap];
        for (int i = 0; i < cap; i++) buckets[i] = 0;
    }
    ~HashTable() {
        for (int i = 0; i < cap; i++) {
            Node* cur = buckets[i];
            while (cur) { Node* nx = cur->next; delete cur; cur = nx; }
        }
        delete[] buckets;
    }

    Node* find(const String& key) const {
        int idx = (int)(key.hash() % (unsigned long)cap);
        for (Node* c = buckets[idx]; c; c = c->next)
            if (c->key.equals(key)) return c;
        return 0;
    }
    bool contains(const String& key) const { return find(key) != 0; }
    int get(const String& key) const {
        Node* n = find(key);
        return n ? n->value : 0;
    }
    /* add 'delta' to the value of key (inserts with delta if absent) */
    void add(const String& key, int delta) {
        Node* n = find(key);
        if (n) { n->value += delta; return; }
        if ((count + 1) * 4 > cap * 3) rehash();
        int idx = (int)(key.hash() % (unsigned long)cap);
        buckets[idx] = new Node(key, delta, buckets[idx]);
        count++;
    }
    void clear() {                         // remove every entry, keep the buckets
        for (int i = 0; i < cap; i++) {
            Node* cur = buckets[i];
            while (cur) { Node* nx = cur->next; delete cur; cur = nx; }
            buckets[i] = 0;
        }
        count = 0;
    }
    int size() const { return count; }
    int bucketCount() const { return cap; }
    Node* bucketHead(int i) const { return buckets[i]; }
};

/* ===================== 4. TOKENIZER ========================== */
class Tokenizer {
    static bool isAlnum(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    }
    static char toLower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
public:
    /* lower-cases text, removes punctuation, splits into words */
    static void tokenize(const String& text, Array<String>& out) {
        String cur;
        for (int i = 0; i < text.length(); i++) {
            char c = text.at(i);
            if (isAlnum(c)) cur.append(toLower(c));
            else if (cur.length() > 0) { out.push(cur); cur = String(); }
        }
        if (cur.length() > 0) out.push(cur);
    }
};

/* ===================== 5. DOCUMENT =========================== */
class Document {
    String name;
    Array<String> tokens;
    HashTable termFreq;     // word -> count          (for Cosine)
    HashTable shingles;     // k-word phrase -> 1     (for Jaccard)
    int k;                  // current shingle size
public:
    Document(const String& docName, const String& text, int shingleSize)
        : name(docName), k(shingleSize) {
        Tokenizer::tokenize(text, tokens);
        for (int i = 0; i < tokens.size(); i++) termFreq.add(tokens[i], 1);
        buildShingles(shingleSize);
    }
    /* (Re)builds the shingle set. Called again when the user tunes k, so the
     * text never has to be re-read or re-tokenized. */
    void buildShingles(int size) {
        k = size;
        shingles.clear();
        int n = tokens.size();
        if (n >= k) {
            for (int i = 0; i + k <= n; i++) {
                String sh;
                for (int j = 0; j < k; j++) {
                    if (j > 0) sh.append(' ');
                    sh.append(tokens[i + j]);
                }
                if (!shingles.contains(sh)) shingles.add(sh, 1);   // set semantics
            }
        } else if (n > 0) {                  // very short doc: whole text = 1 shingle
            String sh;
            for (int j = 0; j < n; j++) { if (j) sh.append(' '); sh.append(tokens[j]); }
            shingles.add(sh, 1);
        }
    }
    const String& getName() const { return name; }
    int wordCount() const { return tokens.size(); }
    int getShingleSize() const { return k; }
    const HashTable& getTermFreq() const { return termFreq; }
    const HashTable& getShingles() const { return shingles; }
};

#endif
