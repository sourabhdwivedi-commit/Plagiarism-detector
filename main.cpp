/*
 * main.cpp  --  FILE 4 of 4 : USER INTERFACE, FOLDER INPUT, REPORTS, MAIN
 * --------------------------------------------------------------
 * Contains : ReportPrinter  (screen / .txt / .csv report writer)
 *            FolderScanner  (lists .txt files of a folder, path helpers)
 *            Input          (safe console input, no STL)
 *            PlagiarismDetector (facade over documents + engine + settings)
 *            ConsoleUI      (menu-driven interface)
 * Concepts : composition, facade pattern, file I/O, directory reading
 * Depends  : similarity.h  (which pulls in the other two headers)
 *
 * Compile  : g++ -std=c++11 -Wall -o plagiarism main.cpp
 * Run      : ./plagiarism                       (menu)
 *            ./plagiarism folder_or_file ...    (pre-load, then menu)
 *
 * Note: <dirent.h> is the C/POSIX directory API (not STL). It works on
 * Linux, macOS and MinGW (Windows).
 */

#include <cstdio>
#include <dirent.h>
#include "similarity.h"

/* ===================== REPORT PRINTER ======================== */
class ReportPrinter {
public:
    /* Human-readable report. 'out' can be stdout or an opened file. */
    static void print(FILE* out, const Array<Document*>& docs, const Array<Result>& results,
                      const Settings& st, const char* modeText) {
        fprintf(out, "\n=========== PLAGIARISM REPORT ===========\n");
        fprintf(out, "Mode       : %s\n", modeText);
        fprintf(out, "Shingle k  : %d word(s)\n", st.shingleSize);
        fprintf(out, "Weights    : Jaccard %.0f%% / Cosine %.0f%%\n",
                st.wJaccard * 100, st.wCosine * 100);
        fprintf(out, "Thresholds : SUSPICIOUS >= %.0f%%, HIGH PLAGIARISM >= %.0f%%\n",
                st.suspicious * 100, st.high * 100);
        fprintf(out, "Documents  : %d\n\n", docs.size());
        for (int i = 0; i < docs.size(); i++)
            fprintf(out, "  [%d] %-24.24s words=%d, unique=%d, shingles=%d\n", i,
                    docs[i]->getName().c_str(), docs[i]->wordCount(),
                    docs[i]->getTermFreq().size(), docs[i]->getShingles().size());
        fprintf(out, "\n%-24s %-24s %9s %9s %9s  %s\n",
                "Doc A", "Doc B", "Jaccard", "Cosine", "Combined", "Verdict");
        fprintf(out, "-----------------------------------------------------------------------"
                     "-----------------\n");
        int high = 0, sus = 0, orig = 0;
        for (int i = 0; i < results.size(); i++) {
            const Result& r = results[i];
            const char* v = st.verdict(r.combined);
            if (r.combined >= st.high) high++;
            else if (r.combined >= st.suspicious) sus++;
            else orig++;
            fprintf(out, "%-24.24s %-24.24s %8.1f%% %8.1f%% %8.1f%%  %s\n",
                    docs[r.a]->getName().c_str(), docs[r.b]->getName().c_str(),
                    r.jaccard * 100, r.cosine * 100, r.combined * 100, v);
        }
        fprintf(out, "\nSummary: %d high plagiarism, %d suspicious, %d original "
                     "(%d comparisons)\n\n", high, sus, orig, results.size());
    }

    /* Spreadsheet-friendly export */
    static void exportCsv(FILE* out, const Array<Document*>& docs,
                          const Array<Result>& results, const Settings& st) {
        fprintf(out, "doc_a,doc_b,jaccard_percent,cosine_percent,combined_percent,verdict\n");
        for (int i = 0; i < results.size(); i++) {
            const Result& r = results[i];
            fprintf(out, "\"%s\",\"%s\",%.2f,%.2f,%.2f,%s\n",
                    docs[r.a]->getName().c_str(), docs[r.b]->getName().c_str(),
                    r.jaccard * 100, r.cosine * 100, r.combined * 100,
                    st.verdict(r.combined));
        }
    }
};

