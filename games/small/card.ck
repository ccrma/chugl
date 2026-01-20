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

UI QOL
- err msg when trying to swap ace with top card of playfield stack (can't swap, it wouldn't do anything)

TODO
- fixed over logic to test for card-card overlap
- fix transparency sorting for dropshadow and g2d_sprite
- for 2-10, draw symbols in pattern (see zachtronics for example)
- go right to left for aces and the card numbers + symbol, matching chinese scrolls
    - card symbol + number should be on top-right corner.
- don't draw drop shadow if being held
- juice
    - see emoji tsuika github for ideas 
        - interactive music/sfx toggle buttons
    - inkwash shader background (see guqin.ck)
        - green and orange color sources
- remove unused assets
    - font files
    - handwritten number pngs
- sfx
    - hovering over card "strums" it, mapped to pentatonic scale
        - e.g. running 2-10 is a gliss
*/

@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"

G2D g;
g.antialias(true);
GG.outputPass().gamma(false);

g.backgroundColor(.2 * Color.GREEN);

// == load assets ================================================
TextureLoadDesc tex_load_desc;
true => tex_load_desc.flip_y;
true => tex_load_desc.gen_mips;
Texture.load(me.dir() + "./card_assets/card2.png", tex_load_desc) @=> Texture card_back_sprite;

Texture.load(me.dir() + "./card_assets/duck.PNG", tex_load_desc) @=> Texture duck_sprite;
Texture.load(me.dir() + "./card_assets/magpie.PNG", tex_load_desc) @=> Texture magpie_sprite;
Texture.load(me.dir() + "./card_assets/orange.PNG", tex_load_desc) @=> Texture orange_sprite;
Texture.load(me.dir() + "./card_assets/persimmon.PNG", tex_load_desc) @=> Texture persimmon_sprite;

[
Texture.load(me.dir() + "./card_assets/A.PNG", tex_load_desc),
Texture.load(me.dir() + "./card_assets/2.PNG", tex_load_desc),
Texture.load(me.dir() + "./card_assets/3.PNG", tex_load_desc),
Texture.load(me.dir() + "./card_assets/4.PNG", tex_load_desc),
Texture.load(me.dir() + "./card_assets/5.PNG", tex_load_desc),
Texture.load(me.dir() + "./card_assets/6.PNG", tex_load_desc),
Texture.load(me.dir() + "./card_assets/7.PNG", tex_load_desc),
Texture.load(me.dir() + "./card_assets/8.PNG", tex_load_desc),
Texture.load(me.dir() + "./card_assets/9.PNG", tex_load_desc),
Texture.load(me.dir() + "./card_assets/10.PNG", tex_load_desc),
] @=> Texture number_sprites[];



fun Texture suit2sprite(int suit) {
    if (suit == Suit_Club) return duck_sprite;
    if (suit == Suit_Diamond) return orange_sprite;
    if (suit == Suit_Heart) return persimmon_sprite;
    if (suit == Suit_Spade) return magpie_sprite;
    return null;
}

GText.defaultFont(me.dir() + "./card_assets/ritegaki.otf");

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
] @=> vec3 suit_colors[];

[
    // old colors
    // new UI_Float3(Color.hex(0xffffcc)), // light orange
    // new UI_Float3(Color.hex(0xddeeff)), // light blue

    new UI_Float3(Color.hex(0xaaddff)), // duck - club
    new UI_Float3(Color.hex(0xffffaa)), // orange - diamond
    new UI_Float3(Color.hex(0xffffaa)), // persimmon - heart
    new UI_Float3(Color.hex(0xaaddff)), // magpie - spade
] @=> UI_Float3 ace_colors[];

[
    "duck",
    "orange",
    "persimmon",
    "magpie"
] @=> string card_symbol_names[];

2 => float CARD_HEIGHT;
5.0 / 7.0 => float CARD_ASPECT;
CARD_HEIGHT * CARD_ASPECT => float CARD_WIDTH;
CARD_HEIGHT * .5 => float CARD_HH;
CARD_WIDTH * .5 => float CARD_HW;

