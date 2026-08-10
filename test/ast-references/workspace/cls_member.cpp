// Tests: class member reference from method scope
struct ClsVec {
    int clsX;
    int clsY;
    int clsLen() {
        return clsX + clsY;
    }
};
