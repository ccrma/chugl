HnkyTonk h => Pan2 p => Chorus c[2] => NRev r[2] => dac;

// turn volume down a bit
0.33 => r[0].gain => r[1].gain;
0.07 => c[0].mix => c[1].mix;
0.04 => r[0].mix => r[1].mix;