UI_Float CARD_BORDER(.05);
UI_Float CARD_ROUNDING(.1);
UI_Float CARD_INNER_PAD(.1);
UI_Float CARD_DOTTED_SEGMENT_LENGTH(CARD_WIDTH / 10);
UI_Float SYMBOL_SIZE(1.29);


UI_Float3 BACKGROUND_COLOR(.4 * Color.GREEN);

UI_Float SUIT_SYMBOL_OFFSET(-.05);

UI_Float STACK_OFFET_RATIO(.25); // ratio of CARD_HEIGHT to displace in Y axis when stacking

UI_Float STACK_SPACING(CARD_WIDTH * .25);
UI_Float STACK_START_Y(1); // absolute y position that stack centers begin
UI_Float ACE_STACK_START_Y(3.5); // absolute y position that stack centers begin


class Card {
    int val;
    int suit;

    int hovered;
    int selected;

    // stack params
    Stack@ stack;
    int stack_idx;

    fun Card(int val, int suit) {
        val => this.val; suit => this.suit;
    }

    fun void reset() {
        false => hovered;
        false => selected;
        null @=> stack;
        0 => stack_idx;
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

    fun int isTopCard() {
        return stack.isTopCard(this);
    }

    // get position in stack
    fun vec2 pos() {
        T.assert(stack != null, "cannot get pos of card not in stack");
        T.assert(stack.cards[stack_idx] == this, "stack_idx state not synced");
        return stack.cardPos(stack_idx);
    }

    fun static void draw(Card c, vec2 pos, int layer) {
        CARD_ROUNDING.val() * .5 => float radius;

        Color.WHITE => vec3 body_color;
        suit_colors[c.suit] => vec3 suit_color;

        // invert colors on ace
        if (c.val == 1) {
            body_color => vec3 tmp;
            suit_color => body_color;
            tmp => suit_color;
        }

        // force body color to be white
        // .8 * Color.WHITE => body_color;

        // draw card body
        g.pushLayer(layer); g.pushPolygonRadius(radius); {
            CARD_HEIGHT * CARD_ASPECT => float w;
            CARD_HEIGHT => float h;
            radius -=> w;
            radius -=> h;

            // draw border
            0 => float border_sca;
            if (c.selected || c.hovered) .06 => border_sca;

            g.pushPolygonRadius(radius + .5 * border_sca);
            g.boxFilled(pos, w + 0, h + 0, (c.selected || c.hovered) ? .8 * Color.hex(0xCD071E) : Color.BLACK);
            g.popPolygonRadius();

            1.0 * CARD_BORDER.val() -=> w;
            1.0 * CARD_BORDER.val() -=> h;

            if (c.val == 1) g.boxFilled(pos, w, h, ace_colors[c.suit].val());
            else g.boxFilled(pos, w, h, body_color);

            .07 => float shadow_displacement;
            g.pushLayer(layer - .01); 
            // g.pushAlpha(.1);
            // g.pushBlend(g.BLEND_MULT);
            g.boxFilled(pos - shadow_displacement * @(1, 1), w, h, .2 * Color.GREEN);
            // g.popAlpha(); 
            // g.popBlend();
            g.popLayer(); 
        } g.popPolygonRadius(); g.popLayer();

        // TODO fix layer/depth sorting
        g.pushLayer(layer + .01); g.pushColor(suit_color); {
            pos + CARD_HH * g.UP + CARD_HW * g.RIGHT => vec2 top_right;
            pos + CARD_HH * g.UP + CARD_HW * g.LEFT => vec2 top_left;
            .5 * CARD_HW => float suit_size;

            top_left + CARD_INNER_PAD.val() * @(1, -1) => vec2 number_pos;
            g.pushColor(Color.BLACK);
            g.pushTextControlPoint(0, 1);
            if (c.val == 1) g.text("A", number_pos, 1*suit_size);
            else g.text("" + c.val, number_pos, 1*suit_size);
            g.popTextControlPoint();
            g.popColor();

            top_right - (.5 * suit_size + CARD_INNER_PAD.val() + SUIT_SYMBOL_OFFSET.val()) * @(1, 1) => vec2 suit_pos;

            // draw suit
            // drawSuit(c.suit, top_right - (.5 * suit_size + CARD_INNER_PAD.val() + SUIT_SYMBOL_OFFSET.val()) * @(1, 1), suit_size);

            // audrey's numbers
            // g.pushColor(Color.BLACK);
            // g.sprite(number_sprites[c.val-1], number_pos, suit_size, 0);
            // g.popColor();

            // audge's art
            g.pushColor(Color.WHITE);
            g.sprite(suit2sprite(c.suit), suit_pos, suit_size * 1.2, 0);
            g.sprite(suit2sprite(c.suit), pos, CARD_HH * SYMBOL_SIZE.val(), 0);
            g.popColor();

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

    // ace stacks
    int is_ace_stack;
    int suit;

    fun int isTopCard(Card c) {
        return cards.size() && cards[-1] == c;
    }

    fun int empty() {
        return cards.size() == 0;
    }

    fun void clear() {
        while (cards.size()) remove(0);
    }

    fun void add(Card c) {
        // T.assert(c.stack != this, "adding duplicate card to stack");
        this @=> c.stack;
        cards.size() => c.stack_idx;
        cards << c;
    }

    fun int legalToAdd(Card c) {
        (
            cards.size() == 0
            || 
            (
                Card.oppositeColors(c.suit, cards[-1].suit)
                &&
                cards[-1].val - 1 == c.val
            )
        ) => int ret;
        <<< ret >>>;
        return ret;
    }

    // does NOT change the card's assigned stack and idx to handle if the player drops in invalid position,
    // can easily return card to original position
    fun Card remove(int idx) {
        cards[idx] @=> Card c;
        cards.erase(idx);
        return c;
    }

    // remove top card of stack
    fun void pop() {
        if (cards.size()) {
            cards.erase(cards.size() - 1);
        }
    }

    fun int hovered(vec2 mouse_pos) {
        return 
            (mouse_pos.x >= pos.x - CARD_HW) && (mouse_pos.x <= pos.x + CARD_HW)
            &&
            (mouse_pos.y >= pos.y - CARD_HH) && (mouse_pos.y <= pos.y + CARD_HH);
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
            Card.draw(this.cards[card_idx], card_pos, card_idx);
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

// == GAMESTATE ===============================

// room
0 => int Room_Deal;
1 => int Room_Play;
2 => int Room_Win;

// persistent card pool
Card deck[0];
Card aces[0];

Card@ hovered_card;
Stack@ hovered_stack; // which stack the mouse is over
Stack@ hovered_ace_stack; // which ace stack the mouse is over
Card@ selected_ace; // card selected from ace area. can also be a non-ace the player is trying to put back

Card@ held_cards[0];
vec2 held_card_delta; 

Stack stacks[7];
Stack ace_stack[4];

// ============================================

// called once per app-start, to initialize 
fun void init() {
    // create 2-10
    for (int suit; suit < Suit_Count; suit++) {
        for (2 => int i; i <= 10; i++)
            deck << new Card(i, suit);
    }

    // create aces and init ace stacks
    for (int suit; suit < Suit_Count; suit++) {
        aces << new Card(1, suit);
        true => ace_stack[suit].is_ace_stack;
        suit => ace_stack[suit].suit;
    }
}

// called once per hand/game to reset board
fun void deal() {
    // reset all cards
    for (auto card : deck) card.reset();
    for (auto card : aces) card.reset();

    // reset all stacks
    for (auto stack : stacks) stack.clear();
    for (auto stack : ace_stack) stack.clear();

    // add aces
    for (int suit; suit < Suit_Count; suit++) {
        ace_stack[suit].clear();
        ace_stack[suit].add(aces[suit]);
    }

    // shuffle
    deck.shuffle();

    // add non-aces
    for (int i; i < 6; i++) {
        for (int j; j < 6; j++) {
            stacks[i].add(deck[i*6 + j]);
        }
    }

    // move last empty stack to middle
    // stacks[-1] @=> Stack tmp;
    // stacks[3] @=> stacks[-1];
    // tmp @=> stacks[3];

    // reset held card state
    null @=> hovered_card;
    null @=> hovered_stack;
    null @=> hovered_ace_stack;
    null @=> selected_ace;

    held_cards.clear();
    @(0,0) => held_card_delta;
}

// returns true if all stacks are sorted
fun int winstate() {
    for (auto stack : stacks) {
        if (stack.empty()) continue;
        if (
            stack.cards[0].val != 10
            || stack.cards.size() < 9
            || !stack.cards[0].legalToPickup()
        ) return false;
    }
    return true;
}

init();
deal();

while (1) {
    GG.nextFrame() => now;
    g.mousePos() => vec2 mouse_pos;

    // ui
    UI.slider("card inner pad", CARD_INNER_PAD, 0, 1);
    UI.slider("card symbol size", SYMBOL_SIZE, 0, 2);
    UI.slider("suit symbol offset", SUIT_SYMBOL_OFFSET, -1, 1);
    UI.slider("stack spacing", STACK_SPACING, 0, 1);
    UI.slider("stack dy", STACK_OFFET_RATIO, 0, 1);
    UI.colorEdit("background color", BACKGROUND_COLOR);

    for (int i; i < Suit_Count; i++) {
        UI.pushID(i);
        UI.colorEdit("suit color "+ card_symbol_names[i], ace_colors[i]);
        UI.popID();
    }

    if (UI.button("reset")) deal();

    // drawSuit(Suit_Club, @(0, 0), Color.WHITE, 1.0);
    // drawSuit(Suit_Diamond, @(0, 1), Color.WHITE, 1.0);
    // drawSuit(Suit_Heart, @(0, -1), Color.WHITE, 1.0);
    // drawSuit(Suit_Spade, @(0, -2), Color.WHITE, 1.0);

    /* input logic

    mousedown
    - if ace_selected
        - if hover, try swap
        - else cancel selection
    - else
        - if hover, try pickup substack

    */

    { // input

        { // calculate hovered card and stacks
            // first check main playfield stacks
            for (auto s : stacks) {
                if (s.cards.size() == 0 && s.hovered(mouse_pos)) {
                    s @=> hovered_stack;
                }

                // draw cards in stack
                STACK_OFFET_RATIO.val() * CARD_HEIGHT => float dy;
                for (int card_idx; card_idx < s.cards.size(); card_idx++) {
                    s.cards[card_idx] @=> Card card;
                    s.cardPos(card_idx) => vec2 card_pos;

                    // check which card is hovered from stack
                    (card_idx == s.cards.size() - 1) => int last_card;
                    if (
                        (last_card && M.inside(mouse_pos, M.aabb(card_pos, CARD_HW, CARD_HH)))
                        ||
                        (!last_card && M.inside(mouse_pos, M.aabb(card_pos + @(0, CARD_HH - .5 * dy), CARD_HW, .5*dy)))
                    ) {
                        true => card.hovered;
                        card @=> hovered_card;
                    } else {
                        false => card.hovered;
                    }
                }
            }

            // then check ace stacks
            for (auto s : ace_stack) {
                s.hovered(mouse_pos) => int hovered;
                if (s.cards.size()) {
                    hovered => s.cards[-1].hovered;
                    if (hovered) s.cards[-1] @=> hovered_card;
                } else {
                    if (hovered) s @=> hovered_ace_stack;
                }
            }
        }

        if (g.mouseLeftDown()) {

            <<< "selected_ace", selected_ace >>>;
            if (selected_ace != null && selected_ace.val == 1) {
                // ace already selected, attempt to perform a swap
                if (hovered_card != null) {
                    <<< "attempting swap" >>>;
                    <<< hovered_card.isTopCard() >>>;
                    <<< !hovered_card.stack.is_ace_stack >>>;
                    <<< Card.oppositeColors(selected_ace.suit, hovered_card.suit) >>>;
                    <<< hovered_card.val == 2 >>>;
                    if (
                        hovered_card.suit == selected_ace.suit && !hovered_card.stack.is_ace_stack
                        // && !hovered_card.isTopCard()
                    ) {
                        // remove ace
                        selected_ace.stack.remove(0);

                        // copy data
                        selected_ace @=> hovered_card.stack.cards[hovered_card.stack_idx];
                        hovered_card.stack_idx => selected_ace.stack_idx;
                        hovered_card.stack @=> selected_ace.stack;

                        // add hovered to ace stack
                        ace_stack[selected_ace.suit].add(hovered_card);
                    }
                }

                false => selected_ace.selected;
                null @=> selected_ace;
            }
            else if (selected_ace == null && hovered_card != null && hovered_card.stack.is_ace_stack && hovered_card.val == 1) {
                <<< "selecting from ace stack" >>>;
                hovered_card @=> selected_ace;
                true => selected_ace.selected;
            }
            else if (selected_ace == null && hovered_card != null && hovered_card.legalToPickup()) {
                <<< "selecting from play stack" >>>;
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
                    stack.remove(i) @=> Card c;
                    c @=> held_cards[i - hovered_card.stack_idx];
                    true => c.selected;
                }
            }
        }

        // drop cards
        if (g.mouseLeftUp() && held_cards.size()) {
            Stack @stack;

            if (hovered_ace_stack != null) {
            <<< "hovered_stack", hovered_stack, "hovered_ace_stack", hovered_ace_stack >>>;
            <<< "held_cards", held_cards.size(), "hovered_ace_stack count", hovered_ace_stack.cards.size() >>>;
            }

            // no stack hovered OR illegal move, put back in originalA
            if (hovered_card != null && hovered_card.stack.legalToAdd(held_cards[0])) hovered_card.stack @=> stack;
            else if (hovered_stack != null && hovered_stack.legalToAdd(held_cards[0])) hovered_stack @=> stack;
            else if (held_cards.size() == 1 && hovered_ace_stack != null && hovered_ace_stack.empty() && held_cards[0].suit == hovered_ace_stack.suit) {
                <<< "putting card in ace stack" >>>;
                hovered_ace_stack @=> stack;
            }
            else held_cards[0].stack @=> stack;

            // add to selected stack
            for (auto c : held_cards) {
                false => c.selected;
                stack.add(c);
            }
            held_cards.clear();
        }
    } // input

    if (held_cards.size() == 0) { // if any aces are on top of playfield stacks, send back to ace stack
        for (auto s : stacks) {
            if (!s.empty() && s.cards[-1].val == 1) {
                s.cards[-1] @=> Card ace;
                if (ace_stack[ace.suit].empty()) {
                    // remove from current stack
                    s.pop();
                    // put it back
                    ace_stack[ace.suit].add(ace);
                }
            }
        }
    }


    { // draw held cards
        STACK_OFFET_RATIO.val() * CARD_HEIGHT => float dy;
        for (int i; i < held_cards.size(); i++) {
            g.mousePos() + dy * i * g.DOWN + held_card_delta => vec2 card_pos;
            Card.draw(held_cards[i], card_pos, deck.size() + i);
        }
    }

    { // update stack state
        // calculate stack centers
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
            if (stack == hovered_stack) Color.ORANGE => color;

            // draw
            stack.draw(color, mouse_pos);

            // update center
            CARD_WIDTH + STACK_SPACING.val() +=> center.x;
        }
    }

    { // update ace stacks
        // g.screen_w / 4 => float dx;
        // @(
        //     -1.5 * dx, ACE_STACK_START_Y.val()
        // ) => vec2 center;

        @(
            -1 * (stacks.size() - 1) * (STACK_SPACING.val() * .5 + CARD_HW),
            ACE_STACK_START_Y.val()
        ) => vec2 center;
        for (int i; i < ace_stack.size(); i++) {
            ace_stack[i] @=> Stack stack;
            center => stack.pos;

            // check if mouse is in this column
            // TODO add y bounds checking

            // draw
            Color.WHITE => vec3 color;
            if (hovered_ace_stack == stack) Color.ORANGE => color;
            stack.draw(color, mouse_pos);

            // update center
            // dx +=> center.x;
            1 * (CARD_WIDTH + STACK_SPACING.val()) +=> center.x;
        }
    }

    // draw background
    g.pushLayer(-1);
    g.sprite(card_back_sprite, @(0,0), 1.1*@(g.screen_w, g.screen_h), 0, BACKGROUND_COLOR.val());
    g.popLayer();

    { // cleanup
        null @=> hovered_card;
        null @=> hovered_stack;
        null @=> hovered_ace_stack;
    }
}
