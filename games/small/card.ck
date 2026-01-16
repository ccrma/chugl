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
- do you start with aces unearthed? or should they be in the stack, and you need to uncover them first?

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

G2D g;
g.antialias(true);

// GText.defaultFont("chugl:proggy-tiny");

0 => int Suit_Club;
1 => int Suit_Diamond;
2 => int Suit_Heart;
3 => int Suit_Spade;
4 => int Suit_Count;

[
    Color.LIGHTGRAY,
    Color.RED,
    Color.RED,
    Color.LIGHTGRAY,
    // Color.GREEN,
    // Color.ORANGE,
    // Color.ORANGE,
    // Color.GREEN,
] @=> vec3 suit_colors[];

2.0 => float CARD_HEIGHT;
5.0 / 7.0 => float CARD_ASPECT;
CARD_HEIGHT * CARD_ASPECT => float CARD_WIDTH;
CARD_HEIGHT * .5 => float CARD_HH;
CARD_WIDTH * .5 => float CARD_HW;

UI_Float CARD_BORDER(.05);
UI_Float CARD_ROUNDING(.1);
UI_Float CARD_INNER_PAD(.1);
UI_Float CARD_DOTTED_SEGMENT_LENGTH(CARD_WIDTH / 10);

UI_Float SUIT_SYMBOL_OFFSET(-.05);

UI_Float STACK_OFFET_RATIO(.25); // ratio of CARD_HEIGHT to displace in Y axis when stacking

class Card {
    int val;
    int suit;

    // stack params
    Stack@ stack;
    int stack_idx;

    fun Card(int val, int suit) {
        val => this.val; suit => this.suit;
    }

    fun static int oppositeColors(int a, int b) {
        (a == Suit_Club || a == Suit_Spade) => int a_is_black;
        (b == Suit_Club || b == Suit_Spade) => int b_is_black;
        (a == Suit_Diamond || a == Suit_Heart) => int a_is_red;
        (b == Suit_Diamond || b == Suit_Heart) => int b_is_red;
        return (
            a_is_black && b_is_red
            ||
            a_is_red && b_is_black
        );
    }

    // get position in stack
    fun vec2 pos() {
        T.assert(stack != null, "cannot get pos of card not in stack");
        T.assert(stack.cards[stack_idx] == this, "stack_idx state not synced");
        return stack.cardPos(stack_idx);
    }

    fun static void draw(Card c, vec2 pos, int layer, int selected) {
        CARD_ROUNDING.val() * .5 => float radius;

        g.pushLayer(layer); g.pushPolygonRadius(radius); {
            CARD_HEIGHT * CARD_ASPECT => float w;
            CARD_HEIGHT => float h;
            radius -=> w;
            radius -=> h;
            g.boxFilled(pos, w, h, selected ? Color.WHITE : Color.GRAY);
            CARD_BORDER.val() -=> w;
            CARD_BORDER.val() -=> h;
            g.boxFilled(pos, w, h, Color.BLACK);
        } g.popPolygonRadius(); g.popLayer();

        // TODO fix layer/depth sorting
        g.pushLayer(layer + .01); g.pushColor(suit_colors[c.suit]); {
            pos + CARD_HH * g.UP + CARD_HW * g.RIGHT => vec2 top_right;
            pos + CARD_HH * g.UP + CARD_HW * g.LEFT => vec2 top_left;
            .5 * CARD_HW => float suit_size;

            g.pushTextControlPoint(0, 1);
            g.text("" + c.val, top_left + CARD_INNER_PAD.val() * @(1, -1), 1.1*suit_size);
            g.popTextControlPoint();

            drawSuit(c.suit, top_right - (.5 * suit_size + CARD_INNER_PAD.val() + SUIT_SYMBOL_OFFSET.val()) * @(1, 1), suit_size);
        } g.popColor(); g.popLayer();
    }

