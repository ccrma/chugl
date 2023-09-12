// example in c++ of overloading classes outside of class definion
// also hmmmm...

#include <iostream>
using namespace std;

ostream & operator+(ostream & lhs, ostream & rhs)
{
    lhs << 1 << endl;
    rhs << 2 << endl;
    return lhs;
}

int main()
{
    // wat
    (cerr+cerr) << 3 << endl;
    return 0;
}
