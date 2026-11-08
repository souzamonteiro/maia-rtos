struct Point {
    i32 x;
    i32 y;
};

i32 manhattan(Point* p) {
    return p->x + p->y;
}

i32 main() {
    Point pt;
    pt.x = 20;
    pt.y = 22;
    return manhattan(&pt);
}

