/*
Ideas
- pot temperature should be a factor, can upgrade heat source?
    - food takes N units to cook, where units is determined by power of pot
    - a fuller pot means everything in it cooks slower
- broth levels decreasing, how does this effect the game?
- sauce bar
- beverages (tea)
    - separate game idea: tea brewing
- restaurant stocking / operations

- after food is done, you have to click to take it out of pot?
    - food takes same amount of energy to burn as it does to cook. If burned it's removed and you get no flavor points (but still have to eat it and it takes up volume)
    - research: automated chopsticks to take food out for you (at certain rate)

TODO
- napa cabbage buff (boosts broth flavor somehow?)

*/
// entity type enum
0 => int EntityType_Food;

// room type enum
0 => int RoomType_Eat;
1 => int RoomType_Upgrade;

[
    "Eat",
    "Upgrade"
] @=> string room_type_names[];

// food type enum
0 => int FoodType_Spinach;
1 => int FoodType_Cabbage;
2 => int FoodType_CrownDaisy;
3 => int FoodType_Count;

class FoodCategory
{
    string name;
    float volume;        // amt of space the food takes up (in pot and stomach)
    float base_fp_rate;  // fp/sec that this food accumulates while cooking
    dur cook_time; // time to cook to completion
    dur overcook_dur; 

    int cost_to_unlock;
    0 => int unlocked;

    0 => int last_add_failed;
    time last_add_attempt;  // time of last attempt to add to pot

    fun init(string n, float f, float b, dur c, int cost_to_unlock) {
        n => this.name;
        f => this.volume;
        b => this.base_fp_rate;
        c => this.cook_time;
        .5 * this.cook_time => this.overcook_dur;
        cost_to_unlock => this.cost_to_unlock;
    }
}

FoodCategory foods[FoodType_Count];
foods[FoodType_Spinach].init("Spinach", 1.0, 1, 5::second, 0); 
foods[FoodType_Cabbage].init("Cabbage", 2.5, 2, 12::second, 500);
foods[FoodType_CrownDaisy].init("Crown Daisy (Tong Hao)", 2.5, 3, 15::second, 1000);

// init food unlocks
true => foods[FoodType_Spinach].unlocked;

// Upgrade Type enum
// 0 => int UpgradeType_Workout; // maybe working out is *Training* not an Upgrade
// 0 => int UpgradeType_PotSize;
// class Upgrade
// {
// }


class Entity
{
    // type system
    int entity_type;
    int food_type;

    // cooking stats
    float accum_fp;           // the accumulated amount of FPs
    dur accum_cook_time;      // amount of time it's been cooked for
    int finished_cooking;   
    time finished_cooking_time;    // time that food reached 100% cooked
    float finished_cooking_fp;    // max FPs upon finishing cooking


    fun static Entity makeFood(int food_type) {
        Entity e;
        EntityType_Food => e.entity_type;
        food_type => e.food_type;
        return e;
    }
}

class GameState
{
    false => int game_state_initialized;

    // active room 
    UI_Int active_room_type;

    // player stats
    float flavor_points;
    100.0 => float stomach_max_cap;
    float stomach_curr_cap;
    -1 => float metabolism;

    // pot stats
    20.0 => float pot_max_cap;
    float pot_curr_cap;

    // currently cooking
    Entity cooking[0];

    // dev console
    UI_Float dev_flavor_points;
    UI_Float dev_time_factor(1.0);
    UI_Float font_global_scale(1.25);

    // Colors
    UI.convertHSVtoRGB(@(2.0/7, 0.7, 0.7)) => vec3 Color_ItemUnlocked;
    @(
        Color_ItemUnlocked.x,
        Color_ItemUnlocked.y,
        Color_ItemUnlocked.z,
        1.0
    ) => vec4 Color_ItemUnlockedVec4;
    UI.convertHSVtoRGB(@(0.0/7, 0.7, 0.7)) => vec3 Color_ItemTooExpensive;
    @(
        Color_ItemTooExpensive.x,
        Color_ItemTooExpensive.y,
        Color_ItemTooExpensive.z,
        1.0
    ) => vec4 Color_ItemTooExpensiveVec4;

    UI.convertHSVtoRGB(@(4.0/7, 0.7, 0.7)) => vec3 Color_ItemAffordable;

    // Upgrade Shop

    fun float clamp01(float f) {
        return Math.clampf(f, 0, 1);
    }

    fun vec3 lerp(vec3 a, vec3 b, float t) {
        return a * (1-t) + b * t;
    }

