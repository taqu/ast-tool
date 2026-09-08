class Widget {
public:
    Widget();
    ~Widget();
    void update();
    static void create();
    inline void reset();
private:
    int value_;
};
