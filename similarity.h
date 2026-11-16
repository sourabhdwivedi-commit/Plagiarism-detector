/*
 * similarity.h  --  FILE 3 of 4 : SIMILARITY ENGINE
 * --------------------------------------------------------------
 * Contains : SimilarityMeasure (abstract base class)
 *            JaccardSimilarity, CosineSimilarity (derived classes)
 *            mySqrt (Newton-Raphson), Settings (tunable k / weights /
 *            thresholds), Result, SimilarityEngine
 * Concepts : abstraction, inheritance, runtime polymorphism,
 *            composition, set / vector mathematics
 * Depends  : hash_table.h
 */
#ifndef SIMILARITY_H
#define SIMILARITY_H

#include "hash_table.h"

/* ========= 6. SIMILARITY MEASURES (ABSTRACTION + POLYMORPHISM) ========= */
class SimilarityMeasure {
public:
    virtual double compute(const Document& a, const Document& b) const = 0;
    virtual const char* name() const = 0;
    virtual ~SimilarityMeasure() {}
};

/* Newton-Raphson square root (replaces <cmath> sqrt) */
static double mySqrt(double x) {
    if (x <= 0) return 0;
    double g = x > 1 ? x / 2 : 1;
    for (int i = 0; i < 60; i++) g = 0.5 * (g + x / g);
    return g;
}

/*  J(A,B) = |A and B| / |A or B| = inter / (|A| + |B| - inter)  */
class JaccardSimilarity : public SimilarityMeasure {
public:
    double compute(const Document& a, const Document& b) const {
        const HashTable& A = a.getShingles();
        const HashTable& B = b.getShingles();
        if (A.size() == 0 && B.size() == 0) return 0.0;
        int inter = 0;
        for (int i = 0; i < A.bucketCount(); i++)
            for (Node* n = A.bucketHead(i); n; n = n->next)
                if (B.contains(n->key)) inter++;
        int uni = A.size() + B.size() - inter;
        return uni == 0 ? 0.0 : (double)inter / (double)uni;
    }
    const char* name() const { return "Jaccard"; }
};

/*  cos(theta) = (A . B) / (|A| |B|)  over term-frequency vectors  */
class CosineSimilarity : public SimilarityMeasure {
public:
    double compute(const Document& a, const Document& b) const {
        const HashTable& A = a.getTermFreq();
        const HashTable& B = b.getTermFreq();
        double dot = 0, na = 0, nb = 0;
        for (int i = 0; i < A.bucketCount(); i++)
            for (Node* n = A.bucketHead(i); n; n = n->next) {
                na += (double)n->value * n->value;
                dot += (double)n->value * B.get(n->key);   // 0 if word absent in B
            }
        for (int i = 0; i < B.bucketCount(); i++)
            for (Node* n = B.bucketHead(i); n; n = n->next)
                nb += (double)n->value * n->value;
        if (na == 0 || nb == 0) return 0.0;
        return dot / (mySqrt(na) * mySqrt(nb));
    }
    const char* name() const { return "Cosine"; }
};

/* User-tunable parameters (changed from the Settings menu in main.cpp) */
struct Settings {
    int shingleSize;                 // k : words per shingle (Jaccard)
    double wJaccard, wCosine;        // weights, always sum to 1
    double suspicious, high;         // verdict thresholds (fractions, 0..1)
    Settings() : shingleSize(2), wJaccard(0.5), wCosine(0.5),
                 suspicious(0.40), high(0.70) {}
    const char* verdict(double score) const {
        if (score >= high) return "HIGH PLAGIARISM";
        if (score >= suspicious) return "SUSPICIOUS";
        return "ORIGINAL";
    }
};

/* One comparison result between two documents */
struct Result {
    int a, b;                              // indexes of the two documents
    double jaccard, cosine, combined;
    Result() : a(0), b(0), jaccard(0), cosine(0), combined(0) {}
    static bool higher(const Result& x, const Result& y) { return x.combined > y.combined; }
};

/* Runs both measures on document pairs and produces a ranked Result list */
class SimilarityEngine {
    SimilarityMeasure* jac;                // base-class pointers -> polymorphism
    SimilarityMeasure* cos;
    double wJ, wC;                         // weights: combined = wJ*J + wC*cos

    SimilarityEngine(const SimilarityEngine&);             // non-copyable
    SimilarityEngine& operator=(const SimilarityEngine&);

    Result compare(const Array<Document*>& docs, int i, int j) const {
        Result r;
        r.a = i;
        r.b = j;
        r.jaccard  = jac->compute(*docs[i], *docs[j]);
        r.cosine   = cos->compute(*docs[i], *docs[j]);
        r.combined = wJ * r.jaccard + wC * r.cosine;
        return r;
    }
public:
    SimilarityEngine(double wJaccard, double wCosine)
        : jac(new JaccardSimilarity()), cos(new CosineSimilarity()),
          wJ(wJaccard), wC(wCosine) {}
    ~SimilarityEngine() { delete jac; delete cos; }

    void setWeights(double wJaccard, double wCosine) { wJ = wJaccard; wC = wCosine; }

    /* every document against every other document */
    void compareAll(const Array<Document*>& docs, Array<Result>& out) const {
        out.clear();
        for (int i = 0; i < docs.size(); i++)
            for (int j = i + 1; j < docs.size(); j++)
                out.push(compare(docs, i, j));
        out.sort(Result::higher);                    // merge sort, O(n log n)
    }
    /* docs[suspect] against all the other documents (the references) */
    void compareWithSuspect(const Array<Document*>& docs, int suspect,
                            Array<Result>& out) const {
        out.clear();
        for (int j = 0; j < docs.size(); j++)
            if (j != suspect) out.push(compare(docs, suspect, j));
        out.sort(Result::higher);
    }
};

#endif