    fun void tooltip(FoodCategory category) {
        if (UI.beginItemTooltip()) {
            UI.text("Volume: " + category.volume);
            UI.text("Flavor Points: " + category.base_fp_rate + "/second");
            UI.text("Cook Time: " + (category.cook_time / second) + " seconds");
            UI.endTooltip();
        }
    }

    fun int coloredButton(string text, vec3 color) {
        UI.pushStyleColor(UI_Color.Button, color * .9);
        UI.pushStyleColor(UI_Color.ButtonHovered, color);
        UI.pushStyleColor(UI_Color.ButtonActive, color * 1.1);

        UI.button(text) => int clicked;
        UI.popStyleColor(3);
        return clicked;
    }

    fun int centeredButton(string text, vec2 size, vec3 color) { // TODO put in ulib_imgui
        UI.pushStyleColor(UI_Color.Button, color * .9);
        UI.pushStyleColor(UI_Color.ButtonHovered, color);
        UI.pushStyleColor(UI_Color.ButtonActive, color * 1.1);

        UI.getContentRegionAvail().x => float avail_width;
        UI.setCursorPosX(UI.getCursorPosX() + (avail_width - size.x) * .5);

        UI.button(text, size) => int clicked;

        UI.popStyleColor(3);
        return clicked;
    }

    fun void init() {
        if (game_state_initialized) return;
        GG.nextFrame() => now;

        true => game_state_initialized;
        
        // TODO: broken
        // me.dir() + "../../../assets/fonts/DroidSans.ttf" => string droid_sans_filepath;
        // UI.addFontFromFileTTF(droid_sans_filepath, 16) @=> UI_Font droid_sans_font;
    }

    fun void update(float dt_sec)
    {
        dev_time_factor.val() *=> dt_sec;

        if (active_room_type.val() == RoomType_Eat) {

            // process foods
            // ==optimize== double buffer or swap with last instead of deletion
            for (int i; i < cooking.size(); i++) {
                cooking[i] @=> Entity food;
                foods[food.food_type] @=> FoodCategory food_category;

                // clamp to max time
                Math.clampf((food_category.cook_time - food.accum_cook_time) / second, 0.0, dt_sec) => float remaining_sec;
                <<< remaining_sec >>>;
                remaining_sec::second => dur remaining_dur;

                // add cook time
                dt_sec::second +=> food.accum_cook_time;

                // accumulate points from cooking
                remaining_sec * food_category.base_fp_rate +=> food.accum_fp; 

                // mark if done cooking
                if (!food.finished_cooking && food.accum_cook_time > food_category.cook_time) {
                    true => food.finished_cooking;
                    now - dt_sec::second + remaining_dur => food.finished_cooking_time;
                    food.accum_fp => food.finished_cooking_fp;
                }

                // lose points from overcooking
                if (food.finished_cooking) {
                    food.finished_cooking_fp * clamp01(1.0 - ((now - food.finished_cooking_time) / food_category.overcook_dur)) => food.accum_fp;
                }
                <<< food.accum_fp >>>;
            }
        }


        if (active_room_type.val() == RoomType_Upgrade) {
            // metabolize
            Math.max(0, stomach_curr_cap + dt_sec * metabolism) => stomach_curr_cap;
        }
    }

    // returns true if successfully added to pot
    fun int addToPot(int food_type) {
        T.assert(food_type >= 0 && food_type <= FoodType_Count, "Invalid Food Type: " + food_type);
        foods[food_type] @=> FoodCategory cat;
        now => cat.last_add_attempt;
        true => cat.last_add_failed;

        // volume check
        if (pot_curr_cap + cat.volume > pot_max_cap) return false;

        // fullness check
        if (stomach_curr_cap + cat.volume > stomach_max_cap) return false;

        // else add
        cat.volume +=> pot_curr_cap;
        cooking << Entity.makeFood(food_type);

        false => cat.last_add_failed;
        return true;
    }

