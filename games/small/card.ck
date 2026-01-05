/*
IDEAS
- make zachtronics sawayama solitaire with QOL visual tooltip improvements
    - when hovering over a stack, show
        - the other copies of that card
        - the suits above
        - the suits below
    - when hovering over the deposit pile
        - show location of next card needed on stack
- while playing I noticed it was really satisfying to shuffle cards around and sort them with 1 or 2 empty slots
    - maybe this can be a mini-game in of itself

NEW IDEA: ACE Solitaire

Most solid one from playtesting with physical cards:
- 36 cards (2-10 each suit)
- 6x6 layout: 6 columns of six cards, all face-up
- 1 empty column
- iffy: allow placing cards in foundation (starting at 2, going up)
    - makes it easier to win, not sure if necessary

Other configs to Test
- 36 cards, 8 columns with 1,2,3,...,8 cards each
- 20 cards (2-6 each suit)
    - 5 cols, 4 cards per col
    - 4 cols, 5 cards per col
- 48 cards (2-K each suit)
    - 8 cols, 6 cards per suit
*/

@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"
@import "../bytepath/topograph.ck"

G2D g;
g.antialias(true);

// GText.defaultFont("chugl:proggy-tiny");

0 => int Suit_Club;
1 => int Suit_Diamond;
2 => int Suit_Heart;
3 => int Suit_Spade;
4 => int Suit_Count;

2.0 => float CARD_HEIGHT;
5.0 / 7.0 => float CARD_ASPECT;
CARD_HEIGHT * CARD_ASPECT => float CARD_WIDTH;
CARD_HEIGHT * .5 => float CARD_HH;
CARD_WIDTH * .5 => float CARD_HW;

UI_Float CARD_BORDER(.05);
UI_Float CARD_ROUNDING(.1);
UI_Float CARD_INNER_PAD(.1);
UI_Float SUIT_SYMBOL_OFFSET(-.05);

class Card {
    int val;
    int suit;
    vec2 pos;

    fun Card(int val, int suit) {
        val => this.val; suit => this.suit;
    }
}

Card deck[0];

// add A-10
for (int suit; suit < Suit_Count; suit++) {
    for (1 => int i; i <= 10; i++)
        deck << new Card(i, suit);
}

deck.shuffle();
// for (auto card : deck) {
//     <<< card.val, card.suit >>>;
// }


fun void draw(Card c) {
    CARD_ROUNDING.val() * .5 => float radius;
    g.pushPolygonRadius(radius);

    CARD_HEIGHT * CARD_ASPECT - radius => float w;
    CARD_HEIGHT - radius => float h;
    g.boxFilled(c.pos, w, h, Color.WHITE);
    CARD_BORDER.val() -=> w;
    CARD_BORDER.val() -=> h;
    g.boxFilled(c.pos, w, h, Color.BLACK);

    g.popPolygonRadius();
}

fun void drawSuit(int suit, vec2 pos, vec3 color, float sz) {
    g.pushColor(color);
    .9 => float mod; // inner padding for sz
    if (suit == Suit_Club) {
        sz * .25 => float l;
        mod * .95 *  l * 2 / Math.sqrt(3) => float r2;

        M.rot2vec(-30 * M.DEG2RAD) => vec2 dir1;
        M.rot2vec(-150 * M.DEG2RAD) => vec2 dir2;
 
        g.circleFilled(pos + .9 * l * g.UP, r2, color);
        g.circleFilled(pos + l * dir1, r2, color);
        g.circleFilled(pos + l * dir2, r2, color);
        g.circleFilled(pos, .5 * r2, color);

        g.circleFilled(pos + 1.5 * l * g.DOWN, .25 * l, color);

    }
    else if (suit == Suit_Diamond) {
        g.pushColor(color);
        g.diamond(pos, 0, mod * sz / 1.5, mod * sz);
        g.popColor();
    }
    else {
        (suit == Suit_Heart) ? 1 : -1 => int inv; // spade is just a flipped diamond

        mod * sz / (1 + M.INV_ROOT2) => float l; // side length of diamond
        l * M.INV_ROOT2 => float w; // half width of diamond
        1.5 * w + .5 * l => float h; // height of entire heart
        pos + .5 * (sz - h) * g.DOWN * inv => vec2 c; // centered position of heart
        c.y + inv * (.5-.4531) * h => c.y; // adjust center to be center of diamond

        c + .5 * w * @(-1, inv) => vec2 p1;
        c + .5 * w * @(1, inv) => vec2 p2;
        g.circleFilled(p1, l * .5, color);
        g.circleFilled(p2, l * .5, color);
        g.diamond(c, 0, 2*w, 2*w);

        // g.square(p1, 0, l, Color.WHITE);
        // g.square(pos, 0, 2*w, Color.WHITE);
    }
    g.popColor();

    // bbox
    // g.square(pos, 0, sz, Color.WHITE);
} 


Card@ held_card;
vec2 held_card_delta;



while (1) {
    GG.nextFrame() => now;

    // ui
    UI.slider("card inner pad", CARD_INNER_PAD, 0, 1);
    UI.slider("suit symbol offset", SUIT_SYMBOL_OFFSET, -1, 1);

    drawSuit(Suit_Club, @(0, 0), Color.WHITE, 1.0);
    drawSuit(Suit_Diamond, @(0, 1), Color.WHITE, 1.0);
    drawSuit(Suit_Heart, @(0, -1), Color.WHITE, 1.0);
    drawSuit(Suit_Spade, @(0, -2), Color.WHITE, 1.0);

    deck[0] @=> Card c;

    Color.WHITE => vec3 col;

    if (g.mouseLeftDown()) {
        if (M.inside(g.mousePos(), M.aabb(c.pos, CARD_HW, CARD_HH))) {
            c @=> held_card;
            held_card.pos - g.mousePos() => held_card_delta;
        } 
    }

    if (g.mouseLeftUp()) {
        null @=> held_card;
    }

    if (held_card != null) g.mousePos() + held_card_delta => held_card.pos;


    // M.aabb(c.pos, CARD_HW, CARD_HH) => vec4 aabb;
    // aabb.z - aabb.x => float w;
    // aabb.w - aabb.y => float h;
    // g.boxFilled(c.pos, w, h, Color.GREEN);


    draw(c);
    g.pushLayer(2);
    c.pos + CARD_HH * g.UP + CARD_HW * g.RIGHT => vec2 top_right;
    c.pos + CARD_HH * g.UP + CARD_HW * g.LEFT => vec2 top_left;
    .5 * CARD_HW => float suit_size;

    g.pushTextControlPoint(0, 1);
    g.text("" + c.val, top_left + CARD_INNER_PAD.val() * @(1, -1), 1.1*suit_size);
    g.popTextControlPoint();

    drawSuit(c.suit, top_right - (.5 * suit_size + CARD_INNER_PAD.val() + SUIT_SYMBOL_OFFSET.val()) * @(1, 1), col, suit_size);
    // drawSuit(c.suit, top_left - (.5 * suit_size + CARD_INNER_PAD.val() + SUIT_SYMBOL_OFFSET.val()) * @(-2, 1), col, suit_size);

    // M.inside()


}