    fun int legalToPickup() {
        // legal if this card and below are sorted in descending + alternating color order
        this @=> Card@ prev_card;
        for (this.stack_idx + 1 => int i; i < stack.cards.size(); i++) {
            stack.cards[i] @=> Card@ next_card;
            if (!oppositeColors(prev_card.suit, next_card.suit) || prev_card.val != next_card.val + 1)
                return false;
            next_card @=> prev_card;
        }
        return true;
    }
}

class Stack {
    vec2 pos;
    Card cards[0];

    fun void add(Card c) {
        // T.assert(c.stack != this, "adding duplicate card to stack");
        this @=> c.stack;
        cards.size() => c.stack_idx;
        cards << c;
    }

    fun int legalToAdd(Card c) {
        return (
            cards.size() == 0
            || 
            (
                Card.oppositeColors(c.suit, cards[-1].suit)
                &&
                cards[-1].val - 1 == c.val
            )
        );
    }

    // does NOT change the card's assigned stack and idx to handle if the player drops in invalid position,
    // can easily return card to original position
    fun Card remove(int idx) {
        cards[idx] @=> Card c;
        cards.erase(idx);
        return c;
    }

    fun int hovered(vec2 mouse_pos) {
        return (mouse_pos.x >= pos.x - CARD_HW) && (mouse_pos.x <= pos.x + CARD_HW);
    }

    fun vec2 cardPos(int card_idx) {
        STACK_OFFET_RATIO.val() * CARD_HEIGHT => float dy;
        return pos + dy * card_idx * g.DOWN;
    }

    fun void draw(vec3 color, vec2 mouse_pos) {
        // draw stack base
        g.pushColor(color);
        g.boxDotted(pos, CARD_WIDTH + .0, CARD_HEIGHT + 0.0, CARD_WIDTH / 9);
        g.popColor();

        // draw cards in stack
        STACK_OFFET_RATIO.val() * CARD_HEIGHT => float dy;
        for (int card_idx; card_idx < cards.size(); card_idx++) {
            this.cards[card_idx] @=> Card card;
            cardPos(card_idx) => vec2 card_pos;

            // check which card is hovered from stack
            int invert;
            (card_idx == this.cards.size() - 1) => int last_card;
            if (
                (last_card && M.inside(mouse_pos, M.aabb(card_pos, CARD_HW, CARD_HH)))
                ||
                (!last_card && M.inside(mouse_pos, M.aabb(card_pos + @(0, CARD_HH - .5 * dy), CARD_HW, .5*dy)))
            ) {
                true => invert;
                card @=> hovered_card;
            } 

            Card.draw(this.cards[card_idx], card_pos, card_idx, invert);
        }
    }
}

fun void drawSuit(int suit, vec2 pos, float sz) {
    .9 => float mod; // inner padding for sz
    if (suit == Suit_Club) {
        sz * .25 => float l;
        mod * .95 *  l * 2 / Math.sqrt(3) => float r2;

        M.rot2vec(-30 * M.DEG2RAD) => vec2 dir1;
        M.rot2vec(-150 * M.DEG2RAD) => vec2 dir2;
 
        g.circleFilled(pos + .9 * l * g.UP, r2);
        g.circleFilled(pos + l * dir1, r2);
        g.circleFilled(pos + l * dir2, r2);
        g.circleFilled(pos, .5 * r2);
        g.circleFilled(pos + 1.5 * l * g.DOWN, .25 * l);

    }
    else if (suit == Suit_Diamond) {
        g.diamond(pos, 0, mod * sz / 1.5, mod * sz);
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
        g.circleFilled(p1, l * .5);
        g.circleFilled(p2, l * .5);
        g.diamond(c, 0, 2*w, 2*w);

        // g.square(p1, 0, l, Color.WHITE);
        // g.square(pos, 0, 2*w, Color.WHITE);
    }

    // bbox
    // g.square(pos, 0, sz, Color.WHITE);
} 


Card@ hovered_card;

