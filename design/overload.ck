// example of operator overloading
class Overloader {
    // lua
    this.__add = function (v2) return this + v2 end

    // python 
    def __add__(self, other):
        return self + other

    // c++
    Overloader operator+(const Overloader& other) {
        return Overloader(this + other);
    }

    // chuck operator
    fun Overloader __chuck__(Overloader other) {
        this => other;
    }

    // gruck operator
    fun Overloader __gruck__(Overloader other) {
        this.SetParent(other)
        return other;
    }
}

public class GameObject {
    fun GameObject __gruck__(GameObject other) {
        this.SetParent(other)
    }
}




Overloader o1;
Overloader o2;
Overloader o3;

o1 -> o2 -> o3;

o1 + o2; // o1.__add__(o2)
