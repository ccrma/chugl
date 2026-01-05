float test[0];

// repeat(5) test << @(1,1);
repeat(5) test << 1.0;

<<< "before", test[4] >>>;
<<< "before", test[5] >>>;
<<< "before", test[6] >>>;

for (test.size() - 1 => int i; i >= 0; i++) {
    <<< i, test.size(), test[i] >>>;
    // test.erase(i);
}