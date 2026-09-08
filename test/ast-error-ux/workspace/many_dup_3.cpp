// Part of a 6-way unqualified ambiguity set (see many_dup_1.cpp through
// many_dup_6.cpp) used to exercise candidate-list bounding: "manyDup"
// resolves to 6 candidates, exceeding the kMaxErrorCandidates bound of 5.
namespace euxmany3
{
    void manyDup() {}
}
