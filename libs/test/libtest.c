int testVar = 0xDADA;

int testLibMain() {
    return testVar + 20;
}

unsigned long long otherFunct() {
    int test = testLibMain();
    return 0xC007BEEFDEADBEEF;
}