/* ===================== FILE / FOLDER HELPERS ================= */
class FolderScanner {
public:
    static bool isFolder(const char* path) {
        DIR* d = opendir(path);
        if (!d) return false;
        closedir(d);
        return true;
    }
    /* collects *.txt names (alphabetically, via merge sort) */
    static bool listTextFiles(const String& folder, Array<String>& names) {
        DIR* d = opendir(folder.c_str());
        if (!d) return false;
        struct dirent* e;
        while ((e = readdir(d)) != 0) {
            String n(e->d_name);
            if (n.endsWith(".txt") || n.endsWith(".TXT")) names.push(n);
        }
        closedir(d);
        names.sort(String::lessThan);
        return true;
    }
    static String join(const String& folder, const String& name) {
        String p = folder;
        if (p.length() > 0 && p.at(p.length() - 1) != '/' && p.at(p.length() - 1) != '\\')
            p.append('/');
        p.append(name);
        return p;
    }
    static String baseName(const String& path) {
        int a = path.lastIndexOf('/'), b = path.lastIndexOf('\\');
        return path.substring((a > b ? a : b) + 1);
    }
    static bool readFile(const String& path, String& out) {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) return false;
        int c;
        while ((c = fgetc(f)) != EOF) out.append((char)c);
        fclose(f);
        return true;
    }
};

/* ===================== CONSOLE INPUT ========================= */
class Input {
public:
    /* reads one line; returns false only at end of input */
    static bool line(String& out) {
        out = String();
        int c;
        bool got = false;
        while ((c = getchar()) != EOF && c != '\n') {
            got = true;
            if (c != '\r') out.append((char)c);
        }
        if (c == EOF && !got) return false;
        out = out.trimmed();
        return true;
    }
    static void ask(const char* prompt, String& out) {
        printf("%s", prompt);
        fflush(stdout);
        if (!line(out)) out = String();
    }
    /* blank answer keeps 'current' */
    static int askInt(const char* prompt, int lo, int hi, int current) {
        for (;;) {
            String s;
            printf("%s [%d-%d, current %d]: ", prompt, lo, hi, current);
            fflush(stdout);
            if (!line(s) || s.length() == 0) return current;
            int v;
            if (sscanf(s.c_str(), "%d", &v) == 1 && v >= lo && v <= hi) return v;
            printf("  Please enter a whole number from %d to %d.\n", lo, hi);
        }
    }
    /* user types a percentage (0-100); returns a fraction (0-1) */
    static double askPercent(const char* prompt, double currentFraction) {
        for (;;) {
            String s;
            printf("%s [0-100, current %.0f]: ", prompt, currentFraction * 100);
            fflush(stdout);
            if (!line(s) || s.length() == 0) return currentFraction;
            double v;
            if (sscanf(s.c_str(), "%lf", &v) == 1 && v >= 0 && v <= 100) return v / 100.0;
            printf("  Please enter a number from 0 to 100.\n");
        }
    }
};

/* ===================== PLAGIARISM DETECTOR =================== */
class PlagiarismDetector {
    Array<Document*> docs;
    Array<Result> results;
    SimilarityEngine engine;
    Settings st;
    bool analyzed, allPairs;
    int suspect;
public:
    PlagiarismDetector() : engine(0.5, 0.5), analyzed(false), allPairs(false), suspect(0) {}
    ~PlagiarismDetector() { clearDocuments(); }

    void addDocument(const String& name, const String& text) {
        docs.push(new Document(name, text, st.shingleSize));
        analyzed = false;
    }
    void clearDocuments() {
        for (int i = 0; i < docs.size(); i++) delete docs[i];
        docs.clear();
        results.clear();
        analyzed = false;
        suspect = 0;
    }
    int count() const { return docs.size(); }
    const Document& doc(int i) const { return *docs[i]; }
    int getSuspect() const { return suspect; }
    void setSuspect(int i) { suspect = i; analyzed = false; }

    Settings& settings() { return st; }
    void settingsChanged() { analyzed = false; }
    void setShingleSize(int k) {                    // rebuild shingles, no re-reading
        st.shingleSize = k;
        for (int i = 0; i < docs.size(); i++) docs[i]->buildShingles(k);
        analyzed = false;
    }

