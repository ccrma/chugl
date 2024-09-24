/*
Tests for TextureSampler
*/

TextureSampler defaultSampler;

T.assert(
    TextureSampler.WRAP_REPEAT == 0
    &&
    TextureSampler.WRAP_MIRROR == 1
    &&
    TextureSampler.WRAP_CLAMP == 2
    &&
    TextureSampler.FILTER_NEAREST == 0
    &&
    TextureSampler.FILTER_LINEAR == 1,
    "TextureSampler constants incorrect"
);

T.assert(
    defaultSampler.wrapU == TextureSampler.WRAP_REPEAT
    &&
    defaultSampler.wrapV == TextureSampler.WRAP_REPEAT
    &&
    defaultSampler.wrapW == TextureSampler.WRAP_REPEAT
    &&
    defaultSampler.filterMin == TextureSampler.FILTER_LINEAR
    &&
    defaultSampler.filterMag == TextureSampler.FILTER_LINEAR
    &&
    defaultSampler.filterMip == TextureSampler.FILTER_LINEAR,
    "defaultSampler values incorrect"
);

/* 
Texture tests
*/

