/*
 * string_array.h  --  FILE 1 of 4 : BASIC DATA STRUCTURES
 * --------------------------------------------------------------
 * Contains : String      (own dynamic string, djb2 hash, path/text helpers)
 *            Array<T>    (template dynamic array + O(n log n) merge sort)
 * Concepts : encapsulation, constructors / destructors,
 *            Rule of Three (copy ctor, assignment, destructor), templates
 * Depends  : nothing
 */
#ifndef STRING_ARRAY_H
#define STRING_ARRAY_H

/* ===================== 1. STRING CLASS ======================= */
class String {
    char* data;
    int len;
    int cap;
    void grow(int minCap) {
        int nc = cap;
        while (nc < minCap) nc *= 2;
        char* nd = new char[nc];
        for (int i = 0; i < len; i++) nd[i] = data[i];
        nd[len] = '\0';
        delete[] data;
        data = nd;
        cap = nc;
    }
public:
    String() : data(new char[8]), len(0), cap(8) { data[0] = '\0'; }
    String(const char* s) : len(0) {
        while (s[len]) len++;
        cap = len + 1;
        data = new char[cap];
        for (int i = 0; i <= len; i++) data[i] = s[i];
    }
    String(const String& o) : len(o.len), cap(o.len + 1) {          // copy ctor
        data = new char[cap];
        for (int i = 0; i <= len; i++) data[i] = o.data[i];
    }
    String& operator=(const String& o) {                            // assignment
        if (this != &o) {
            char* nd = new char[o.len + 1];
            for (int i = 0; i <= o.len; i++) nd[i] = o.data[i];
            delete[] data;
            data = nd;
            len = o.len;
            cap = o.len + 1;
        }
        return *this;
    }
    ~String() { delete[] data; }                                    // destructor

    int length() const { return len; }
    char at(int i) const { return data[i]; }
    const char* c_str() const { return data; }

    void append(char c) {
        if (len + 1 >= cap) grow(cap * 2);
        data[len++] = c;
        data[len] = '\0';
    }
    void append(const String& s) {
        for (int i = 0; i < s.len; i++) append(s.data[i]);
    }
    bool endsWith(const char* suffix) const {
        int sl = 0;
        while (suffix[sl]) sl++;
        if (sl > len) return false;
        for (int i = 0; i < sl; i++)
            if (data[len - sl + i] != suffix[i]) return false;
        return true;
    }
    int lastIndexOf(char c) const {
        for (int i = len - 1; i >= 0; i--)
            if (data[i] == c) return i;
        return -1;
    }
    String substring(int from) const {                 // text from index 'from' to end
        String r;
        for (int i = from; i < len; i++) r.append(data[i]);
        return r;
    }
    String trimmed() const {                            // without leading/trailing blanks
        int a = 0, b = len;
        while (a < b && (data[a] == ' ' || data[a] == '\t' || data[a] == '\r' || data[a] == '\n')) a++;
        while (b > a && (data[b-1] == ' ' || data[b-1] == '\t' || data[b-1] == '\r' || data[b-1] == '\n')) b--;
        String r;
        for (int i = a; i < b; i++) r.append(data[i]);
        return r;
    }
    static bool lessThan(const String& x, const String& y) {   // alphabetical order
        int n = x.len < y.len ? x.len : y.len;
        for (int i = 0; i < n; i++)
            if (x.data[i] != y.data[i]) return x.data[i] < y.data[i];
        return x.len < y.len;
    }
    bool equals(const String& o) const {
        if (len != o.len) return false;
        for (int i = 0; i < len; i++)
            if (data[i] != o.data[i]) return false;
        return true;
    }
    unsigned long hash() const {                                    // djb2 hash
        unsigned long h = 5381;
        for (int i = 0; i < len; i++) h = ((h << 5) + h) + (unsigned char)data[i];
        return h;
    }
};

/* ================= 2. DYNAMIC ARRAY (TEMPLATE) =============== */
template <class T>
class Array {
    T* items;
    int n;
    int cap;
    void grow() {
        int nc = cap * 2;
        T* ni = new T[nc];
        for (int i = 0; i < n; i++) ni[i] = items[i];
        delete[] items;
        items = ni;
        cap = nc;
    }
    /* merge sort helper: sorts a[lo..hi) using tmp as scratch space */
    static void mergeSort(T* a, T* tmp, int lo, int hi,
                          bool (*before)(const T&, const T&)) {
        if (hi - lo < 2) return;
        int mid = (lo + hi) / 2;
        mergeSort(a, tmp, lo, mid, before);
        mergeSort(a, tmp, mid, hi, before);
        int i = lo, j = mid, k = lo;
        while (i < mid && j < hi) {
            if (before(a[j], a[i])) tmp[k++] = a[j++];   // right strictly first
            else                    tmp[k++] = a[i++];   // ties keep order (stable)
        }
        while (i < mid) tmp[k++] = a[i++];
        while (j < hi)  tmp[k++] = a[j++];
        for (k = lo; k < hi; k++) a[k] = tmp[k];
    }
public:
    Array() : items(new T[4]), n(0), cap(4) {}
    Array(const Array& o) : items(new T[o.cap]), n(o.n), cap(o.cap) {
        for (int i = 0; i < n; i++) items[i] = o.items[i];
    }
    Array& operator=(const Array& o) {
        if (this != &o) {
            T* ni = new T[o.cap];
            for (int i = 0; i < o.n; i++) ni[i] = o.items[i];
            delete[] items;
            items = ni;
            n = o.n;
            cap = o.cap;
        }
        return *this;
    }
    ~Array() { delete[] items; }

    void push(const T& v) { if (n == cap) grow(); items[n++] = v; }
    int size() const { return n; }
    T& operator[](int i) { return items[i]; }
    const T& operator[](int i) const { return items[i]; }
    /* Merge sort, O(n log n), stable. before(a,b) is true when a belongs before b. */
    void sort(bool (*before)(const T&, const T&)) {
        if (n < 2) return;
        T* tmp = new T[n];
        mergeSort(items, tmp, 0, n, before);
        delete[] tmp;
    }
    void clear() { n = 0; }
};

#endif