    void analyze(bool all) {
        engine.setWeights(st.wJaccard, st.wCosine);
        if (all) engine.compareAll(docs, results);
        else     engine.compareWithSuspect(docs, suspect, results);
        allPairs = all;
        analyzed = true;
    }
    bool hasResults() const { return analyzed; }

    void report(FILE* out) const {
        char mode[200];
        if (allPairs) sprintf(mode, "all pairs (every document vs every other)");
        else sprintf(mode, "suspect '%.100s' vs all other documents",
                     docs[suspect]->getName().c_str());
        ReportPrinter::print(out, docs, results, st, mode);
    }
    void exportCsv(FILE* out) const { ReportPrinter::exportCsv(out, docs, results, st); }
};

/* ===================== CONSOLE UI ============================ */
class ConsoleUI {
    PlagiarismDetector det;

    void loadFile(const String& path) {
        String text;
        if (!FolderScanner::readFile(path, text)) {
            printf("  Cannot open '%s'\n", path.c_str());
            return;
        }
        if (text.trimmed().length() == 0) {
            printf("  Skipped empty file '%s'\n", path.c_str());
            return;
        }
        det.addDocument(FolderScanner::baseName(path), text);
        printf("  Loaded %s\n", FolderScanner::baseName(path).c_str());
    }
    void loadFolder(const String& folder) {
        Array<String> names;
        if (!FolderScanner::listTextFiles(folder, names)) {
            printf("  Cannot open folder '%s'\n", folder.c_str());
            return;
        }
        if (names.size() == 0) { printf("  No .txt files found in '%s'\n", folder.c_str()); return; }
        for (int i = 0; i < names.size(); i++)
            loadFile(FolderScanner::join(folder, names[i]));
    }
    void listDocs() const {
        if (det.count() == 0) { printf("  No documents loaded.\n"); return; }
        for (int i = 0; i < det.count(); i++)
            printf("  [%d]%s %-30.30s (%d words)\n", i, i == det.getSuspect() ? "*" : " ",
                   det.doc(i).getName().c_str(), det.doc(i).wordCount());
        printf("  (* = suspect document)\n");
    }
    void chooseSuspect() {
        if (det.count() == 0) { printf("  Load documents first.\n"); return; }
        listDocs();
        det.setSuspect(Input::askInt("Index of the suspect document", 0, det.count() - 1,
                                     det.getSuspect()));
        printf("  Suspect is now '%s'\n", det.doc(det.getSuspect()).getName().c_str());
    }
    void loadDemo() {
        det.addDocument(String("suspect.txt"), String(
            "A data structure is a way of storing and organizing data so that it can be "
            "modified and accessed efficiently."));
        det.addDocument(String("reference1.txt"), String(
            "Data structures are ways of organizing and storing data so that it can be "
            "accessed and modified efficiently."));
        det.addDocument(String("reference2.txt"), String(
            "The monsoon season brings heavy rainfall to the northern plains of India "
            "every year."));
        det.addDocument(String("copy_of_ref1.txt"), String(
            "Data structures are ways of organizing and storing data so that it can be "
            "accessed and modified efficiently."));
        printf("  Loaded 4 demo documents.\n");
    }
    void settingsMenu() {
        for (;;) {
            Settings& s = det.settings();
            printf("\n--- SETTINGS ---\n");
            printf(" 1) Shingle size k          : %d  (1 = single words, larger = stricter phrase match)\n", s.shingleSize);
            printf(" 2) Weights Jaccard/Cosine  : %.0f%% / %.0f%%\n", s.wJaccard * 100, s.wCosine * 100);
            printf(" 3) Verdict thresholds      : SUSPICIOUS >= %.0f%%, HIGH >= %.0f%%\n", s.suspicious * 100, s.high * 100);
            printf(" 4) Reset to defaults\n 0) Back\nChoice: ");
            fflush(stdout);
            String l;
            int ch;
            if (!Input::line(l) || sscanf(l.c_str(), "%d", &ch) != 1) return;
            if (ch == 0) return;
            if (ch == 1) {
                int k = Input::askInt("New shingle size", 1, 10, s.shingleSize);
                if (k != s.shingleSize) { det.setShingleSize(k); printf("  Shingles rebuilt with k=%d.\n", k); }
            } else if (ch == 2) {
                double j = Input::askPercent("Jaccard weight % (cosine gets the rest)", s.wJaccard);
                s.wJaccard = j;
                s.wCosine = 1.0 - j;
                det.settingsChanged();
            } else if (ch == 3) {
                double sus = Input::askPercent("SUSPICIOUS threshold %", s.suspicious);
                double hi  = Input::askPercent("HIGH PLAGIARISM threshold %", s.high);
                if (sus > 0 && sus < hi) { s.suspicious = sus; s.high = hi; det.settingsChanged(); }
                else printf("  Rejected: need 0 < suspicious < high.\n");
            } else if (ch == 4) {
                Settings d;
                det.setShingleSize(d.shingleSize);
                s = d;
                det.settingsChanged();
                printf("  Defaults restored.\n");
            }
        }
    }
    void runAnalysis() {
        if (det.count() < 2) { printf("  Load at least 2 documents first.\n"); return; }
        printf(" 1) Suspect '%s' vs all others\n 2) All pairs\n",
               det.doc(det.getSuspect()).getName().c_str());
        int mode = Input::askInt("Mode", 1, 2, 1);
        det.analyze(mode == 2);
        det.report(stdout);
    }
    void exportReport() {
        if (!det.hasResults()) { printf("  Run an analysis first (menu 7).\n"); return; }
        String name;
        Input::ask("File name (.txt for text report, .csv for spreadsheet; blank = cancel): ", name);
        if (name.length() == 0) return;
        FILE* f = fopen(name.c_str(), "w");
        if (!f) { printf("  Cannot write '%s'\n", name.c_str()); return; }
        if (name.endsWith(".csv")) det.exportCsv(f);
        else                       det.report(f);
        fclose(f);
        printf("  Report saved to '%s'\n", name.c_str());
    }
public:
    void loadPath(const char* path) {
        if (FolderScanner::isFolder(path)) loadFolder(String(path));
        else loadFile(String(path));
    }
    void run() {
        for (;;) {
            printf("\n========== PLAGIARISM DETECTION ENGINE ==========\n");
            printf(" Documents: %d   Suspect: %s\n", det.count(),
                   det.count() ? det.doc(det.getSuspect()).getName().c_str() : "-");
            printf(" 1) Load all .txt files from a folder\n 2) Add a single file\n"
                   " 3) Load demo documents\n 4) List documents\n 5) Choose suspect document\n"
                   " 6) Settings (shingle size, weights, thresholds)\n 7) Run analysis\n"
                   " 8) Show last report\n 9) Export report (.txt / .csv)\n"
                   "10) Clear all documents\n 0) Exit\nChoice: ");
            fflush(stdout);
            String l, arg;
            int ch;
            if (!Input::line(l)) break;
            if (sscanf(l.c_str(), "%d", &ch) != 1) { printf("  Please enter a number.\n"); continue; }
            if (ch == 0) break;
            switch (ch) {
                case 1:
                    Input::ask("Folder path: ", arg);
                    if (arg.length()) loadFolder(arg);
                    break;
                case 2:
                    Input::ask("File path: ", arg);
                    if (arg.length()) loadFile(arg);
                    break;
                case 3: loadDemo(); break;
                case 4: listDocs(); break;
                case 5: chooseSuspect(); break;
                case 6: settingsMenu(); break;
                case 7: runAnalysis(); break;
                case 8:
                    if (det.hasResults()) det.report(stdout);
                    else printf("  No current results. Run an analysis (menu 7).\n");
                    break;
                case 9: exportReport(); break;
                case 10: det.clearDocuments(); printf("  All documents removed.\n"); break;
                default: printf("  Unknown option.\n");
            }
        }
        printf("Goodbye.\n");
    }
};

int main(int argc, char* argv[]) {
    ConsoleUI ui;
    for (int i = 1; i < argc; i++) ui.loadPath(argv[i]);   // optional pre-loading
    ui.run();
    return 0;
}
