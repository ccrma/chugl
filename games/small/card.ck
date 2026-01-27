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

Instructions
- stack cards by alternating type and decreasing value. Sorted stacks can be moved together.
- win by sorting all number cards 10-2 into 4 stacks
- Aces are special cards that can be swapped with any card of the same suit. 
    - Once in play, aces may be stacked like any other card. 
    - Aces return to their empty free cell when possible, and can then be swapped again.
- Free cells in the top left may be filled with any card of a matching suit.

TODO
- fix transparency sorting for dropshadow and g2d_sprite
- juice
    - animate the ace swap and ace returning with a zeno interp
    - see emoji tsuika github for ideas 
        - interactive music/sfx toggle buttons
- remove unused assets
    - font files
    - handwritten number pngs
- sfx
    - bamboo sound for shuffle?
    - reso or beats for card swap or ace click
- music
    - birdcalls! (magpie and duck)
    - terry sonic pinwheel
    - geodesics energy?
    - thinking.ck
- art
    - small alpha hole in persimmon (obvious on ace stack all white)


*/

@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"
@import "card_music.ck"

G2D g;
g.antialias(true);
GG.outputPass().gamma(false);

g.backgroundColor(.2 * Color.GREEN);


// == video ================================================

class YCrCbMaterial extends Material {
    "
    #include FRAME_UNIFORMS
    #include DRAW_UNIFORMS
    #include STANDARD_VERTEX_INPUT

    struct VertexOutput {
        @builtin(position) position : vec4f,
        @location(1) v_uv : vec2f,
    };

    @group(1) @binding(0) var texture_sampler: sampler;
    @group(1) @binding(1) var texture_y: texture_2d<f32>;   
    @group(1) @binding(2) var texture_cr: texture_2d<f32>;   
    @group(1) @binding(3) var texture_cb: texture_2d<f32>;   
    @group(1) @binding(4) var<uniform> crop_uv: vec2f;


    @vertex 
    fn vs_main(in : VertexInput) -> VertexOutput
    {
        var out : VertexOutput;
        var u_Draw : DrawUniforms = u_draw_instances[in.instance];

        let worldpos = u_Draw.model * vec4f(in.position, 1.0f);
        out.position = (u_frame.projection * u_frame.view) * worldpos;
        out.v_uv     = in.uv;

        // crop UV because yCrCb planes are always rounded to a multiple of 16
        out.v_uv *= crop_uv;

        return out;
    }

    // YCrCb --> srgb conversion matrix
    const rec601 = mat4x4f(
        1.16438,  0.00000,  1.59603, -0.87079,
        1.16438, -0.39176, -0.81297,  0.52959,
        1.16438,  2.01723,  0.00000, -1.08139,
        0, 0, 0, 1
    );

    @fragment 
    fn fs_main(in : VertexOutput) -> @location(0) vec4f
    {   
        var y = textureSample(texture_y, texture_sampler, in.v_uv).r;
        var cb = textureSample(texture_cb, texture_sampler, in.v_uv).r;
        var cr = textureSample(texture_cr, texture_sampler, in.v_uv).r;

        let col_srgb = vec4f(y, cb, cr, 1.0) * rec601;

        // after multiplying with the conversion matrix, we are now in gamma/srgb space.
        // convert back to linear space so the final color doesn't look overly bright
        let col_linear = pow(col_srgb, vec4f(2.2));

        return col_linear;
    }
    " => static string shader_code;

    static Shader@ ycrcb_shader;
    if (ycrcb_shader == null) {
        ShaderDesc shader_desc;
        shader_code => shader_desc.vertexCode;
        shader_code => shader_desc.fragmentCode;

        new Shader(shader_desc) @=> ycrcb_shader;
    }

    // set shader
    ycrcb_shader => this.shader;

    fun @construct(Video video) {
        // set uniform defaults
        this.sampler(0, TextureSampler.linear());
        this.texture(1, video.textureY());
        this.texture(2, video.textureCr());
        this.texture(3, video.textureCb());
            // YCrCb dimensions always a multiple of 16, so set a crop window to remove empty pixels
        this.uniformFloat2(
            4, 
            @(
                video.width() $ float / video.textureY().width(),
                video.height() $ float / video.textureY().height()
            )
        ); 
    }
}

// Video video( "/Users/Andrew/Downloads/chugl-sizzle.mpg" ) => dac; 
// video.mode(Video.MODE_YCRCB);

// material for rendering YCrCb planes
// YCrCbMaterial ycrcb_material(video);

// FlatMaterial rgba_material;
// rgba_material.colorMap(video.texture());