Card@ held_cards[0];
vec2 held_card_delta;
-1 => int hovered_stack_idx; // which stack the mouse is over. -1 if none


UI_Float STACK_SPACING(CARD_WIDTH * .25);
UI_Float STACK_START_Y(3); // absolute y position that stack centers begin

Stack stacks[6];
Card deck[0];

Stack ace_stack[4];

// add 2-10
// keep aces on side
for (int suit; suit < Suit_Count; suit++) {
    for (2 => int i; i <= 10; i++)
        deck << new Card(i, suit);
}

// shuffle
deck.shuffle();

// add to stack
for (int i; i < 6; i++) {
    for (int j; j < 6; j++) {
        stacks[i].add(deck[i*6 + j]);
    }
}


while (1) {
    GG.nextFrame() => now;
    g.mousePos() => vec2 mouse_pos;

    // ui
    UI.slider("card inner pad", CARD_INNER_PAD, 0, 1);
    UI.slider("suit symbol offset", SUIT_SYMBOL_OFFSET, -1, 1);
    UI.slider("stack spacing", STACK_SPACING, 0, 1);
    UI.slider("stack dy", STACK_OFFET_RATIO, 0, 1);

    // drawSuit(Suit_Club, @(0, 0), Color.WHITE, 1.0);
    // drawSuit(Suit_Diamond, @(0, 1), Color.WHITE, 1.0);
    // drawSuit(Suit_Heart, @(0, -1), Color.WHITE, 1.0);
    // drawSuit(Suit_Spade, @(0, -2), Color.WHITE, 1.0);

    { // update stack state
        // calculate stack centers
        -1 => hovered_stack_idx;
        @(
            -1 * (stacks.size() - 1) * (STACK_SPACING.val() * .5 + CARD_HW),
            STACK_START_Y.val()
        ) => vec2 center;
        for (int i; i < stacks.size(); i++) {
            stacks[i] @=> Stack stack;
            center => stack.pos;

            // check if mouse is in this column
            // TODO add y bounds checking
            Color.WHITE => vec3 color;
            if (stack.hovered(mouse_pos)) {
                i => hovered_stack_idx;
                Color.ORANGE => color;
            }

            // draw
            stack.draw(color, mouse_pos);

            // update center
            CARD_WIDTH + STACK_SPACING.val() +=> center.x;
        }
    }

    // if (hovered_card != null) <<< hovered_card.suit, hovered_card.val >>>;

    { // input
        if (g.mouseLeftDown() && hovered_card != null && hovered_card.legalToPickup()) {
            hovered_card.stack @=> Stack stack;
            // pick up entire stack
            T.assert(held_cards.size() == 0, "held cards should be clear");

            stack.cardPos(hovered_card.stack_idx) - g.mousePos() => held_card_delta;
            held_cards.size(stack.cards.size() - hovered_card.stack_idx);
            for (
                stack.cards.size() - 1 => int i;
                i >= hovered_card.stack_idx;
                i--
            ) {
                stack.remove(i) @=> held_cards[i - hovered_card.stack_idx];
            }
        }

        // drop cards
        if (g.mouseLeftUp() && held_cards.size()) {
            Stack @stack;
            // no stack hovered OR illegal move, put back in original
            if (hovered_card == null || !hovered_card.stack.legalToAdd(held_cards[0])) 
                held_cards[0].stack @=> stack;
            else
                hovered_card.stack @=> stack;

            // add to selected stack
            for (auto c : held_cards) stack.add(c);
            held_cards.clear();
        }

        // draw held cards
        STACK_OFFET_RATIO.val() * CARD_HEIGHT => float dy;
        for (int i; i < held_cards.size(); i++) {
            g.mousePos() + dy * i * g.DOWN + held_card_delta => vec2 card_pos;
            Card.draw(held_cards[i], card_pos, deck.size() + i, true);
        }
    }

    { // cleanup
        null @=> hovered_card;
    }
}
