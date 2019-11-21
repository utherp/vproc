
int somefunc () { return 0; }
int someother() { return 1; }

int main (int c, char *v[]) {
    int (* const myfunc)() = somefunc;

    *somefunc = (int(*)())someother;
    return 0;
}