// GMesh mesh(new PlaneGeometry, ycrcb_material) --> GG.scene();
// mesh.posZ(10);
// mesh.scaX(3 * (video.width() $ float / video.height()));
// mesh.scaY(-3);


// == load assets ================================================
TextureLoadDesc tex_load_desc;
true => tex_load_desc.flip_y;
true => tex_load_desc.gen_mips;

Texture.load(me.dir() + "./card_assets/victory.png", tex_load_desc) @=> Texture victory_sprite;
Texture.load(me.dir() + "./card_assets/card.png", tex_load_desc) @=> Texture card_back_sprite;

Texture.load(me.dir() + "./card_assets/duck.PNG", tex_load_desc) @=> Texture duck_sprite;
Texture.load(me.dir() + "./card_assets/magpie.PNG", tex_load_desc) @=> Texture magpie_sprite;
Texture.load(me.dir() + "./card_assets/orange.PNG", tex_load_desc) @=> Texture orange_sprite;
Texture.load(me.dir() + "./card_assets/persimmon.PNG", tex_load_desc) @=> Texture persimmon_sprite;

Texture.load(me.dir() + "./card_assets/Instructions-new.PNG", tex_load_desc) @=> Texture rule_sprite;

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

0 => int VictoryAnim_Left;
1 => int VictoryAnim_Right;
2 => int VictoryAnim_Up;
3 => int VictoryAnim_Count;

0 => int Room_Deal;
1 => int Room_Play;
2 => int Room_Win;

[
    // old colors
    // new UI_Float3(Color.hex(0xffffcc)), // light orange
    // new UI_Float3(Color.hex(0xddeeff)), // light blue

    new UI_Float3(Color.hex(0xaaddff)), // duck - club
    new UI_Float3(Color.hex(0xffffaa)), // orange - diamond
    new UI_Float3(Color.hex(0xffffaa)), // persimmon - heart
    new UI_Float3(Color.hex(0xaaddff)), // magpie - spade
] @=> UI_Float3 ace_colors[];

.8 * Color.hex(0xCD071E) => vec3 selected_color;

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

"./card_data.txt" => string SAVE_FILE;

UI_Float CARD_BORDER(.05);
UI_Float CARD_ROUNDING(.1);
UI_Float CARD_INNER_PAD(.085);
UI_Float CARD_DOTTED_SEGMENT_LENGTH(CARD_WIDTH / 10);
UI_Float ACE_SYMBOL_SIZE(1.29);
UI_Float SYMBOL_SIZE(.4);
UI_Float ACE_STACK_SYMBOL(.5);

UI_Float CARD_ZENO_INTERP(.16);

UI_Float3 BACKGROUND_COLOR(.4 * Color.GREEN);

UI_Float SUIT_SYMBOL_OFFSET(-.05);

UI_Float STACK_OFFET_RATIO(.22); // ratio of CARD_HEIGHT to displace in Y axis when stacking

UI_Float STACK_SPACING(CARD_WIDTH * .25);
UI_Float STACK_START_Y(1); // absolute y position that stack centers begin
UI_Float ACE_STACK_START_Y(3.5); // absolute y position that stack centers begin

UI_Float RULEBOOK_ANIM_ZENO(.12);

vec2 symbol_positions[0][0];
// init positions
for (int i; i < 11; i++) {
    symbol_positions << symbolPositions(CARD_HH / 5.25, CARD_HW / 2, i);
}

// dy corresponds to smallest increment in x or y
// try CARD_HH / 5 or something
fun vec2[] symbolPositions(float dy, float dx, int n) {

    if (n == 1) {
        return [@(0, 0)];
    }

    if (n == 2) {
        return [
            @(0, dy),
            @(0, -dy),
        ];
    }

    if (n == 3) {
        return [
            @(-dx, -2 * dy),
            @(0, 0),
            @(dx, 2 * dy),
        ];
    }

    if (n == 4) {
        return [
            @(dx, 2 * dy),
            @(-dx, 2 * dy),
            @(-dx, -2 * dy),
            @(dx, -2 * dy),
        ];
    }

    if (n == 5) {
        return [
            @(dx, 2 * dy),
            @(-dx, 2 * dy),
            @(-dx, -2 * dy),
            @(dx, -2 * dy),
            @(0, 0),
        ];
    }

    if (n == 6) {
        return [
            @(dx, 2 * dy),
            @(-dx, 2 * dy),
            @(-dx, -2 * dy),
            @(dx, -2 * dy),
            @(dx, 0),
            @(-dx, 0),
        ];
    }

    if (n == 7) {
        return [
            @(dx, 2 * dy),
            @(-dx, 2 * dy),
            @(-dx, -2 * dy),
            @(dx, -2 * dy),
            @(dx, 0),
            @(-dx, 0),
            @(0, dy),
        ];
    }

    if (n == 8) {
        return [
            @(dx, 2 * dy),
            @(-dx, 2 * dy),
            @(-dx, -2 * dy),
            @(dx, -2 * dy),
            @(0, dy),
            @(0, -dy),
            @(0, 3*dy),
            @(0, -3*dy),
        ];
    }

    if (n == 9) {
        return [
            @(dx, 2 * dy),
            @(-dx, 2 * dy),
            @(-dx, -2 * dy),
            @(dx, -2 * dy),
            @(0, dy),
            @(0, 3*dy),
            @(0, -3*dy),
            @(dx, 0),
            @(-dx, 0),
        ];
    }
    
    if (n == 10) {
        return [
            @(dx, 2 * dy),
            @(-dx, 2 * dy),
            @(-dx, -2 * dy),
            @(dx, -2 * dy),
            @(0, dy),
            @(0, -dy),
            @(0, 3*dy),
            @(0, -3*dy),
            @(dx, 0),
            @(-dx, 0),
        ];
    }

    return null;
}