    // for now drawing also handles user input...
    fun void draw()
    {
        UI.getStyle() @=> UI_Style ui_style;

        // fullscreen
        UI.getMainViewport() @=> UI_Viewport @ viewport;
        UI.setNextWindowPos(viewport.workPos(), UI_Cond.Always);
        UI.setNextWindowSize(viewport.workSize(), UI_Cond.Always);
        UI.pushStyleVar(UI_StyleVar.WindowRounding, 0.0);
        if (UI.begin("HOTPOT HERO", null, UI_WindowFlags.NoDecoration )) {
            UI.getWindowSize() => vec2 window_size;

            { // left pane
                UI.getContentRegionAvail() => vec2 pane_size;

                false => int ui_disabled;
                if (active_room_type.val() != RoomType_Eat) {
                    UI.beginDisabled();
                    true => ui_disabled;
                }

                UI.beginChild(
                    "EAT",
                    @(window_size.x * .5, window_size.y * .5),
                    // @(0,0),
                    UI_ChildFlags.Border,
                    0
                );

                UI.text("Flavor Points: " + (flavor_points));
                // stomach capacity
                (stomach_curr_cap $ int) + "/" + (stomach_max_cap $ int) => string stomach_cap_str;
                UI.progressBar(stomach_curr_cap / stomach_max_cap, @(0,0), null);
                UI.sameLine();
                if (active_room_type.val() != RoomType_Eat && stomach_curr_cap > 0) {
                    UI.textColored(Color_ItemUnlockedVec4, "DIGESTING " + stomach_cap_str);
                } else {
                    UI.text("Stomach " + stomach_cap_str);
                }

                // pot capacity
                UI.progressBar(pot_curr_cap / pot_max_cap, @(0,0), null);
                UI.sameLine();
                UI.text("Pot Capacity " + (pot_curr_cap $ int) + "/" + (pot_max_cap $ int));

                UI.separatorText("Menu");  // -------------MENU--------------
                for (int food_type; food_type < foods.size(); food_type++) {
                    foods[food_type] @=> FoodCategory food_category;
                    if (!food_category.unlocked) continue;

                    // USER INPUT
                    if (UI.button(food_category.name)) {
                        addToPot(food_type);
                    }
                    if (food_category.last_add_failed && now - food_category.last_add_attempt < 1::second) {
                        UI.sameLine();
                        UI.textColored(Color_ItemTooExpensiveVec4, "Cannot Add, already FULL");
                    } 
                }

                // -------------Cooking--------------
                UI.separatorText("COOKING"); 
                for (int i; i < cooking.size(); i++) {
                    cooking[i] @=> Entity food;
                    foods[food.food_type] @=> FoodCategory category;

                    // check finished
                    if (food.accum_cook_time >= category.cook_time) {
                        // overcook meter
                        clamp01(1.0 - (now - food.finished_cooking_time) / category.overcook_dur) => float overcook_progress;
                        UI.progressBar(overcook_progress, @(0,0), (food.accum_fp $ int) + " flavor");
                        UI.sameLine();
                        // lerp color between affordable and item too expensive
                        lerp(Color_ItemTooExpensive, Color_ItemAffordable, overcook_progress) => vec3 eat_button_col;
                        if (coloredButton("EAT " + category.name + "##" + i, eat_button_col)) {
                            // get the ratio of this you can it
                            Math.clampf((stomach_max_cap - stomach_curr_cap)  / category.volume, 0.0, 1.0) => float ratio;

                            // add points
                            (ratio * food.accum_fp) +=> flavor_points;
                            // add to stomach
                            (ratio * category.volume) +=> stomach_curr_cap;
                            // remove from pot
                            category.volume -=> pot_curr_cap;

                            cooking.popOut(i); // SLOW!!!
                            i--;

                            // sanity checks
                            T.assert(pot_curr_cap >= 0 && pot_curr_cap <= pot_max_cap, "pot capacity error: curr/max" + pot_curr_cap + "/" + pot_max_cap);
                        }
                    } else {
                        UI.progressBar(food.accum_cook_time / category.cook_time, @(0,0), (food.accum_fp $ int) + " flavor");
                        UI.sameLine();
                        UI.text(category.name);
                    }
                }

                // check end game (stomach full)
                Math.fabs(stomach_curr_cap - stomach_max_cap) < .0001 => int stomach_full;
                if (stomach_full) {
                    if (centeredButton("MAI DAN", @(pane_size.x * .2, 40), Color.GOLD)) {
                        RoomType_Upgrade => active_room_type.val;
                    }
                }

                UI.endChild(); // "Left Generator"

                if (ui_disabled) {
                    UI.endDisabled();
                }
            }

            UI.sameLine();

            { // right pane
                UI.getContentRegionAvail() => vec2 pane_size;

                // room check
                false => int ui_disabled;
                if (active_room_type.val() != RoomType_Upgrade) {
                    true => ui_disabled;
                    UI.beginDisabled();
                }

                UI.beginChild(
                    "UPGRADE",
                    // @(window_size.x * .5,0),
                    @(-1,window_size.y * .5),
                    // @(size.x * .2, 0), 
                    UI_ChildFlags.Border,
                    0
                );

                UI.separatorText("Body Upgrades");

                // TODO: should there be progress bars / time requirements for these too?
                (flavor_points >= 2000) => int work_out_affordable;
                if (work_out_affordable) {
                    if (coloredButton("Work Out (-2000 Flavor Points) (+10 stomach capacity) (+.1 metabolism)", Color_ItemAffordable)) {
                        2000 -=> flavor_points;
                        10 +=> stomach_max_cap;
                        .2 -=> metabolism;
                    }
                    UI.sameLine();
                    UI.text("2000 FPs");
                } else {
                    coloredButton("Work Out (+10 stomach capacity) (+.1 metabolism)", Color_ItemTooExpensive);
                    UI.sameLine();
                    UI.textColored(Color_ItemTooExpensiveVec4, "2000 FPs");
                }

                UI.separatorText("Pot Upgrades");

                UI.separatorText("Food Upgrades");
                for (int i; i < foods.size(); i++) {
                    foods[i] @=> FoodCategory category;

                    if (category.unlocked) {
                        UI.pushStyleColor(UI_Color.Button, Color_ItemUnlocked);
                        UI.pushStyleColor(UI_Color.ButtonHovered, Color_ItemUnlocked);
                        UI.pushStyleColor(UI_Color.ButtonActive, Color_ItemUnlocked);

                        UI.button(category.name);
                        tooltip(category);
                        UI.sameLine();
                        UI.text("Unlocked");

                        UI.popStyleColor(3);
                    } else if (flavor_points >= category.cost_to_unlock) {
                        // UI.pushStyleColor(UI_Color.Button, UI.convertHSVtoRGB(@(2.0/7, 0.6, 0.6)));
                        // UI.pushStyleColor(UI_Color.ButtonHovered, UI.convertHSVtoRGB(@(2.0/7, 0.7, 0.7)));
                        // UI.pushStyleColor(UI_Color.ButtonActive, UI.convertHSVtoRGB(@(2.0/7, 0.8, 0.8)));

                        // USER INPUT
                        if (UI.button(category.name)) {
                            // subtract from account
                            category.cost_to_unlock -=> flavor_points;
                            true => category.unlocked;
                        }
                        tooltip(category);

                        UI.sameLine();
                        UI.text(category.cost_to_unlock + " FPs");
                    } else if (flavor_points < category.cost_to_unlock) {
                        UI.pushStyleColor(UI_Color.Button, Color_ItemTooExpensive);
                        UI.pushStyleColor(UI_Color.ButtonHovered, Color_ItemTooExpensive);
                        UI.pushStyleColor(UI_Color.ButtonActive, Color_ItemTooExpensive);

                        UI.button(category.name);
                        tooltip(category);
                        UI.sameLine();
                        UI.textColored(Color_ItemTooExpensiveVec4, category.cost_to_unlock + " FPs");

                        UI.popStyleColor(3);
                    }
                }

                UI.dummy(@(0.0f, 20.0f)); // vertical spacing

                if (centeredButton("LET'S EAT", @(pane_size.x * .2, 40), Color.GOLD)) {
                    RoomType_Eat => active_room_type.val;
                }

                UI.endChild();

                if (ui_disabled) {
                    UI.endDisabled();
                }
            }

            // developer console
            {
                UI.beginChild(
                    "DEV CONSOLE",
                    // @(window_size.x * .5,0),
                    @(0, 0),
                    // @(size.x * .2, 0), 
                    UI_ChildFlags.Border,
                    0
                );

                UI.separatorText("Developer Console");

                if (UI.listBox("Active Room", active_room_type, room_type_names)) {
                    <<< "setting room to: ", room_type_names[active_room_type.val()] >>>;
                }

                flavor_points => dev_flavor_points.val;
                if (UI.drag("Flavor Points", dev_flavor_points)) {
                    dev_flavor_points.val() => flavor_points;
                }

                UI.slider("Time Scale", dev_time_factor, .1, 10);

                UI.slider("Font Scale", font_global_scale, .1, 5);
                font_global_scale.val() => UI_IO.fontGlobalScale;

                // TODO: add font support eventually...
                // see ImFontAtlas, ImFont, ImFontConfig in cimgui
                UI.showFontSelector("Font");

                UI.endChild();
            }

        }
        UI.end();
        UI.popStyleVar(1); // WindowRounding = 0.0
    }


}

GameState gs;
gs.init();

while (true) {
    GG.nextFrame() => now;

    gs.update(GG.dt());
    gs.draw();
}