class Card {
    int val;
    int suit;

    int hovered;
    int selected;

    // stack params
    Stack@ stack;
    int stack_idx;

    // animation state (only zeno animates during dealing/shuffle and automatic ace swap)
    int was_swapped;
    float anim_layer;
    vec2 deal_curr_pos;


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
        if (stack == null) return @(0,0);
        return stack.cardPos(stack_idx);
    }

    fun static void draw(Card c, vec2 pos, float layer) {
        // if being dealt, zeno interp towards pos
        if (room == Room_Deal || c.was_swapped) {
            // update visual position
            CARD_ZENO_INTERP.val() * (pos - c.deal_curr_pos) +=> c.deal_curr_pos;

            if (c.was_swapped) {
                c.anim_layer => layer;
                if (M.dist(c.deal_curr_pos, pos) < .0001) {
                    false => c.was_swapped;
                }
            }

            c.deal_curr_pos => pos;
        } 

        CARD_ROUNDING.val() * .5 => float radius;

        .96 * Color.WHITE => vec3 body_color;

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
            g.boxFilled(pos, w + 0, h + 0, (c.selected || c.hovered) ? selected_color : Color.BLACK);
            g.popPolygonRadius();

            1.0 * CARD_BORDER.val() -=> w;
            1.0 * CARD_BORDER.val() -=> h;

            if (c.val == 1) g.boxFilled(pos, w, h, ace_colors[c.suit].val());
            else g.boxFilled(pos, w, h, body_color);

            if (!c.selected && !c.hovered) {
                .07 => float shadow_displacement;
                g.pushLayer(layer - .01); 
                // g.pushAlpha(.1);
                // g.pushBlend(g.BLEND_MULT);
                g.boxFilled(pos - shadow_displacement * @(1, 1), w, h, .2 * Color.GREEN);
                // g.popAlpha(); 
                // g.popBlend();
                g.popLayer(); 
            }
        } g.popPolygonRadius(); g.popLayer();

        // TODO fix layer/depth sorting
        g.pushLayer(layer + .01); 
        {
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

            // audge's art
            g.pushColor(Color.WHITE);
            g.sprite(suit2sprite(c.suit), suit_pos, suit_size * 1.2, 0);

            if (c.val == 1) g.sprite(suit2sprite(c.suit), pos, CARD_HH * ACE_SYMBOL_SIZE.val(), 0);
            else {
                for (auto delta : symbol_positions[c.val]) {
                    g.sprite(suit2sprite(c.suit), pos + delta - @(0, .16), suit_size * 1.12, 0);
                }
            }

            g.popColor();

        } 
        g.popLayer();
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

    fun void add(Card c, int animate) {
        if (c.stack != null) {
            if (animate) {
                c.pos() => c.deal_curr_pos;
                true => c.was_swapped;
                c.stack_idx + 1 => c.anim_layer;
            }
        }

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

        // draw symbol if ace stack
        if (is_ace_stack) {
            g.pushBlend(g.BLEND_MULT);
            g.pushEmission(ACE_STACK_SYMBOL.val()*Color.WHITE);
            g.pushColor(Color.BLACK);
            g.sprite(suit2sprite(suit), pos, CARD_HH * ACE_SYMBOL_SIZE.val(), 0);
            g.popColor();
            g.popEmission();
            g.popBlend();
        }

        // draw cards in stack
        STACK_OFFET_RATIO.val() * CARD_HEIGHT => float dy;
        for (int card_idx; card_idx < cards.size(); card_idx++) {
            this.cards[card_idx] @=> Card card;
            cardPos(card_idx) => vec2 card_pos;
            Card.draw(this.cards[card_idx], card_pos, card_idx + 1);
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

int num_wins;

float gametime;
float win_time;

// options
true => int sfx_on;
true => int music_on;

// room
Room_Play => int room;
int display_rules;

// victory animation
int victory_anim;
int victory_color;

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

// == sound ================================================
class Sound {
    ADSR sfx_adsr => GVerb gverb => dac;
    sfx_adsr.keyOn();
    gverb.revtime(.5::second);
    gverb.roomsize(20);

    CardMusic card_music;
    card_music.rev => ADSR music_adsr => gverb;
    card_music.rev.gain(0);
    music_adsr.keyOn(); // default music on

    SinOsc win_sounds[5];
    ADSR win_adsr(2::second, 4::second, 0, 1::second) => gverb;
    for (auto s : win_sounds) {
        .04 => s.gain;
        s => win_adsr;
    }

    SndBuf magpie_forest(me.dir() + "./card_assets/magpie-moon.wav") => HPF magpie_bpf(1000, 1) => music_adsr;
    1 => magpie_forest.loop;

    ModalBar pickup_card_sfx => LPF ooo(8000) => sfx_adsr;
    0 => pickup_card_sfx.preset;  // marimba
    .8 => pickup_card_sfx.gain;

    ModalBar reject_sfx => LPF r(10000) => sfx_adsr;
    5 => reject_sfx.preset;
    1.5 => reject_sfx.masterGain;

    ModalBar click_nothing => sfx_adsr;
    5 => click_nothing.preset;
    1.5 => click_nothing.masterGain;

    ModalBar swap0 => sfx_adsr;
    6 => swap0.preset;
    2 => swap0.masterGain;
    ModalBar swap1 => sfx_adsr;
    6 => swap1.preset;
    2 => swap1.masterGain;

    fun void enableSfx(int b) {
        if (b) sfx_adsr.keyOn();
        else sfx_adsr.keyOff();
    }

    fun void enableMusic(int b) {
        if (b) music_adsr.keyOn();
        else music_adsr.keyOff();
    }


    fun void win() {
        freq(Math.random2(1,10)) => win_sounds[0].freq;
        freq(Math.random2(1,10)) => win_sounds[1].freq;
        freq(Math.random2(1,10)) => win_sounds[2].freq;
        freq(Math.random2(1,10)) => win_sounds[3].freq;
        freq(Math.random2(1,10)) => win_sounds[4].freq;
        win_adsr.keyOn();
    }

    fun void swap(Card@ a, Card@ b) {
        freq(a.val) => swap0.freq;
        freq(b.val) => swap1.freq;
        2 * swap0.noteOn(Math.random2f(.9, 1));
        2 * swap1.noteOn(Math.random2f(.9, 1));
    }

    fun void returnAce(Card@ ace) {
        freq(ace.val) => swap0.freq;
        2 * swap0.noteOn(Math.random2f(.9, 1));
    }

    Shakers bamboo => LPF bamboo_lpf(2300);
    8 => bamboo.gain;
    22 => bamboo.which;

    [
        -3,
        0,  // A
        2,  // 1
        4,  // 2
        7,  // 3
        9,  // 4
        11, // 5
        12, // 6
        14, // 7
        16, // 8
        19, // 9
        21, // 10
        23, // J
    ] @=> int val2midi[];
    60 => int root;

    [
        new SndBuf(me.dir() + "./card_assets/paper_med_1.wav"),
        new SndBuf(me.dir() + "./card_assets/paper_med_2.wav"),
        new SndBuf(me.dir() + "./card_assets/paper_med_4.wav"),
    ] @=> SndBuf paper_bank[];
    BPF paper_hpf(1500, 2) => sfx_adsr;
    for (auto s : paper_bank) {
        s => paper_hpf;
        s.rate(0);
    }

    fun float freq(int val) {
        return Std.mtof(root + val2midi[val]);
    }

    fun void shuffle() { spork ~ shuffleImpl(); }
    fun void shuffleImpl() {
        bamboo => sfx_adsr;
        freq(Math.random2(1, 10)) => bamboo.freq;
        (Math.random2(10, 60)) => bamboo.objects;
        2 => bamboo.noteOn;
        .7::second => now;
        bamboo.noteOff(.0);
        bamboo =< sfx_adsr;
    }

    fun void paper() { 
        paper_bank[Math.random2(0, paper_bank.size()-1)] @=> SndBuf@ buf;
        buf.pos(0);
        buf.rate(1);
    }

    fun void pickup(Card@ c, vec2 card_pos) { 
        Math.remap(
            c.val * (c.suit + 1),
            1, 40,
            .2, 1
        ) => pickup_card_sfx.stickHardness;
        Math.clampf(M.dist(card_pos, g.mousePos()), 0, 1) => pickup_card_sfx.strikePosition;
        freq(c.val) => pickup_card_sfx.freq;
        pickup_card_sfx.noteOn(Math.random2f(.9, 1.0));
    }


    fun void reject() { 
        Math.random2f(0, 1) => click_nothing.stickHardness;
        Math.random2f(0, 1) => click_nothing.strikePosition;
        Math.random2f(0, 1) => click_nothing.vibratoGain;
        Math.random2f(0, 60) => click_nothing.vibratoFreq;
        click_nothing.noteOn(Math.random2f(.9, 1.0));
    }

    fun void reject(Card@ c) { 
        Math.remap(
            c.val * (c.suit + 1),
            1, 40,
            .0, 1
        ) => reject_sfx.stickHardness;
        Math.clampf(M.dist(c.pos(), g.mousePos()), 0, 1) => reject_sfx.strikePosition;
        freq(c.val) => reject_sfx.freq;
        reject_sfx.noteOn(Math.random2f(.9, 1.0));
    }

    
    [
        -3,
        0,  // A
        2,  // 1
        4,  // 2
        7,  // 3
        9,  // 4
        12, // 5
        14, // 6
        16, // 7
        19, // 8
        21, // 9
        24, // 10
        26, // J
    ] @=> int val2midi_music[];
    fun void musicShred() {
        while (1) {
            Math.random2(8, 16) * 8::second => now;
            card_music.rev.gain(.18);

            card_music.bass_env.keyOn();
            8::second => now;
            card_music.tenor_env.keyOn();
            8::second => now;
            card_music.osc_env.keyOn();
            8::second => now;
            card_music.alto_env.keyOn();
            8::second => now;
            card_music.osc2_env.keyOn();
            8::second => now;
            card_music.soprano_env.keyOn();
            8::second => now;
            card_music.shimmer_switch.keyOn();
            8::second => now;
            8::second => now;
            card_music.mainOff(8::second);
            card_music.allOff();
        }
    } spork ~ musicShred();

    // tried setting melody based on current cards, but sounds repetitive
    fun void soundbankShred() {
        while (1) {
            8::second => now;
            // load current stack state into notes
            card_music.notebank.clear();
            for (auto s : stacks) {
                maybe => int rise; // if true, adds card stack in reverse order
                if (!rise) {
                    for (auto c : s.cards) {
                        card_music.notebank << val2midi_music[c.val] + root;
                    }
                } else {
                    for (s.cards.size() - 1 => int i; i >= 0; i--) {
                        card_music.notebank << val2midi_music[s.cards[i].val] + root;
                    }

                }
            }
        }
    } spork ~ soundbankShred();
}
Sound sfx;

// called once per app-start, to initialize 
fun void init() {
    readWinCount() => num_wins;

    // create 2-10
    for (int suit; suit < Suit_Count; suit++) {
        for (2 => int i; i <= 10; i++)
            deck << new Card(i, suit);
    }

    // create aces and init ace stacks
    @(
        -1 * (stacks.size() - 1) * (STACK_SPACING.val() * .5 + CARD_HW),
        ACE_STACK_START_Y.val()
    ) => vec2 center;
    for (int suit; suit < Suit_Count; suit++) {
        aces << new Card(1, suit);
        true => ace_stack[suit].is_ace_stack;
        suit => ace_stack[suit].suit;

        center + @((suit * (CARD_WIDTH + STACK_SPACING.val())), 0) => ace_stack[suit].pos;
    }
}

// called once per hand/game to reset board
int deal_gen;
true => int first_deal;
fun void deal() {
    ++deal_gen => int curr_gen;

    // reset timers
    0 => gametime;
    0 => win_time;

    // reset all cards and set to dealing
    for (auto card : deck) {
        // if statement to handle spam-clicking reset
        if (room != Room_Deal) card.pos() => card.deal_curr_pos;
        card.reset();
    }
    for (auto card : aces) {
        // if statement to handle spam-clicking reset
        if (room != Room_Deal) card.pos() => card.deal_curr_pos;
        card.reset();
    }

    Room_Deal => room;

    // reset all stacks
    for (auto stack : stacks) stack.clear();
    for (auto stack : ace_stack) stack.clear();

    if (first_deal) {
        1::second => now;
        false => first_deal;
    }

    // add aces
    for (int suit; suit < Suit_Count; suit++) {
        ace_stack[suit].clear();
        ace_stack[suit].add(aces[suit], false);
    }

    // shuffle
    deck.shuffle();

    // add non-aces
    for (int i; i < 6; i++) {
        for (int j; j < 6; j++) {
            stacks[i].add(deck[i*6 + j], false);
        }
    }

    sfx.shuffle();

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

    1::second => now;

    if (deal_gen == curr_gen) {
        Room_Play => room;
    }
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

fun void saveWinCount() {
    // instantiate a file IO object
    FileIO fout;
    // open for write (default mode: ASCII)
    fout.open( me.dir() + SAVE_FILE, FileIO.WRITE );

    // test
    if( !fout.good() ) {
        cherr <= "can't open file for writing..." <= IO.newline();
        return;
    }

    // write some stuff
    fout.write(num_wins);

    fout.close();
}

fun int readWinCount() {
    // instantiate
    FileIO fio;

    // open a file
    fio.open( me.dir() + SAVE_FILE, FileIO.READ );

    // ensure it's ok
    if( !fio.good() ) {
        cherr <= "cannot read" <= IO.newline();
        return 0;
    }

    // variable to read into
    int val;
    fio => val;

    return val;
}


init();
spork ~ deal();

Color.hex(0xCD071E) => vec3 RED;
[
    ace_colors[1].val(), Color.BLACK,
    Color.BLACK, ace_colors[1].val(),
    ace_colors[0].val(), Color.BLACK,
    Color.BLACK, ace_colors[0].val(),
    RED, Color.BLACK,
    Color.BLACK, RED,
    ace_colors[1].val(), RED,
    RED, ace_colors[1].val(),
    ace_colors[0].val(), RED,
    RED, ace_colors[0].val(),
] @=> vec3 victory_color_combos[];

fun void victoryAnimation(float elapsed) {
    g.pushLayer(20);

    victory_color_combos[victory_color * 2 + 0] => vec3 bg_color;
    victory_color_combos[victory_color * 2 + 1] => vec3 char_color;

    // scroll params
    .08 => float border;
    g.screen_w => float w;
    0 => float x;
    .66 * g.screen_min.y => float target_y;
    target_y => float y;

    // scroll progress
    .8 => float scroll_time;
    1 - Math.clampf(elapsed / scroll_time, 0, 1) => float t;

    if (victory_anim == VictoryAnim_Left) M.lerp(t*t, 0, w+1) => x;
    else if (victory_anim == VictoryAnim_Right) M.lerp(t*t, 0, -w-1) => x;
    else if (victory_anim == VictoryAnim_Up) M.lerp((t)*(t), target_y, g.screen_min.y - 1.2) => y;

    // fade in character .2 seconds after scroll
    Math.clampf(elapsed - scroll_time - .2, 0, 1) => float t2;

    // draw scroll border
    g.pushPolygonRadius(CARD_ROUNDING.val() * .5 + .5 * border);
    g.pushLayer(19);
    g.boxFilled(@(x, y), w, 2 , char_color);
    g.popLayer();
    g.popPolygonRadius();

    // draw scroll
    g.pushPolygonRadius(CARD_ROUNDING.val() * .5);
    g.boxFilled(@(x, y), w + border, 2 - border, bg_color);
    g.popPolygonRadius();

    // draw sheng
    g.pushEmission(.95*char_color);
    g.pushAlpha(t2);
    g.sprite(victory_sprite, @(0,y), 2, 0);
    g.popAlpha();
    g.popEmission();

    g.popLayer();
}

float rulebook_zeno; // at 0, off screen. At 1, on screen;
fun void drawRules() {
    // set zeno target
    @(0, 0) => vec2 onscreen_target;
    @(1.1 * g.screen_w, 0) => vec2 offscreen_target;
    display_rules ? 1 : 0 => float zeno_target;

    // lerp towards target
    // RULEBOOK_ANIM_ZENO.val() * (target - rulebook_pos) +=> rulebook_pos;
    RULEBOOK_ANIM_ZENO.val() * (zeno_target - rulebook_zeno) +=> rulebook_zeno;
    M.lerp(rulebook_zeno, offscreen_target, onscreen_target) => vec2 rulebook_pos;

    // calculate size (preserving 2:1 aspect ratio)
    30 => float layer;
    .92 * g.screen_w => float w;
    Math.min(.5 * w, .9 * g.screen_h) => float h;
    2 * h => w;

    // draw
    g.pushLayer(layer);
    g.sprite(rule_sprite, rulebook_pos, @(w, h), 0);
    g.popLayer();
}

fun int button(string text, vec2 center, vec3 color) {
    M.inside(g.mousePos(), center - @(1.1, .3), center) => int inside;

    if (inside) {
        g.pushColor(color);
    } else {
        g.pushColor(.8 * Color.WHITE);
    }
    g.text(text, center, .5 * CARD_HW);
    g.popColor();

    return inside && g.mouseLeftDown();
}


// empty frame to load
GG.nextFrame() => now;


false => int do_ui;
while (1) {
    GG.nextFrame() => now;
    g.mousePos() => vec2 mouse_pos;
    GG.dt() +=> gametime;

    // ui
    if (do_ui) {
        UI.slider("card inner pad", CARD_INNER_PAD, 0, 1);
        UI.slider("card symbol size", SYMBOL_SIZE, 0, 2);
        UI.slider("suit symbol offset", SUIT_SYMBOL_OFFSET, -1, 1);
        UI.slider("stack spacing", STACK_SPACING, 0, 1);
        UI.slider("stack dy", STACK_OFFET_RATIO, 0, 1);
        UI.colorEdit("background color", BACKGROUND_COLOR);
        UI.slider("acemat symbol", ACE_STACK_SYMBOL, 0, 1);
        UI.slider("card anim zeno", CARD_ZENO_INTERP, 0, 1);

        for (int i; i < Suit_Count; i++) {
            UI.pushID(i);
            UI.colorEdit("suit color "+ card_symbol_names[i], ace_colors[i]);
            UI.popID();
        }
    }

    // instructions input
    int bypass_rule_button;
    if (display_rules) {
        // exit instructions on click
        if (g.mouseLeftDown()) {
            false => display_rules;
            true => bypass_rule_button;
            sfx.paper();
        }
    }

    { // game ui
        @(
            (stacks.size() - 1) * (STACK_SPACING.val() * .5 + CARD_HW) + CARD_HW,
            ACE_STACK_START_Y.val() + CARD_HH
        ) => vec2 center;
        g.pushTextControlPoint(1, 1);

        // check isect
        if (M.inside(mouse_pos, center - @(1.1, .3), center)) {
            g.pushColor(ace_colors[0].val());
            if (g.mouseLeftDown()) {
                spork ~ deal();
            }
        } else {
            g.pushColor(.8 * Color.WHITE);
        }
        g.text("reset", center, .5 * CARD_HW);
        g.popColor();
        // bbox
        // g.box(center - @(1.1, .3), center);

        .4 * CARD_HH -=> center.y;

        if (M.inside(mouse_pos, center - @(1.1, .3), center)) {
            g.pushColor(ace_colors[1].val());
            if (!bypass_rule_button && g.mouseLeftDown()) {
                sfx.paper();
                // sfx.swoosh();
                true => display_rules;
            }
        } else {
            g.pushColor(.8 * Color.WHITE);
        }
        g.text("rules", center, .5 * CARD_HW);
        g.popColor();

        .5 * CARD_HH -=> center.y;

        if (button("sfx:" + (sfx_on ? "on" : "off"), center, ace_colors[2].val())) {
            !sfx_on => sfx_on;
            sfx.enableSfx(sfx_on);
        }

        .5 * CARD_HH -=> center.y;

        if (button("music:" + (music_on ? "on" : "off"), center, ace_colors[3].val())) {
            !music_on => music_on;
            sfx.enableMusic(music_on);
            sfx.reject();
        }

        .5 * CARD_HH -=> center.y;
        g.pushColor(.8 * Color.WHITE);
        g.text("W:" + num_wins, center, .5 * CARD_HW);
        g.popColor();

        g.popTextControlPoint();
    }

    if (room == Room_Play && !display_rules) { // play input
        { // calculate hovered card and stacks
            // first check main playfield stacks
            for (auto s : stacks) {
                if (s.cards.size() == 0 && s.hovered(mouse_pos)) {
                    s @=> hovered_stack;
                }

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


            // if still no hovered card, check if held card overlaps stack top
            if (hovered_card == null && held_cards.size()) {
                // keep in sync with drawing of held cards
                STACK_OFFET_RATIO.val() * CARD_HEIGHT => float dy;
                mouse_pos + held_card_delta => vec2 held_card_pos;
                for (auto s : stacks) {
                    // skip all stacks except the one same column as mouse
                    if (mouse_pos.x < s.pos.x - CARD_HW || mouse_pos.x > s.pos.x + CARD_HW) continue;
                    if (s.empty()) continue;
                    if (M.aabbIsect(held_card_pos, @(CARD_HW, CARD_HH), s.cards[-1].pos(), @(CARD_HW, CARD_HH))) {
                        true => s.cards[-1].hovered;
                        s.cards[-1] @=> hovered_card;
                        break;
                    } 
                }
            }

            // then check ace stacks
            if (hovered_card == null) {
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
        }

        if (g.mouseLeftDown()) {
            if (selected_ace != null && selected_ace.val == 1) {
                // ace already selected, attempt to perform a swap
                if (hovered_card != null) {
                    if (
                        hovered_card.suit == selected_ace.suit && !hovered_card.stack.is_ace_stack
                    ) {
                        // remove ace
                        selected_ace.pos() => selected_ace.deal_curr_pos;
                        hovered_card.stack_idx + 1 => selected_ace.anim_layer;
                        true => selected_ace.was_swapped;
                        selected_ace.stack.remove(0);

                        // copy data
                        selected_ace @=> hovered_card.stack.cards[hovered_card.stack_idx];
                        hovered_card.stack_idx => selected_ace.stack_idx;
                        hovered_card.stack @=> selected_ace.stack;

                        // add hovered to ace stack
                        ace_stack[selected_ace.suit].add(hovered_card, true);

                        // play sound
                        sfx.swap(selected_ace, hovered_card);
                    } else {
                        sfx.reject(selected_ace);
                    }
                } else {
                    sfx.reject(selected_ace);
                }

                false => selected_ace.selected;
                null @=> selected_ace;
            }
            else if (selected_ace == null && hovered_card != null && hovered_card.stack.is_ace_stack && hovered_card.val == 1) {
                hovered_card @=> selected_ace;
                true => selected_ace.selected;
                false => selected_ace.was_swapped;
                sfx.pickup(selected_ace, selected_ace.pos());
            }
            else if (selected_ace == null && hovered_card != null && hovered_card.legalToPickup()) {
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
                    false => c.was_swapped;
                }

                // play pickup card sfx
                sfx.pickup(held_cards[0], held_cards[0].pos());
            }
            else if (hovered_card != null && !hovered_card.legalToPickup()) {
                sfx.reject(hovered_card);
            }
        }

        // drop cards
        if (g.mouseLeftUp() && held_cards.size()) {
            held_cards[0].stack @=> Stack original_stack;
            Stack @stack;

            // no stack hovered OR illegal move, put back in original
            if (hovered_card != null && hovered_card.stack.legalToAdd(held_cards[0])) hovered_card.stack @=> stack;
            else if (hovered_stack != null && hovered_stack.legalToAdd(held_cards[0])) hovered_stack @=> stack;
            else if (held_cards.size() == 1 && hovered_ace_stack != null && hovered_ace_stack.empty() && held_cards[0].suit == hovered_ace_stack.suit) {
                hovered_ace_stack @=> stack;
            }
            else held_cards[0].stack @=> stack;

            if (stack == original_stack) {
                // sfx.swoosh();
                // sfx.pickup(held_cards[0], held_cards[0].pos());
                sfx.reject(held_cards[0]);
            } else {
                vec2 pos;
                Card@ c;
                if (stack.empty()) {
                    stack.pos => pos;
                    held_cards[0] @=> c;
                }
                else {
                    stack.cards[-1] @=> c;
                    stack.cards[-1].pos() => pos;
                }
                sfx.pickup(c, pos);
            }

            // add to selected stack
            for (auto c : held_cards) {
                false => c.selected;
                stack.add(c, false);
            }
            held_cards.clear();
        }
    } // input

    if (held_cards.size() == 0) { 
        for (auto s : stacks) {
            if (!s.empty() && s.cards[-1].val == 1) {
                s.cards[-1] @=> Card ace;
                if (ace_stack[ace.suit].empty()) {
                    // remove from current stack
                    s.pop();
                    // put it back
                    ace_stack[ace.suit].add(ace, true);
                    sfx.returnAce(ace);
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
        for (int i; i < ace_stack.size(); i++) {
            ace_stack[i] @=> Stack stack;

            // draw
            Color.WHITE => vec3 color;
            if (hovered_ace_stack == stack) {
                Color.ORANGE => color;
            }
            stack.draw(color, mouse_pos);
        }
    }

    // draw background
    g.pushLayer(-1);
    g.sprite(card_back_sprite, @(0,0), 1.1*@(g.screen_w, g.screen_h), 0, BACKGROUND_COLOR.val());
    g.popLayer();

    // check win condition
    if (room == Room_Play && winstate() || (UI.button("win"))) {
        Room_Win => room;
        gametime => win_time;

        ++num_wins;
        saveWinCount();

        sfx.win();

        // deselect all cards
        for (auto c : deck) {
            false => c.hovered;
            false => c.selected;
        }

        // randomize win animation params
        Math.random2(0, VictoryAnim_Count - 1) => victory_anim;
        Math.random2(0, victory_color_combos.size() / 2 - 1) => victory_color;
    }
    
    // draw rules
    drawRules();
    
    // draw victory screen
    if (room == Room_Win) victoryAnimation(gametime - win_time);

    { // cleanup
        null @=> hovered_card;
        null @=> hovered_stack;
        null @=> hovered_ace_stack;
    }
}
